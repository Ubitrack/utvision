//wglGetCurrentContext, wglGetCurrentDC

#include "OpenCLManager.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>

//OCL
#ifdef HAVE_OPENCL
#include <opencv2/core/ocl.hpp>

#ifdef __APPLE__
    #include <OpenGL/OpenGL.h>
    #include "OpenCL/cl_gl.h"
#else
    #include "GL/gl.h"
    #include "CL/cl_gl.h"
    #ifdef WIN32
// DirectX support is disabled for now
// specifically, because it requires NVIDIA OpenCL
//        #include <CL/cl_d3d11_ext.h>
    #else
        #include "GL/glx.h"
    #endif
#endif
#endif // HAVE_OPENCL

#include <log4cpp/Category.hh>

// get loggers
static log4cpp::Category& logger(log4cpp::Category::getInstance("Ubitrack.Vision.OpenCLManager"));

#if defined (__APPLE__) || defined(MACOSX)
static const char* CL_GL_SHARING_EXT = "cl_APPLE_gl_sharing";
#else
static const char* CL_GL_SHARING_EXT = "cl_khr_gl_sharing";
#endif

namespace Ubitrack {
namespace Vision {

static boost::scoped_ptr<OpenCLManager> g_pOpenCLManager;

const char *getOpenCLErrorString(cl_int error)
{
switch(error){
    // run-time and JIT compiler errors
    case 0: return "CL_SUCCESS";
    case -1: return "CL_DEVICE_NOT_FOUND";
    case -2: return "CL_DEVICE_NOT_AVAILABLE";
    case -3: return "CL_COMPILER_NOT_AVAILABLE";
    case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case -5: return "CL_OUT_OF_RESOURCES";
    case -6: return "CL_OUT_OF_HOST_MEMORY";
    case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case -8: return "CL_MEM_COPY_OVERLAP";
    case -9: return "CL_IMAGE_FORMAT_MISMATCH";
    case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case -11: return "CL_BUILD_PROGRAM_FAILURE";
    case -12: return "CL_MAP_FAILURE";
    case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case -15: return "CL_COMPILE_PROGRAM_FAILURE";
    case -16: return "CL_LINKER_NOT_AVAILABLE";
    case -17: return "CL_LINK_PROGRAM_FAILURE";
    case -18: return "CL_DEVICE_PARTITION_FAILED";
    case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

    // compile-time errors
    case -30: return "CL_INVALID_VALUE";
    case -31: return "CL_INVALID_DEVICE_TYPE";
    case -32: return "CL_INVALID_PLATFORM";
    case -33: return "CL_INVALID_DEVICE";
    case -34: return "CL_INVALID_CONTEXT";
    case -35: return "CL_INVALID_QUEUE_PROPERTIES";
    case -36: return "CL_INVALID_COMMAND_QUEUE";
    case -37: return "CL_INVALID_HOST_PTR";
    case -38: return "CL_INVALID_MEM_OBJECT";
    case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case -40: return "CL_INVALID_IMAGE_SIZE";
    case -41: return "CL_INVALID_SAMPLER";
    case -42: return "CL_INVALID_BINARY";
    case -43: return "CL_INVALID_BUILD_OPTIONS";
    case -44: return "CL_INVALID_PROGRAM";
    case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case -46: return "CL_INVALID_KERNEL_NAME";
    case -47: return "CL_INVALID_KERNEL_DEFINITION";
    case -48: return "CL_INVALID_KERNEL";
    case -49: return "CL_INVALID_ARG_INDEX";
    case -50: return "CL_INVALID_ARG_VALUE";
    case -51: return "CL_INVALID_ARG_SIZE";
    case -52: return "CL_INVALID_KERNEL_ARGS";
    case -53: return "CL_INVALID_WORK_DIMENSION";
    case -54: return "CL_INVALID_WORK_GROUP_SIZE";
    case -55: return "CL_INVALID_WORK_ITEM_SIZE";
    case -56: return "CL_INVALID_GLOBAL_OFFSET";
    case -57: return "CL_INVALID_EVENT_WAIT_LIST";
    case -58: return "CL_INVALID_EVENT";
    case -59: return "CL_INVALID_OPERATION";
    case -60: return "CL_INVALID_GL_OBJECT";
    case -61: return "CL_INVALID_BUFFER_SIZE";
    case -62: return "CL_INVALID_MIP_LEVEL";
    case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
    case -64: return "CL_INVALID_PROPERTY";
    case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
    case -66: return "CL_INVALID_COMPILER_OPTIONS";
    case -67: return "CL_INVALID_LINKER_OPTIONS";
    case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

    // extension errors
    case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
    case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
    case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
    case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
    case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
    default: return "Unknown OpenCL error";
    }
}



OpenCLManager& OpenCLManager::singleton()
{
    // @todo we should try to get rid of this lock for the default case, that is - everything initialized !!!
	static boost::mutex singletonMutex;
	boost::mutex::scoped_lock l(singletonMutex);

    //create a new singleton if necessary
    if (!g_pOpenCLManager) {
        LOG4CPP_INFO(logger, "Create Instance of OpenCLManager");
        g_pOpenCLManager.reset(new OpenCLManager);
    }

    return *g_pOpenCLManager;
}

void OpenCLManager::destroyOpenCLManager()
{
    g_pOpenCLManager.reset(0);
}

void notifyOpenCLState(const char* errinfo, const void* private_info, size_t cb, void* user_data)
{
    // @todo OCLManager should log should log errinfo
    LOG4CPP_INFO(logger, "OpenCL Error: " << errinfo);
}

bool OpenCLManager::isInitialized() const
{
    return m_isInitialized;
}

bool OpenCLManager::isEnabled() const
{
#ifdef HAVE_OPENCL
    return true;
#else
    return false;
#endif
}

OpenCLManager::OpenCLManager()
        :  m_isInitialized(false)
		, m_isActive(false)
{

}

OpenCLManager::~OpenCLManager(void)
{

}

void OpenCLManager::activate() {
	m_isActive = true;
}

void OpenCLManager::deactivate() {
	m_isActive = false;
}

bool OpenCLManager::isActive() {
	return m_isActive;
}

void OpenCLManager::registerInitCallback(InitCallbackType cb) {
    m_initCallbacks.push_back(cb);
}

void OpenCLManager::notifyInitComplete() {
    for (int i=0; i < m_initCallbacks.size(); ++i) {
        m_initCallbacks.at(i)();
    }
    // clear the callback list
    m_initCallbacks.clear();
}

// DirectX support is disabled for now
// specifically, because it requires NVIDIA OpenCL
// #ifdef HAVE_OPENCL
// #ifdef WIN32
// void OpenCLManager::initializeDirectX(ID3D11Device* pD3D11Device)
// {

//     if(m_isInitialized){
//         return;
//     }
//     //Get all Platforms and select a GPU one
//     cl_uint numPlatforms;
//     clGetPlatformIDs (65536, NULL, &numPlatforms);
//     LOG4CPP_INFO( logger, "DX: Platforms detected: " << numPlatforms );


//     std::vector<cl_platform_id> platforms(numPlatforms);

//     cl_int err = clGetPlatformIDs(numPlatforms, platforms.data(), NULL);
//     if(err != CL_SUCCESS)
//     {
//         LOG4CPP_INFO( logger, "DX: error at clGetPlatformIDs :" << getOpenCLErrorString(err) );
//         m_isInitialized = false;
//     }

//     int found = -1;
//     cl_device_id device = NULL;
//     cl_uint numDevices = 0;
//     cl_context context = NULL;

//     clGetDeviceIDsFromD3D11NV_fn clGetDeviceIDsFromD3D11NV = (clGetDeviceIDsFromD3D11NV_fn)
//                 clGetExtensionFunctionAddress("clGetDeviceIDsFromD3D11NV");
//      for (int i = 0; i < (int)numPlatforms; i++)
//     {
//         if (!clGetDeviceIDsFromD3D11NV)
//             break;

//         device = NULL;
//         numDevices = 0;
//         err = clGetDeviceIDsFromD3D11NV(platforms[i], CL_D3D11_DEVICE_NV, pD3D11Device,
//                 CL_PREFERRED_DEVICES_FOR_D3D11_NV, 1, &device, &numDevices);
//         if (err != CL_SUCCESS)
//             continue;

//         if (numDevices > 0)
//         {
//             cl_context_properties properties[] = {
//                     CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[i],
//                     CL_CONTEXT_D3D11_DEVICE_NV, (cl_context_properties)(pD3D11Device),
//                     NULL, NULL
//             };
//             m_clContext = clCreateContext(properties, 1, &device, NULL, NULL, &err);
//             if(!m_clContext || err!= CL_SUCCESS)
//             {
//                 LOG4CPP_INFO( logger,  "error at clCreateContext :" << getOpenCLErrorString(err) );
//             }
//             else
//             {
//                 found = i;
//                 break;
//             }
//         }
//     }
//      if (found < 0)
//     {
//         LOG4CPP_ERROR(logger, "OpenCL: no preferred D3D11_NV device found. Trying"
//             << "now all devices context for DirectX interop");
//         // try with CL_ALL_DEVICES_FOR_D3D11_KHR
//         for (int i = 0; i < (int)numPlatforms; i++)
//         {
//             if (!clGetDeviceIDsFromD3D11NV)
//                 continue;

//             device = NULL;
//             numDevices = 0;
//             err = clGetDeviceIDsFromD3D11NV(platforms[i], CL_D3D11_DEVICE_NV, pD3D11Device,
//                     CL_ALL_DEVICES_FOR_D3D11_NV, 1, &device, &numDevices);
//             if (err != CL_SUCCESS)
//                 continue;

//             if (numDevices > 0)
//             {
//                 cl_context_properties properties[] = {
//                         CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[i],
//                         CL_CONTEXT_D3D11_DEVICE_NV, (cl_context_properties)(pD3D11Device),
//                         NULL, NULL
//                 };

//                 m_clContext = clCreateContext(properties, 1, &device, NULL, NULL, &err);
//                    if(!m_clContext || err != CL_SUCCESS)
//                 {
//                     LOG4CPP_INFO( logger,  "error at clCreateContext :" << getOpenCLErrorString(err) );
//                 }
//                 else
//                 {
//                     found = i;
//                     break;
//                 }
//             }
//         }
//         if (found < 0){
//             LOG4CPP_ERROR(logger, "OpenCL: Can't create context for DirectX interop");
//             m_isInitialized = false;
//             return;
//         }
//     }


//     cl_device_id selectedDeviceID;
//     //Select a GPU device
//     err = clGetDeviceIDs(platforms[found], CL_DEVICE_TYPE_GPU, 1, &selectedDeviceID, NULL);
//     if(err != CL_SUCCESS)
//     {
//         LOG4CPP_INFO( logger, "DX: error at clGetDeviceIDs :" << getOpenCLErrorString(err) );
//         return;
//     }

//     char cDeviceNameBuffer[1024];
//     clGetDeviceInfo (selectedDeviceID, CL_DEVICE_NAME, sizeof(char) *  1024, cDeviceNameBuffer, NULL);
//     LOG4CPP_INFO( logger, ": DX: Device Name: "		<< cDeviceNameBuffer );


// 	char cPlatformNameBuffer[1024];
// 	clGetPlatformInfo(platforms[found], CL_PLATFORM_NAME, sizeof(char) * 1024, cPlatformNameBuffer, NULL);
// 	std::string platform_name(cPlatformNameBuffer);


//     m_clCommandQueue = clCreateCommandQueue(m_clContext, selectedDeviceID, 0, &err);
//     if(!m_clCommandQueue || err!= CL_SUCCESS)
//     {
//         LOG4CPP_INFO( logger, "DX: error at clCreateCommandQueue :" << getOpenCLErrorString(err) );
//         return;
//     }


// 	cv::ocl::attachContext(platform_name, platforms[found], m_clContext, selectedDeviceID);
// 	if (cv::ocl::useOpenCL()) {
// 		LOG4CPP_INFO(logger, "OpenCV+OpenCL works OK!");
// 	}
// 	else {
// 		LOG4CPP_INFO(logger, "Can't init OpenCV with OpenCL TAPI");
// 	}

//     m_isInitialized = true;
// 	notifyInitComplete();
//     LOG4CPP_INFO( logger, "initialized OpenCL: " << isInitialized());
// }
// #endif // WIN32
// #endif

void OpenCLManager::initializeOpenGL()
{
    // prevent initialize to be executed multiple times
    boost::mutex::scoped_lock lock(m_mutex);

    if (m_isInitialized) {
        return;
    }

#ifdef HAVE_OPENCL

    LOG4CPP_INFO(logger, "OpenCLManager begin Initialization for OpenGL Context Sharing");

    cl_int err;
    size_t returned_size;

    //Get all Platforms and select a GPU one
    cl_uint numPlatforms;
    clGetPlatformIDs(65536, NULL, &numPlatforms);
    LOG4CPP_INFO(logger, "Platforms detected: " << numPlatforms);

    cl_platform_id* platformIDs;
    platformIDs = new cl_platform_id[numPlatforms];

    err = clGetPlatformIDs(numPlatforms, platformIDs, NULL);
    if (err!=CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "error at clGetPlatformIDs :" << getOpenCLErrorString(err));
        return;
    }

