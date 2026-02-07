#include <popl/popl.hpp>
#include <CL/opencl.hpp>
#include <common.hpp>

#include <fstream>
#include <string>

static std::string readStringFromFile(
    const std::string& filename )
{
    std::ifstream is(filename, std::ios::binary);
    if (!is.good()) {
        printf("[ERRO] Couldn't open file '%s'!\n", filename.c_str());
        return "";
    }

    size_t filesize = 0;
    is.seekg(0, std::ios::end);
    filesize = (size_t)is.tellg();
    is.seekg(0, std::ios::beg);

    std::string source{
        std::istreambuf_iterator<char>(is),
        std::istreambuf_iterator<char>() };

    return source;
}

int main(
    int argc,
    char** argv )
{
    int platformIndex = 0;
    int deviceIndex = 0;
    int iters = 1;
    int workDim = 3; // work dimenstion.

    std::string fileName("../apps/cdm_workgroup_issue/cdm_workgroup_issue.cl");
    std::string kernelName("cdm_workgroup_issue");
    std::string buildOptions;
    std::string compileOptions;
    std::string linkOptions;
    bool compile = false;
    size_t gwx = 512;
    size_t lwx = 32;

    {
        bool advanced = false;

        popl::OptionParser op("Supported Options");
        op.add<popl::Value<int>>("p", "platform", "Platform Index", platformIndex, &platformIndex);
        op.add<popl::Value<int>>("d", "device", "Device Index", deviceIndex, &deviceIndex);
        op.add<popl::Value<int>>("i", "iteration", "iters", iters, &iters);
        op.add<popl::Value<int>>("", "workDim", "work dimension", workDim, &workDim);
        op.add<popl::Value<std::string>>("", "file", "Kernel File Name", fileName, &fileName);
        op.add<popl::Value<std::string>>("", "name", "Kernel Name", kernelName, &kernelName);
        op.add<popl::Value<std::string>>("", "options", "Program Build Options", buildOptions, &buildOptions);
        op.add<popl::Value<size_t>>("", "gwx", "Global Work Size", gwx, &gwx);
        op.add<popl::Value<size_t>>("", "lwx", "Local Work Size (0 -> NULL)", lwx, &lwx);
        op.add<popl::Switch>("a", "advanced", "Show advanced options", &advanced);
        op.add<popl::Switch, popl::Attribute::advanced>("c", "compilelink", "Use clCompileProgram and clLinkProgram", &compile);
        op.add<popl::Value<std::string>, popl::Attribute::advanced>("", "compileoptions", "Program Compile Options", compileOptions, &compileOptions);
        op.add<popl::Value<std::string>, popl::Attribute::advanced>("", "linkoptions", "Program Link Options", linkOptions, &linkOptions);
        bool printUsage = false;
        try {
            op.parse(argc, argv);
        } catch (std::exception& e) {
            fprintf(stderr, "Error: %s\n\n", e.what());
            printUsage = true;
        }

        if (printUsage || !op.unknown_options().empty() || !op.non_option_args().empty()) {
            fprintf(stderr,
                "Usage: ndrangekernelfromfile [options]\n"
                "%s", op.help(advanced ? popl::Attribute::advanced : popl::Attribute::optional).c_str());
            return -1;
        }
    }

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    printf("[INFO] Running on platform: %s\n", platforms[platformIndex].getInfo<CL_PLATFORM_NAME>().c_str() );

    std::vector<cl::Device> devices;
    platforms[platformIndex].getDevices(CL_DEVICE_TYPE_ALL, &devices);
    printf("[INFO] Running on device: %s\n", devices[deviceIndex].getInfo<CL_DEVICE_NAME>().c_str() );

    device_info_t deviceInfo = getDeviceInfo(devices[deviceIndex]);
    uint64_t globalWorkItems = deviceInfo.numCUs * deviceInfo.computeWgsPerCU * deviceInfo.maxWGSize;
    uint64_t t               = std::min((globalWorkItems *  sizeof(cl_float)), deviceInfo.maxAllocSize) / sizeof(cl_float);

    printf("[DBG] dev.numCUs:%d\n", deviceInfo.numCUs);
    printf("[DBG] dev.computeWgsPerCU:%d\n", deviceInfo.computeWgsPerCU);
    printf("[DBG] dev.maxWgSize:%d\n", deviceInfo.maxWGSize);
    printf("[DBG] dev.maxAllocSize:%ld\n", deviceInfo.maxAllocSize);
    //cl::NDRange globalWorkSize1d = roundToMultipleOf(t, deviceInfo.maxWGSize);
    cl::NDRange* globalWorkSize1d;
    globalWorkSize1d = new cl::NDRange(1024);
    //globalWorkSize1d = new cl::NDRange(8192);
    cl::NDRange* globalWorkSize2d;
    globalWorkSize2d = new cl::NDRange(32, 32);
    //globalWorkSize2d = new cl::NDRange(128, 64);
    cl::NDRange* globalWorkSize3d;
    globalWorkSize3d = new cl::NDRange(16, 16, 4);
    //globalWorkSize3d = new cl::NDRange(32, 32, 8);

    cl::NDRange* globalWorkInfo;
    switch(workDim)
    {
        case 1: globalWorkInfo = globalWorkSize1d;
                printf("[DBG] globalWorkSize1d:dim:%ld, size:%ld, size:%ld, size:%ld\n", 
                globalWorkSize1d->dimensions(), globalWorkSize1d->get()[0], globalWorkSize1d->get()[1], globalWorkSize1d->get()[2]);
            break;
        case 2: globalWorkInfo = globalWorkSize2d;
                printf("[DBG] globalWorkSize2d:dim:%ld, size:%ld, size:%ld, size:%ld\n",
                globalWorkSize2d->dimensions(), globalWorkSize2d->get()[0], globalWorkSize2d->get()[1], globalWorkSize2d->get()[2]);
            break;
        case 3: globalWorkInfo = globalWorkSize3d;
                printf("[DBG] globalWorkSize3d:dim:%ld, size:%ld, size:%ld, size:%ld\n",
                globalWorkSize3d->dimensions(), globalWorkSize3d->get()[0], globalWorkSize3d->get()[1], globalWorkSize3d->get()[2]);
            break;
        default:
            printf("[ERRO] invalid Global Work Dimension %ld!\n", globalWorkInfo->dimensions());
            return 1;
    }

    cl::NDRange* localWorkSize1d;
    localWorkSize1d  = new cl::NDRange(1024);
    cl::NDRange* localWorkSize2d;
    localWorkSize2d  = new cl::NDRange(32, 32);
    cl::NDRange* localWorkSize3d;
    localWorkSize3d  = new cl::NDRange(16, 16, 4);
    cl::NDRange* localWorkInfo;
    switch(workDim)
    {
        case 1: localWorkInfo = localWorkSize1d;
        printf("[DBG] localWorkInfo:dim:%ld, size:%ld, size:%ld, size:%ld\n", localWorkInfo->dimensions(), 
            localWorkInfo->get()[0], localWorkInfo->get()[1], localWorkInfo->get()[2]);
        break;
        case 2: localWorkInfo = localWorkSize2d;
        printf("[DBG] localWorkInfo:dim:%ld, size:%ld, size:%ld, size:%ld\n", localWorkInfo->dimensions(), 
            localWorkInfo->get()[0], localWorkInfo->get()[1], localWorkInfo->get()[2]);
        break;
        case 3: localWorkInfo = localWorkSize3d;
        printf("[DBG] localWorkInfo:dim:%ld, size:%ld, size:%ld, size:%ld\n", localWorkInfo->dimensions(), 
            localWorkInfo->get()[0], localWorkInfo->get()[1], localWorkInfo->get()[2]);
        break;
        default:
            printf("[ERRO] invalid local Work Dimension %ld!\n", localWorkInfo->dimensions());
            return 1;
    }


    cl::Context context{devices[deviceIndex]};
    cl::CommandQueue commandQueue = cl::CommandQueue{context, devices[deviceIndex]};

    printf("[INFO] Reading program source from file: %s\n", fileName.c_str() );
    std::string kernelString = readStringFromFile(fileName.c_str());

    cl::Program program;
    if (compile) {
        printf("[INFO] Compiling program with compile options: %s\n",
            compileOptions.empty() ? "(none)" : compileOptions.c_str() );
        cl::Program object{ context, kernelString };
        object.compile(compileOptions.c_str());
        for( auto& device : program.getInfo<CL_PROGRAM_DEVICES>() )
        {
            printf("[INFO] Program compile log for device %s:\n",
                device.getInfo<CL_DEVICE_NAME>().c_str() );
            printf("[INFO] %s\n",
                object.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device).c_str() );
        }

        printf("[INFO] Linking program with link options: %s\n",
            linkOptions.empty() ? "(none)" : linkOptions.c_str() );
        program = cl::linkProgram({object}, linkOptions.c_str());
        for( auto& device : program.getInfo<CL_PROGRAM_DEVICES>() )
        {
            printf("[INFO] Program link log for device %s:\n",
                device.getInfo<CL_DEVICE_NAME>().c_str() );
            printf("[INFO] %s\n",
                program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device).c_str() );
        }
    } else {
        printf("[INFO] Building program with build options: %s\n",
            buildOptions.empty() ? "(none)" : buildOptions.c_str() );
        program = cl::Program{ context, kernelString };
        program.build(buildOptions.c_str());
        for( auto& device : program.getInfo<CL_PROGRAM_DEVICES>() )
        {
            printf("[INFO] Program build log for device %s:\n",
                device.getInfo<CL_DEVICE_NAME>().c_str() );
            printf("[INFO] %s\n",
                program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device).c_str() );
        }
    }
    printf("[INFO] Creating kernel: %s\n", kernelName.c_str() );
    cl::Kernel kernel = cl::Kernel{ program, kernelName.c_str() };

    //cl::Buffer deviceMemDst = cl::Buffer{ context, CL_MEM_ALLOC_HOST_PTR, gwx * sizeof( cl_uint ) };
    cl::Buffer deviceMemDst = cl::Buffer{ context, CL_MEM_ALLOC_HOST_PTR, globalWorkItems * sizeof( cl_float) };

    // execution
    cl_float A = 1.3f;
    kernel.setArg(0, deviceMemDst), kernel.setArg(1, A);

    for (uint i = 0; i < iters; i++)
    {
        commandQueue.enqueueNDRangeKernel(
            kernel, // kernel
            cl::NullRange, // global work offset
            *globalWorkInfo,  // global work size
            *localWorkInfo);  // local work size
            //cl::NDRange{gwx}, 
            //(lwx == 0) ? cl::NullRange : cl::NDRange{lwx});

        // // verify results by printing the first few values
        // if (gwx > 3) {
        //     auto ptr = (const cl_uint*)commandQueue.enqueueMapBuffer(
        //         deviceMemDst,
        //         CL_TRUE,
        //         CL_MAP_READ,
        //         0,
        //         gwx * sizeof( cl_uint ) );

        //     printf("[INFO] First few values: [0] = %u, [1] = %u, [2] = %u\n", ptr[0], ptr[1], ptr[2]);

        //     commandQueue.enqueueUnmapMemObject(
        //         deviceMemDst,
        //         (void*)ptr );
        // }

        commandQueue.finish();
    }

    printf("[INFO] Done.\n");

    return 0;
}
