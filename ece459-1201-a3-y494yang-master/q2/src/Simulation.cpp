#include "Simulation.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>

using std::cerr;
using std::cout;
using std::endl;

int print_flag=1;

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

bool retIsEmpty(const std::vector<Particle*> &ret) {
    for (Particle* p : ret) {
        if (p != nullptr) {
            return false;
        }
    }

    return true;
}

void reset(std::vector<Particle*> &v) {
    for (int i = 0; i < v.size(); i++) {
        delete v[i];
        v[i] = nullptr;
    }
}

//-----------------------------------------------------------------------------
// Simulation
//-----------------------------------------------------------------------------

Simulation::Simulation(float initTimeStep, float errorTolerance)
    : initTimeStep(initTimeStep)
    , errorTolerance(errorTolerance) {
    // nop
}

Simulation::~Simulation() {
    reset(y0);
    reset(y1);
    reset(z1);
}

//-----------------------------------------------------------------------------
// Input/output
//-----------------------------------------------------------------------------

void Simulation::readInputFile(std::string inputFile) {
    std::ifstream file(inputFile);
    if (!file.is_open()) {
        cerr << "Unable to open file: " << inputFile << endl;
        exit(1);
    }

    std::string line;
    while (getline(file, line)) {
        std::stringstream ss(line);
        std::string token;

        getline(ss, token, ',');
        Particle::ParticleType type = (token == "p" ? Particle::PROTON : Particle::ELECTRON);

        getline(ss, token, ',');
        float x = std::stod(token);

        getline(ss, token, ',');
        float y = std::stod(token);

        getline(ss, token, ',');
        float z = std::stod(token);

        y0.push_back(new Particle(
            type,
            Vec3(x, y, z)
        ));
    }
}

void Simulation::print() {
    int digitsAfterDecimal = 5;
    int width = digitsAfterDecimal + std::string("-0.e+00").length();

    for (const Particle* p : z1) {
        char type;
        switch (p->type) {
            case Particle::PROTON:
                type = 'p';
                break;
            case Particle::ELECTRON:
                type = 'e';
                break;
            default:
                type = 'u';
        }

        cout << type << ","
             << std::scientific
             << std::setprecision(digitsAfterDecimal)
             << std::setw(width)
             << p->position.x << ","
             << std::setw(width)
             << p->position.y << ","
             << std::setw(width)
             << p->position.z << endl;
    }
}

//-----------------------------------------------------------------------------
// OpenCL Simulation
//-----------------------------------------------------------------------------

#ifdef USE_OPENCL

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

void Simulation::computeForces(std::vector<Vec3> &ret, const std::vector<Particle*> &particles) {
    assert(ret.size() == 0);
    ret.resize(particles.size());

    #pragma omp parallel for
    for (int i = 0; i < particles.size(); i++) {
        Vec3 totalForces;

        for (int j = 0; j < particles.size(); j++) {
            totalForces += particles[i]->computeForceOnMe(particles[j]);
        }

        ret[i] = totalForces;
    }

    assert(ret.size() == particles.size());
}

