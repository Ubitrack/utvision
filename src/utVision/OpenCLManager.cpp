//wglGetCurrentContext, wglGetCurrentDC
#include <GL/glut.h>

#include "OpenCLManager.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>

//OCL
#include <opencv2/core/ocl.hpp>
#include <CL/cl_gl.h>
#include <CL/cl_d3d11_ext.h>

#include <log4cpp/Category.hh>

// get loggers
static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Vision.OpenCLManager" ) );

namespace Ubitrack { namespace Vision {

static boost::scoped_ptr< OpenCLManager > g_pOpenCLManager;

OpenCLManager& OpenCLManager::singleton()
{
	static boost::mutex singletonMutex;
	
	boost::mutex::scoped_lock l( singletonMutex );

	//create a new singleton if necessary
	if( !g_pOpenCLManager ){
		g_pOpenCLManager.reset( new OpenCLManager );
		cv::ocl::setUseOpenCL(true);
	}
	return *g_pOpenCLManager;
}

void OpenCLManager::destroyOpenCLManager()
{
	g_pOpenCLManager.reset( 0 );
}

void notifyOpenCLState(const char *errinfo, const void  *private_info, size_t  cb, void  *user_data)
{
	LOG4CPP_INFO( logger, "notifyOpenCLState");
}

bool OpenCLManager::isInitialized() const
{
	return m_isInitialized;
}

OpenCLManager::OpenCLManager() :
	  m_isInitialized(false)
{}

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
	/*cl_platform_id* platformIDs;
	platformIDs = new cl_platform_id[numPlatforms];*/
 
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




	//cl_platform_id selectedPlatformID = platformIDs[selectedPlatform];
	//delete[] platformIDs; 	
 //
	//cl_device_id selectedDeviceID; 
	////Select a GPU device
	//err = clGetDeviceIDs(selectedPlatformID, CL_DEVICE_TYPE_GPU, 1, &selectedDeviceID, NULL);
	//if(err != CL_SUCCESS)
	//{
	//	LOG4CPP_INFO( logger, "DX: error at clGetDeviceIDs :" << err );
	//}

	//char cDeviceNameBuffer[1024];
	//clGetDeviceInfo (selectedDeviceID, CL_DEVICE_NAME, sizeof(char) *  1024, cDeviceNameBuffer, NULL);
	//LOG4CPP_INFO( logger, ": DX: Device Name: "		<< cDeviceNameBuffer );

	//cl_context_properties properties[] = {
 //               CL_CONTEXT_PLATFORM, (cl_context_properties)selectedPlatformID,
 //               CL_CONTEXT_D3D11_DEVICE_NV, (cl_context_properties)(pD3D11Device)
	//};

 //   m_clContext = clCreateContext(properties, 1, &selectedDeviceID, NULL, NULL, &err);
	//
	//if(!m_clContext || err!= CL_SUCCESS)
	//{
	//	LOG4CPP_INFO( logger,  "DX: error at clCreateContext :" << err );
	//}

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

void OpenCLManager::initializeOpenGL()
{
	if(m_isInitialized){
		return;
	}
	//Get all Platforms and select a GPU one
	cl_uint numPlatforms;
	clGetPlatformIDs (65536, NULL, &numPlatforms); 
	LOG4CPP_INFO( logger, "Platforms detected: " << numPlatforms );
 
	cl_platform_id* platformIDs;
	platformIDs = new cl_platform_id[numPlatforms];
 
	cl_int err = clGetPlatformIDs(numPlatforms, platformIDs, NULL);
	if(err != CL_SUCCESS)
	{
		LOG4CPP_INFO( logger, "error at clGetPlatformIDs :" << err );
	}
		
	cl_uint selectedPlatform =0;  
 
	cl_platform_id selectedPlatformID = platformIDs[selectedPlatform];
	delete[] platformIDs; 	
 
	cl_device_id selectedDeviceID; 
	//Select a GPU device
	err = clGetDeviceIDs(selectedPlatformID, CL_DEVICE_TYPE_GPU, 1, &selectedDeviceID, NULL);
	if(err != CL_SUCCESS)
	{
		LOG4CPP_INFO( logger, "error at clGetDeviceIDs :" << err );
	}

	char cDeviceNameBuffer[1024];
	clGetDeviceInfo (selectedDeviceID, CL_DEVICE_NAME, sizeof(char) *  1024, cDeviceNameBuffer, NULL);
	LOG4CPP_INFO( logger, ": Device Name: "		<< cDeviceNameBuffer );

	
	//Get a context with OpenGL connection
	cl_context_properties props[] = { 
		CL_GL_CONTEXT_KHR, 
		(cl_context_properties)wglGetCurrentContext(), 
		CL_WGL_HDC_KHR, 
		(cl_context_properties) wglGetCurrentDC(), 
		CL_CONTEXT_PLATFORM, 
		(cl_context_properties) selectedPlatformID, 
		0};
 
	cl_int status = 0;
	m_clContext = clCreateContextFromType(props, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
	//m_clContext = clCreateContext(props,1, &selectedDeviceID, NULL, NULL, &err);
	if(!m_clContext || err!= CL_SUCCESS)
	{
		LOG4CPP_INFO( logger,  "error at clCreateContext :" << err );
	}
	
	m_clCommandQueue = clCreateCommandQueue(m_clContext, selectedDeviceID, 0,&err);
	if(!m_clCommandQueue || err!= CL_SUCCESS)
	{
		LOG4CPP_INFO( logger, "error at clCreateCommandQueue :" << err );
	}
	
	cv::ocl::Context& oclContext = cv::ocl::Context::getDefault(false);
	oclContext.initContextFromHandle(selectedPlatformID, m_clContext, selectedDeviceID);	

	cl_bool temp = CL_FALSE;
    size_t sz = 0;

    bool unifiedmemory = clGetDeviceInfo(selectedDeviceID, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(temp), &temp, &sz) == CL_SUCCESS &&
            sz == sizeof(temp) ? temp != 0 : false;
	LOG4CPP_INFO( logger, "Host Unified Memory: " << unifiedmemory);
	m_isInitialized = true;
	LOG4CPP_INFO( logger, "initialized OpenCL: " << isInitialized());
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

}}