    // @todo is there a reason why we should enable selecting a different platform ?
    cl_uint selectedPlatform = 0;
    cl_platform_id selectedPlatformID = platformIDs[selectedPlatform];
    delete[] platformIDs;

    char cPlatformNameBuffer[1024];
    err = clGetPlatformInfo(selectedPlatformID, CL_PLATFORM_NAME,  sizeof(char)*1024, cPlatformNameBuffer, NULL);
    if(err!=CL_SUCCESS)
    {
        LOG4CPP_ERROR(logger, "Error: Failed to retrieve platform name! " << getOpenCLErrorString(err));
        return;
    }

    std::string platform_name(cPlatformNameBuffer);


#ifdef WIN32
    // Create CL context properties, add WGL context & handle to DC
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(), // WGL Context
        CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(), // WGL HDC
        CL_CONTEXT_PLATFORM, (cl_context_properties)selectedPlatformID, // OpenCL platform
        0
    };

    // Find CL capable devices in the current GL context
    //cl_device_id devices[32];
	//size_t size;
    //clGetGLContextInfoKHR(properties, CL_DEVICES_FOR_GL_CONTEXT_KHR, 32 * sizeof(cl_device_id), devices, &size);
    // Create a context using the supported devices
    //int count = size / sizeof(cl_device_id);
    m_clContext = clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
    //m_clContext = clCreateContext(properties, count, devices, NULL, 0, &err);

#elif __APPLE__