void Simulation::run() {
    try { 
        // Get available platforms
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        // Select the default platform and create a context using this platform and the GPU
        cl_context_properties cps[3] = { 
            CL_CONTEXT_PLATFORM, 
            (cl_context_properties)(platforms[0])(), 
            0 
        };
        cl::Context context(CL_DEVICE_TYPE_GPU, cps);
 
        // Get a list of devices on this platform
        std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
 
        // Create a command queue and use the first device
        cl::CommandQueue queue = cl::CommandQueue(context, devices[0]);
 
        // Read source file
        std::ifstream sourceFile("src/protons.cl");
            if(!sourceFile.is_open()){
                std::cerr << "Cannot find kernel file" << std::endl;
                throw;
            }
        std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
        cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
 
        // Make program of the source code in the context
        cl::Program program = cl::Program(context, source);
 
        // Build program for these specific devices
        try {
            program.build(devices);
        } catch(cl::Error error) {
            std::cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
            throw;
        }

 
        //init vars
        const int numParticles = y0.size();
        k0.reserve(numParticles);
        k1.reserve(numParticles);
        y1.resize(numParticles);
        z1.resize(numParticles);
        float h = initTimeStep;
        k0.clear();


        // Create buffers
        cl::Buffer Buffer_k0 = cl::Buffer(context, CL_MEM_READ_WRITE, numParticles * sizeof(cl_float3));
        cl::Buffer Buffer_k1 = cl::Buffer(context, CL_MEM_READ_WRITE, numParticles * sizeof(cl_float3));
        cl::Buffer Buffer_y0 = cl::Buffer(context, CL_MEM_READ_ONLY, numParticles * sizeof(cl_float4));
        cl::Buffer Buffer_y1 = cl::Buffer(context, CL_MEM_READ_WRITE, numParticles * sizeof(cl_float4));
        cl::Buffer Buffer_z1 = cl::Buffer(context, CL_MEM_READ_WRITE, numParticles * sizeof(cl_float4));

        cl::Buffer Buffer_h = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_float));
        cl::Buffer Buffer_tolerance = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_float));
        cl::Buffer Buffer_num = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_int));


        //------2. write to y0 buffer
        cl_float4 y0_f4[numParticles];
        for(int i = 0; i < numParticles; i++)
        {
            y0_f4[i].s[0] = y0[i]->position.x;
            y0_f4[i].s[1] = y0[i]->position.y;
            y0_f4[i].s[2] = y0[i]->position.z;
            y0_f4[i].s[3] = y0[i]->type;
        }
        queue.enqueueWriteBuffer(Buffer_y0, CL_TRUE, 0, numParticles * sizeof(cl_float4), y0_f4);

        computeForces(k0, y0);
        //-----1. write to k0 buffer
        cl_float3 k0_f3[numParticles];
        for(int i = 0; i < numParticles; i++)
        {
            k0_f3[i].s[0] = k0[i].x;
            k0_f3[i].s[1] = k0[i].y;
            k0_f3[i].s[2] = k0[i].z;
        }
        queue.enqueueWriteBuffer(Buffer_k0, CL_TRUE, 0, numParticles * sizeof(cl_float3), k0_f3);


        cl_int* num_Particles = new cl_int();
        *num_Particles = numParticles;
        queue.enqueueReadBuffer(
            Buffer_num,
            CL_TRUE,
            0,
            sizeof(cl_int),
            num_Particles
        );

        cl::NDRange globalSize(numParticles);
        cl::Kernel kernel0(program, "computeForces");

        while(true)
        {
            k1.clear();
            reset(y1);
            reset(z1);
            //-----set arg computeApproxPositions
            cl_float *h_cl = new cl_float();
            *h_cl = h;
            queue.enqueueWriteBuffer(Buffer_h, CL_TRUE, 0, sizeof(cl_float), h_cl);

            //run computeApproxPositions
            cl::Kernel kernel1(program, "computeApproxPositions");
            kernel1.setArg(0, Buffer_h);
            kernel1.setArg(1, Buffer_k0);
            kernel1.setArg(2, Buffer_y0);
            kernel1.setArg(3, Buffer_y1);
            queue.enqueueNDRangeKernel(kernel1, cl::NullRange, globalSize); 


            kernel0.setArg(0, Buffer_k1);
            kernel0.setArg(1, Buffer_y1);
            kernel0.setArg(2, Buffer_num);
           queue.enqueueNDRangeKernel(kernel0, cl::NullRange, globalSize);



            cl::Kernel kernel2(program, "computeBetterPositions");
            //-------set arg computeBetterPositions
            kernel2.setArg(0, Buffer_h);
            kernel2.setArg(1, Buffer_k0);
            kernel2.setArg(2, Buffer_k1);
            kernel2.setArg(3, Buffer_y0);
            kernel2.setArg(4, Buffer_z1);

            //run computeBetterPositions
            queue.enqueueNDRangeKernel(kernel2, cl::NullRange, globalSize); 

            //-------prepare isErrorAcceptable
            cl_float *err_tolerance = new cl_float();
            *err_tolerance = errorTolerance;           
            queue.enqueueWriteBuffer(Buffer_tolerance, CL_TRUE, 0, sizeof(cl_float), err_tolerance);

            cl::Kernel kernel3(program, "isErrorAcceptable");
            kernel3.setArg(0, Buffer_tolerance);
            kernel3.setArg(1, Buffer_y1);
            kernel3.setArg(2, Buffer_z1);
            cl_int* accept_flag = new cl_int[numParticles];
            cl::Buffer accept_Buffer = cl::Buffer(context, CL_MEM_READ_WRITE, numParticles * sizeof(cl_int));
            queue.enqueueWriteBuffer(accept_Buffer, CL_TRUE, 0, numParticles * sizeof(cl_int), accept_flag);
            kernel3.setArg(3, accept_Buffer);

            //run isErrorAcceptable
            queue.enqueueNDRangeKernel(kernel3, cl::NullRange, globalSize); 

            cl_int* flag_rtn = new cl_int[numParticles];
            queue.enqueueReadBuffer(
                accept_Buffer,
                CL_TRUE,
                0,
                numParticles * sizeof(cl_int),
                flag_rtn
            );

            int all_done = 1;
            for(int i = 0; i < numParticles; i++)
            {
                if(flag_rtn[i] == 0){
                    all_done = 0;
                    break;
                }
            }
            if (all_done == 1)
            {
		cl_float4* z1_rtn = new cl_float4[numParticles];
            	queue.enqueueReadBuffer(
                    Buffer_z1,
                    CL_TRUE,
                    0,
                    numParticles*sizeof(cl_float4),
                    z1_rtn
            	);
		for(int i = 0; i < numParticles; i++)
            	{
                    z1[i] = new Particle(z1_rtn[i].s[3] == 1.f ? Particle::PROTON : Particle::ELECTRON, Vec3(z1_rtn[i].s[0], z1_rtn[i].s[1], z1_rtn[i].s[2]));
            	}
                break;
            }

            h /= 2.0;
        }

    } catch(cl::Error error) {
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
    }
}

