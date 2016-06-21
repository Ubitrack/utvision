//wglGetCurrentContext, wglGetCurrentDC
#include <GL/glut.h>

#include "OpenCLManager.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>

//OCL
#include <opencv2/core/ocl.hpp>

#ifdef __APPLE__
    #include <OpenGL/OpenGL.h>
    #include "OpenCL/cl_gl.h"
#else
    #include "CL/cl_gl.h"
    #ifdef WIN32
        #include <CL/cl_d3d11_ext.h>
    #endif
#endif

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

OpenCLManager& OpenCLManager::singleton()
{
    static boost::mutex singletonMutex;

    boost::mutex::scoped_lock l(singletonMutex);

    //create a new singleton if necessary
    if (!g_pOpenCLManager) {
        g_pOpenCLManager.reset(new OpenCLManager);
        cv::ocl::setUseOpenCL(true);
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
    LOG4CPP_INFO(logger, "notifyOpenCLState");
}

bool OpenCLManager::isInitialized() const
{
    return m_isInitialized;
}

OpenCLManager::OpenCLManager()
        :
        m_isInitialized(false) { }

#ifdef WIN32
void OpenCLManager::initializeDirectX(ID3D11Device* pD3D11Device)
{

    if(m_isInitialized){
        return;
    }
    //Get all Platforms and select a GPU one
    cl_uint numPlatforms;
    clGetPlatformIDs (65536, NULL, &numPlatforms);
    LOG4CPP_INFO( logger, "DX: Platforms detected: " << numPlatforms );


    std::vector<cl_platform_id> platforms(numPlatforms);

    cl_int err = clGetPlatformIDs(numPlatforms, platforms.data(), NULL);
    if(err != CL_SUCCESS)
    {
        LOG4CPP_INFO( logger, "DX: error at clGetPlatformIDs :" << err );
        m_isInitialized = false;
    }

    int found = -1;
    cl_device_id device = NULL;
    cl_uint numDevices = 0;
    cl_context context = NULL;

    clGetDeviceIDsFromD3D11NV_fn clGetDeviceIDsFromD3D11NV = (clGetDeviceIDsFromD3D11NV_fn)
                clGetExtensionFunctionAddress("clGetDeviceIDsFromD3D11NV");
     for (int i = 0; i < (int)numPlatforms; i++)
    {
        if (!clGetDeviceIDsFromD3D11NV)
            break;

        device = NULL;
        numDevices = 0;
        err = clGetDeviceIDsFromD3D11NV(platforms[i], CL_D3D11_DEVICE_NV, pD3D11Device,
                CL_PREFERRED_DEVICES_FOR_D3D11_NV, 1, &device, &numDevices);
        if (err != CL_SUCCESS)
            continue;

        if (numDevices > 0)
        {
            cl_context_properties properties[] = {
                    CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[i],
                    CL_CONTEXT_D3D11_DEVICE_NV, (cl_context_properties)(pD3D11Device),
                    NULL, NULL
            };
            m_clContext = clCreateContext(properties, 1, &device, NULL, NULL, &err);
            if(!m_clContext || err!= CL_SUCCESS)
            {
                LOG4CPP_INFO( logger,  "error at clCreateContext :" << err );
            }
            else
            {
                found = i;
                break;
            }
        }
    }
     if (found < 0)
    {
        LOG4CPP_ERROR(logger, "OpenCL: no preferred D3D11_NV device found. Trying"
            << "now all devices context for DirectX interop");
        // try with CL_ALL_DEVICES_FOR_D3D11_KHR
        for (int i = 0; i < (int)numPlatforms; i++)
        {
            if (!clGetDeviceIDsFromD3D11NV)
                continue;

            device = NULL;
            numDevices = 0;
            err = clGetDeviceIDsFromD3D11NV(platforms[i], CL_D3D11_DEVICE_NV, pD3D11Device,
                    CL_ALL_DEVICES_FOR_D3D11_NV, 1, &device, &numDevices);
            if (err != CL_SUCCESS)
                continue;

            if (numDevices > 0)
            {
                cl_context_properties properties[] = {
                        CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[i],
                        CL_CONTEXT_D3D11_DEVICE_NV, (cl_context_properties)(pD3D11Device),
                        NULL, NULL
                };

                m_clContext = clCreateContext(properties, 1, &device, NULL, NULL, &err);
                   if(!m_clContext || err != CL_SUCCESS)
                {
                    LOG4CPP_INFO( logger,  "error at clCreateContext :" << err );
                }
                else
                {
                    found = i;
                    break;
                }
            }
        }
        if (found < 0){
            LOG4CPP_ERROR(logger, "OpenCL: Can't create context for DirectX interop");
            m_isInitialized = false;
            return;
        }
    }


    cl_device_id selectedDeviceID;
    //Select a GPU device
    err = clGetDeviceIDs(platforms[found], CL_DEVICE_TYPE_GPU, 1, &selectedDeviceID, NULL);
    if(err != CL_SUCCESS)
    {
        LOG4CPP_INFO( logger, "DX: error at clGetDeviceIDs :" << err );
        return;
    }

    char cDeviceNameBuffer[1024];
    clGetDeviceInfo (selectedDeviceID, CL_DEVICE_NAME, sizeof(char) *  1024, cDeviceNameBuffer, NULL);
    LOG4CPP_INFO( logger, ": DX: Device Name: "		<< cDeviceNameBuffer );


    m_clCommandQueue = clCreateCommandQueue(m_clContext, selectedDeviceID, 0, &err);
    if(!m_clCommandQueue || err!= CL_SUCCESS)
    {
        LOG4CPP_INFO( logger, "DX: error at clCreateCommandQueue :" << err );
        return;
    }

    cv::ocl::Context& oclContext = cv::ocl::Context::getDefault(false);
    oclContext.initContextFromHandle(platforms[found], m_clContext, selectedDeviceID);

    m_isInitialized = true;
    LOG4CPP_INFO( logger, "initialized OpenCL: " << isInitialized());
}
#endif // WIN32

int OpenCLManager::isExtensionSupported(const char* support_str, const char* ext_string, size_t ext_buffer_size)
{
    size_t offset = 0;
    const char* space_substr = strnstr(ext_string+offset, " ", ext_buffer_size-offset);
    size_t space_pos = space_substr ? space_substr-ext_string : 0;
    while (space_pos<ext_buffer_size) {
        if (strncmp(support_str, ext_string+offset, space_pos)==0) {
            // Device supports requested extension!
            LOG4CPP_INFO(logger, "Info: Found extension support: " << support_str);
            return 1;
        }
        // Keep searching -- skip to next token string
        offset = space_pos+1;
        space_substr = strnstr(ext_string+offset, " ", ext_buffer_size-offset);
        space_pos = space_substr ? space_substr-ext_string : 0;
    }
    LOG4CPP_WARN(logger, "Warning: Extension not supported: " << support_str);
    return 0;
}

void OpenCLManager::initializeOpenGL()
{
    if (m_isInitialized) {
        return;
    }

    //Get all Platforms and select a GPU one
    cl_uint numPlatforms;
    clGetPlatformIDs(65536, NULL, &numPlatforms);
    LOG4CPP_INFO(logger, "Platforms detected: " << numPlatforms);

    cl_platform_id* platformIDs;
    platformIDs = new cl_platform_id[numPlatforms];

    cl_int err = clGetPlatformIDs(numPlatforms, platformIDs, NULL);
    if (err!=CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "error at clGetPlatformIDs :" << err);
    }

    cl_uint selectedPlatform = 0;
    cl_platform_id selectedPlatformID = platformIDs[selectedPlatform];
    delete[] platformIDs;

    // Select a GPU device
    // find fastest GPU device based on performance metric (e.g. good on laptops with multiple GPUs)
    cl_device_id* deviceIDs;
    cl_uint num_devices = 0;
    err = clGetDeviceIDs(selectedPlatformID, CL_DEVICE_TYPE_GPU, NULL, deviceIDs, &num_devices);
    if (err!=CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "error at clGetDeviceIDs :" << err);
    }

    cl_device_id selectedDeviceID;
    cl_uint max_performance_metric = 0;
    bool found_device = false;

    for (unsigned int i=0; i<num_devices; i++) {
        cl_uint device_max_compute_units = 0;
        cl_uint device_max_frequency = 0;

        err = clGetDeviceInfo(	deviceIDs[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &device_max_compute_units, NULL);
        if (err!=CL_SUCCESS) {
            LOG4CPP_ERROR(logger, "error at clGetDeviceInfo :" << err);
        }

        err = clGetDeviceInfo(	deviceIDs[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &device_max_frequency, NULL);
        if (err!=CL_SUCCESS) {
            LOG4CPP_ERROR(logger, "error at clGetDeviceInfo :" << err);
        }

        if (max_performance_metric < (device_max_compute_units*device_max_frequency)) {
            max_performance_metric = device_max_compute_units*device_max_frequency;
            selectedDeviceID = deviceIDs[i];
            found_device = true;
        }

    }
    if (!found_device) {
        LOG4CPP_ERROR(logger, "No OpenCL GPU device found !!!");
        return;
    }

    char cDeviceNameBuffer[1024];
    clGetDeviceInfo(selectedDeviceID, CL_DEVICE_NAME, sizeof(char)*1024, cDeviceNameBuffer, NULL);
    LOG4CPP_INFO(logger, ": Device Name: " << cDeviceNameBuffer);

    // Get string containing supported device extensions
    size_t ext_size = 1024;
    char* ext_string = (char*) malloc(ext_size);
    err = clGetDeviceInfo(selectedDeviceID, CL_DEVICE_EXTENSIONS, ext_size, ext_string, &ext_size);

    // Search for GL support in extension string (space delimited)
    int supported = isExtensionSupported(CL_GL_SHARING_EXT, ext_string, ext_size);
    if (supported) {
        // Device supports context sharing with OpenGL
        LOG4CPP_INFO(logger, "Found GL Sharing Support!");
    }
    else {

        // @todo Decide what to do if no OpenCL-OpenGL sharing is available -> fallback to default
        LOG4CPP_ERROR(logger, "Device does not support GL Sharing Support!");
        return;
    }

#ifdef WIN32
    // Create CL context properties, add WGL context & handle to DC
    cl_context_properties properties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(), // WGL Context
        CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(), // WGL HDC
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
    m_clContext = clCreateContext(properties, devices, count, NULL, 0, &err);

#elif __APPLE__

    // Get current CGL Context and CGL Share group
    CGLContextObj kCGLContext = CGLGetCurrentContext();
    CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
    // Create CL context properties, add handle & share-group enum
    cl_context_properties properties[] = {
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
            (cl_context_properties) kCGLShareGroup, 0
    };
    // Create a context with device in the CGL share group
    m_clContext = clCreateContext(properties, 0, 0, NULL, 0, &err);

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
    m_clContext = clCreateContext(properties, devices, count, NULL, 0, &err);

#endif

    if (!m_clContext || err!=CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "error at clCreateContext :" << err);
    }

    cl_int status = 0;
    m_clCommandQueue = clCreateCommandQueue(m_clContext, selectedDeviceID, 0, &err);
    if (!m_clCommandQueue || err!=CL_SUCCESS) {
        LOG4CPP_ERROR(logger, "error at clCreateCommandQueue :" << err);
    }

    cv::ocl::Context& oclContext = cv::ocl::Context::getDefault(false);
    oclContext.initContextFromHandle(selectedPlatformID, m_clContext, selectedDeviceID);

    cl_bool temp = CL_FALSE;
    size_t sz = 0;
    bool unifiedmemory =
            clGetDeviceInfo(selectedDeviceID, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(temp), &temp, &sz)==CL_SUCCESS &&
                    sz==sizeof(temp) ? temp!=0 : false;
    LOG4CPP_INFO(logger, "Host Unified Memory: " << unifiedmemory);
    m_isInitialized = true;
    LOG4CPP_INFO(logger, "initialized OpenCL: " << isInitialized());
}

cl_context OpenCLManager::getContext() const
{
    return m_clContext;
}

cl_command_queue OpenCLManager::getCommandQueue() const
{
    return m_clCommandQueue;
}

OpenCLManager::~OpenCLManager(void)
{

}

} // namespace Vision
} // namespace ubitrack