    LOG4CPP_DEBUG(logger, "Acquire OpenGL Context for OpenCL Sharing");
    // Get current CGL Context and CGL Share group
    CGLContextObj kCGLContext = CGLGetCurrentContext();
    CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);

    cl_context_properties properties[] = {
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
            (cl_context_properties)kCGLShareGroup,
            0
    };

    // Create a context from a CGL share group
    //
    m_clContext = clCreateContext(properties, 0, 0, notifyOpenCLState, 0, &err);

#else

    // Create CL context properties, add GLX context & handle to DC
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(), // GLX Context
        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(), // GLX Display
        CL_CONTEXT_PLATFORM, (cl_context_properties)selectedPlatformID, // OpenCL platform
        0
    };

    // Find CL capable devices in the current GL context
    cl_device_id devices[32];
    size_t size;
    clGetGLContextInfoKHR(properties, CL_DEVICES_FOR_GL_CONTEXT_KHR, 32 * sizeof(cl_device_id), devices, &size);
    // Create a context using the supported devices
    int count = size / sizeof(cl_device_id);
    //m_clContext = clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
    m_clContext = clCreateContext(properties, count, devices, NULL, 0, &err);

#endif

    if (err != CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "error at clCreateContext :" << getOpenCLErrorString(err));
        return;
    }

    unsigned int device_count;
    cl_device_id device_ids[16];

    err = clGetContextInfo(m_clContext, CL_CONTEXT_DEVICES, sizeof(device_ids), device_ids, &returned_size);
    if(err!=CL_SUCCESS)
    {
        LOG4CPP_ERROR(logger, "Error: Failed to retrieve compute devices for context! " << getOpenCLErrorString(err));
        return;
    }

    device_count = returned_size / sizeof(cl_device_id);

    cl_device_id selectedDeviceID = 0;
    int i = 0;
    int device_found = 0;
    cl_device_type device_type;
    for(i = 0; i < device_count; i++)
    {
        clGetDeviceInfo(device_ids[i], CL_DEVICE_TYPE, sizeof(cl_device_type), &device_type, NULL);
        if(device_type == CL_DEVICE_TYPE_GPU)
        {
            selectedDeviceID = device_ids[i];
            device_found = 1;
            break;
        }
    }

    if(!device_found)
    {
        LOG4CPP_ERROR(logger, "Error: Failed to locate compute device!");
        return;
    }

    // log info about the selected device
    char cDeviceNameBuffer[1024];
    char cDeviceVendorBuffer[1024];
    cl_uint device_max_compute_units = 0;
    cl_uint device_max_frequency = 0;
    cl_uint device_vendor_id = 0;

    err = clGetDeviceInfo(selectedDeviceID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &device_max_compute_units, NULL);
    if (err!=CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "Error glDeviceInfo CL_DEVICE_MAX_COMPUTE_UNITS:" << getOpenCLErrorString(err));
    }

    err = clGetDeviceInfo(selectedDeviceID, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &device_max_frequency, NULL);
    if (err!=CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "Error glDeviceInfo CL_DEVICE_MAX_CLOCK_FREQUENCY:" << getOpenCLErrorString(err));
    }

    err = clGetDeviceInfo(selectedDeviceID, CL_DEVICE_VENDOR_ID, sizeof(cl_uint), &device_vendor_id, NULL);
    if (err!=CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "Error glDeviceInfo CL_DEVICE_VENDOR_ID:" << getOpenCLErrorString(err));
    }

    err = clGetDeviceInfo(selectedDeviceID, CL_DEVICE_VENDOR, sizeof(char)*1024, cDeviceVendorBuffer, NULL);
    if (err!=CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "Error glDeviceInfo CL_DEVICE_VENDOR:" << getOpenCLErrorString(err));
    }

    clGetDeviceInfo(selectedDeviceID, CL_DEVICE_NAME, sizeof(char)*1024, cDeviceNameBuffer, NULL);
    LOG4CPP_INFO(logger, "Selected OpenCL Device: " << cDeviceNameBuffer
            << " vendor " << cDeviceVendorBuffer << " vendor-id " << device_vendor_id
            << " compute_units " << device_max_compute_units << " max_frequency " << device_max_frequency);


    cl_int status = 0;
    m_clCommandQueue = clCreateCommandQueue(m_clContext, selectedDeviceID, 0, &err);
    if (!m_clCommandQueue || err!=CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "Error creating OCL CommandQueue: " << getOpenCLErrorString(err));
        return;
    }


    cv::ocl::attachContext(platform_name, selectedPlatformID, m_clContext, selectedDeviceID);
    if( cv::ocl::useOpenCL() ) {
        LOG4CPP_INFO(logger, "OpenCV+OpenCL works OK!");
    } else {
        LOG4CPP_INFO(logger, "Can't init OpenCV with OpenCL TAPI");
    }

    cl_bool temp = CL_FALSE;
    size_t sz = 0;
    bool unifiedmemory =
            clGetDeviceInfo(selectedDeviceID, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(temp), &temp, &sz)==CL_SUCCESS &&
                    sz==sizeof(temp) ? temp!=0 : false;
    LOG4CPP_INFO(logger, "Host Unified Memory: " << unifiedmemory);
#else // HAVE_OPENCL
    LOG4CPP_WARN( logger, "OpenCL is DISABLED!");
#endif
    m_isInitialized = true;
    notifyInitComplete();
    LOG4CPP_INFO( logger, "initialized OpenCL: " << isInitialized());
}

#ifdef HAVE_OPENCL
cl_context OpenCLManager::getContext() const
{
    return m_clContext;
}

cl_command_queue OpenCLManager::getCommandQueue() const
{
    return m_clCommandQueue;
}
#endif

} // namespace Vision
} // namespace ubitrack
