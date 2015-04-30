//wglGetCurrentContext, wglGetCurrentDC
#include <GL/glut.h>

#include "OpenCLManager.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>

//OCL
#include <opencv2/core/ocl.hpp>
#include <CL/cl_gl.h>


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

OpenCLManager::OpenCLManager(void)
{
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
		
	cl_uint selectedPlatform =0;  // simply take first platform(index 0), code for taking correct one is long and not postet here
 
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

	m_clContext = clCreateContext(props,1, &selectedDeviceID, NULL, NULL, &err);
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