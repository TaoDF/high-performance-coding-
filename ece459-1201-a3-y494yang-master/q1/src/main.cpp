#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>

#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <base64/base64.h>

using std::cout;
using std::cerr;
using std::endl;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

int gMaxSecretLen = 4;

std::string gAlphabet = "abcdefghijklmnopqrstuvwxyz"
                        "0123456789";

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

void usage(const char *cmd) {
    cout << cmd << " <token> [maxLen] [alphabet]" << endl;
    cout << endl;

    cout << "Defaults:" << endl;
    cout << "maxLen = " << gMaxSecretLen << endl;
    cout << "alphabet = " << gAlphabet << endl;
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    std::stringstream jwt;
    jwt << argv[1];

    if (argc > 2) {
        gMaxSecretLen = atoi(argv[2]);
    }
    if (argc > 3) {
        gAlphabet = argv[3];
    }

    std::string header64;
    getline(jwt, header64, '.');

    std::string payload64;
    getline(jwt, payload64, '.');

    std::string origSig64;
    getline(jwt, origSig64, '.');

    // Our goal is to find the secret to HMAC this string into our origSig
    std::string message = header64 + '.' + payload64;
    std::string origSig = base64_decode(origSig64);

    // Use OpenCL to brute force JWT
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
        std::ifstream sourceFile("src/jwtcracker.cl");
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
 
        // Make kernels
        cl::Kernel kernel(program, "bruteForceJWT");
 
        // Create buffers
        //------1
        char *alphaChar = new char[gAlphabet.size()];
        memcpy(alphaChar, gAlphabet.c_str(), gAlphabet.size());
        cl::Buffer bufferAlphabet = cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            gAlphabet.size()
        );
 
        //----2
        cl_int *alphaLen = new cl_int();
        *alphaLen = gAlphabet.size();
        cl::Buffer bufferAlphaLen= cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            sizeof(cl_int)
        );

        //------3
        cl_int *pswLen = new cl_int();
        *pswLen = gMaxSecretLen;
        cl::Buffer bufferPswLen= cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            sizeof(cl_int)
        );

        //--------4
        char *messageChar = new char[message.size()];
        memcpy(messageChar, message.c_str(), message.size());
        cl::Buffer bufferMessage = cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            message.size()
        );

        //-------5
        cl_int *messageLen = new cl_int();
        *messageLen = message.size();
        cl::Buffer bufferMsgLen= cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            sizeof(cl_int)
        );

        //--------6
        char *origSigChar = new char[origSig.size()];
        memcpy(origSigChar, origSig.c_str(), origSig.size());
        cl::Buffer bufferSig = cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            origSig.size()
        );

        //-------7
        cl_int *sigLen = new cl_int();
        *sigLen = origSig.size();
        cl::Buffer bufferSigLen= cl::Buffer(
            context,
            CL_MEM_READ_ONLY,
            sizeof(cl_int)
        );

        //-------8
        cl_int *stopK = new cl_int();
        *stopK = 0;
        cl::Buffer bufferStop= cl::Buffer(
            context,
            CL_MEM_READ_WRITE,
            sizeof(cl_int)
        );        


        // Write buffers
        //----1
        queue.enqueueWriteBuffer(
            bufferAlphabet,
            CL_TRUE,
            0,
            gAlphabet.size(),
            alphaChar
        );

        //------2
        queue.enqueueWriteBuffer(
            bufferAlphaLen,
            CL_TRUE,
            0,
            sizeof(cl_int),
            alphaLen
        );

        //------3
        queue.enqueueWriteBuffer(
            bufferPswLen,
            CL_TRUE,
            0,
            sizeof(cl_int),
            pswLen
        );

        //----4
        queue.enqueueWriteBuffer(
            bufferMessage,
            CL_TRUE,
            0,
            message.size(),
            messageChar
        );

        //----5
        queue.enqueueWriteBuffer(
            bufferMsgLen,
            CL_TRUE,
            0,
            sizeof(cl_int),
            messageLen
        );

        //-----6
        queue.enqueueWriteBuffer(
            bufferSig,
            CL_TRUE,
            0,
            sizeof(origSig),
            origSigChar
        );        

        //-----7
        queue.enqueueWriteBuffer(
            bufferSigLen,
            CL_TRUE,
            0,
            sizeof(cl_int),
            sigLen
        );               

        //-----8
        queue.enqueueWriteBuffer(
            bufferStop,
            CL_TRUE,
            0,
            sizeof(cl_int),
            stopK
        );       


        // Set arguments to kernel

        kernel.setArg(0, bufferAlphabet);
        kernel.setArg(1, bufferAlphaLen);
        kernel.setArg(2, bufferPswLen);
        kernel.setArg(3, bufferMessage);
        kernel.setArg(4, bufferMsgLen);
        kernel.setArg(5, bufferSig);
        kernel.setArg(6, bufferSigLen);
        kernel.setArg(7, bufferStop);

        // Run the kernel on specific ND range
        unsigned long range = 1; 
        for (int i = 0; i < gMaxSecretLen; i++)
        {
            range = range*(gAlphabet.size()+1);
        }
        cl::NDRange globalSize(range);
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, globalSize); 
 
        // Read buffer(s)
    } catch(cl::Error error) {
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
    }

    return 0;
}
