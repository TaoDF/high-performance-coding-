#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>

#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>

using std::cout;
using std::cerr;
using std::endl;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

const int RANDOM_SEED = 1138;


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

void print_results( float a_pct, float b_pct, float t_pct) {
    cout << "Candidate A wins " << a_pct << "% of the time." << endl;
    cout << "Candidate B wins " << b_pct << "% of the time." << endl;
    cout << "Ties " << t_pct << "% of the time." << endl;
}


int main(int argc, char const *argv[]) {
    cout << std::setprecision(4);
    int num_lines = 100000;
    // Use OpenCL to run simulation
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
        std::ifstream sourceFile("q6/election.cl");
            if(!sourceFile.is_open()){
                std::cerr << "Cannot find kernel file" << std::endl;
                throw;
            }
        std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
        cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length() + 1));
 
        // Make program of the source code in the context
        cl::Program program = cl::Program(context, source);
 
        // Build program for these specific devices
        try {
            program.build(devices);
        } catch(cl::Error error) {
            std::cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
            throw;
        }

        //read file
        cl_float3 voter[num_lines];

        int rows = num_lines;
        int cols = 3;
        std::ifstream file("/home/y494yang/ECE459-W20-Final-Code/q6/voters.csv");
        if (file.is_open()) {
            for (int i = 0; i < rows; ++i) {
                for (int j = 0; j < cols; ++j) {
                    file >> voter[i].s[j];
                  //  std::cout<<voter[i].s[j]<<std::endl;
                    if(j<2)
                    {
                        file.get();
                    }
                }
            }
        }
        else
        {
            std::cout<<"cannot open file"<<std::endl;
        }

        cl_int *seed = new cl_int();
        *seed = RANDOM_SEED;
 
        // Make kernel(s)
        cl::Kernel kernel(program, "elect");
 
        // Create buffers
        cl::Buffer Buffer_voter = cl::Buffer(context, CL_MEM_READ_ONLY, num_lines * sizeof(cl_float3));
        cl::Buffer Buffer_rtn = cl::Buffer(context, CL_MEM_READ_WRITE, num_lines * sizeof(cl_float3));
        cl::Buffer rand_seed= cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_int));
        // Write buffers
        queue.enqueueWriteBuffer(Buffer_voter, CL_TRUE, 0, num_lines * sizeof(cl_float3), voter);
        queue.enqueueWriteBuffer(rand_seed, CL_TRUE, 0, sizeof(cl_int), seed);

        // Set arguments to kernel
        kernel.setArg(0, Buffer_voter);
        kernel.setArg(1, Buffer_rtn);
        kernel.setArg(2, rand_seed);

        // Run the kernel on specific ND range
        cl::NDRange globalSize(num_lines);
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, globalSize);
        
        // Read buffer(s)
        cl_float3* rtn = new cl_float3[num_lines];
        queue.enqueueReadBuffer(Buffer_rtn, CL_TRUE, 0, num_lines*sizeof(cl_float3), rtn);

        float a_wins_num = 0;
        float b_wins_num = 0;
        
        float a_wins=0;
        float b_wins=0;
        float ties=0;

        for(int i = 0; i<num_lines; i++)
        {
            a_wins_num = rtn[i].s[0];
            b_wins_num = rtn[i].s[1];
            if(a_wins_num>b_wins_num)
            {
                a_wins+=1;
            }
            else if(a_wins_num<b_wins_num)
            {
                b_wins+=1;
            }
            else
            {
                ties+=1;
            }
        }
        
        // Placeholder floats; replace them if you wish.
        float a = a_wins/num_lines*100;
        float b = b_wins/num_lines*100;
        float tie = ties/num_lines*100;
        // Print results using print_results function
        print_results(a, b, tie);
        
    } catch(cl::Error error) {
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
    }

    return 0;
}