// ifdef USE_OPENCL
#endif

//-----------------------------------------------------------------------------
// CPU Simulation
//-----------------------------------------------------------------------------

#ifndef USE_OPENCL

void Simulation::computeForces(std::vector<Vec3> &ret, const std::vector<Particle*> &particles) {
    assert(ret.size() == 0);
    ret.resize(particles.size());

    #pragma omp parallel for
    for (int i = 0; i < particles.size(); i++) {
        Vec3 totalForces;

        for (int j = 0; j < particles.size(); j++) {
            totalForces += particles[i]->computeForceOnMe(particles[j]);
        }

        ret[i] = totalForces;
    }

    assert(ret.size() == particles.size());
}

void Simulation::computeApproxPositions(const float h) {
    assert(y0.size() == k0.size());
    assert(retIsEmpty(y1));

    #pragma omp parallel for
    for (int i = 0; i < y0.size(); i++) {
        float mass = y0[i]->getMass();
        Vec3 f = k0[i];

        // h's unit is in seconds
        //
        //         F = ma
        //     F / m = a
        //   h F / m = v
        // h^2 F / m = d

        Vec3 deltaDist = f * std::pow(h, 2) / mass;
        y1[i] = new Particle(y0[i]->type, y0[i]->position + deltaDist);
    }
}

void Simulation::computeBetterPositions(const float h) {
    assert(y0.size() == k0.size());
    assert(y0.size() == k1.size());
    assert(retIsEmpty(z1));

    #pragma omp parallel for
    for (int i = 0; i < y0.size(); i++) {
        float mass = y0[i]->getMass();
        Vec3 f0 = k0[i];
        Vec3 f1 = k1[i];

        Vec3 avgForce = (f0 + f1) / 2.0;
        Vec3 deltaDist = avgForce * std::pow(h, 2) / mass;
        Vec3 y1 = y0[i]->position + deltaDist;

        z1[i] = new Particle(y0[i]->type, y1);
    }
}

bool Simulation::isErrorAcceptable(const std::vector<Particle*> &p0, const std::vector<Particle*> &p1) {
    assert(p0.size() == p1.size());

    bool errorAcceptable = true;

    #pragma omp parallel for
    for (int i = 0; i < p0.size(); i++) {
        if ((p0[i]->position - p1[i]->position).magnitude() > errorTolerance) {
            #pragma omp critical
            {
                errorAcceptable = false;
            }
        }
    }

    return errorAcceptable;
}

void Simulation::run() {
    const int numParticles = y0.size();
    k0.reserve(numParticles);
    k1.reserve(numParticles);
    y1.resize(numParticles);
    z1.resize(numParticles);

    float h = initTimeStep;

    k0.clear();
    computeForces(k0, y0); // Compute k0

    while (true) {
        k1.clear();
        reset(y1);
        reset(z1);

        computeApproxPositions(h); // Compute y1
        computeForces(k1, y1); // Compute k1
        computeBetterPositions(h); // Compute z1

        if (isErrorAcceptable(z1, y1)) {
            // Error is acceptable so we can stop simulation
            break;
        }

        h /= 2.0;
    }
}

// ifndef USE_OPENCL
#endif
