//
// Created by Ulrich Eck on 20/07/16.
//

#ifdef WIN32
#include <utUtil/CleanWindows.h>
#endif

#include <utVision/TextureUpdate.h>

#include <utVision/OpenCLManager.h>
#include <utUtil/Exception.h>

#include <GL/glut.h>

#include <log4cpp/Category.hh>


// get a logger
static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Vision.TextureUpdate" ) );

namespace Ubitrack {
namespace Vision {

bool TextureUpdate::getImageFormat(const Measurement::ImageMeasurement& image, bool use_gpu, int& umatConvertCode,
        GLenum& imgFormat, int& numOfChannels)
{
    bool ret = true;
    switch (image->pixelFormat()) {
    case Image::LUMINANCE:
        imgFormat = GL_LUMINANCE;
        numOfChannels = 1;
        break;
    case Image::RGB:
        numOfChannels = use_gpu ? 4 : 3;
        imgFormat = use_gpu ? GL_RGBA : GL_RGB;
        umatConvertCode = cv::COLOR_RGB2RGBA;
        break;
#ifndef GL_BGR_EXT
    case Image::BGR:
        imgFormat = image_isOnGPU ? GL_RGBA : GL_RGB;
        numOfChannels = use_gpu ? 4 : 3;
        umatConvertCode = cv::COLOR_BGR2RGBA;
        break;
    case Image::BGRA:
        numOfChannels = 4;
        imgFormat = use_gpu ? GL_RGBA : GL_BGRA;
        umatConvertCode = cv::COLOR_BGRA2RGBA;
        break;
#else
        case Image::BGR:
        numOfChannels = use_gpu ? 4 : 3;
        imgFormat = use_gpu ? GL_RGBA : GL_BGR_EXT;
        umatConvertCode = cv::COLOR_BGR2RGBA;
        break;
    case Image::BGRA:
        numOfChannels = 4;
        imgFormat = use_gpu ? GL_RGBA : GL_BGRA_EXT;
        umatConvertCode = cv::COLOR_BGRA2RGBA;
        break;
#endif
    case Image::RGBA:
        numOfChannels = 4;
        imgFormat = GL_RGBA;
        break;
    default:
        // Log Error ?
        ret = false;
        break;
    }
    return ret;
}

void TextureUpdate::cleanupTexture() {
    if (m_bTextureInitialized ) {
        glBindTexture( GL_TEXTURE_2D, 0 );
        glDisable( GL_TEXTURE_2D );
        glDeleteTextures( 1, &(m_texture) );
    }
}

void TextureUpdate::initializeTexture(const Measurement::ImageMeasurement& image) {
#ifdef HAVE_OPENCV
   // access OCL Manager and initialize if needed
    Vision::OpenCLManager& oclManager = Vision::OpenCLManager::singleton();

    if (!image) {
        // LOG4CPP_WARN ??
        return;
    }

    // if OpenCL is enabled and image is on GPU, then use OCL codepath
    bool image_isOnGPU = oclManager.isEnabled() & image->isOnGPU();

    // find out texture format
    int umatConvertCode = -1;
    GLenum imgFormat = GL_LUMINANCE;
    int numOfChannels = 1;
    getImageFormat(image, image_isOnGPU, umatConvertCode, imgFormat, numOfChannels);

    if ( !m_bTextureInitialized )
    {

        // generate power-of-two sizes
        m_pow2Width = 1;
        while ( m_pow2Width < (unsigned)image->width() )
            m_pow2Width <<= 1;

        m_pow2Height = 1;
        while ( m_pow2Height < (unsigned)image->height() )
            m_pow2Height <<= 1;

        glGenTextures( 1, &(m_texture) );
        glBindTexture( GL_TEXTURE_2D, m_texture );

        // define texture parameters
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );

        // load empty texture image (defines texture size)
        glTexImage2D( GL_TEXTURE_2D, 0, numOfChannels, m_pow2Width, m_pow2Height, 0, imgFormat, GL_UNSIGNED_BYTE, 0 );
        LOG4CPP_DEBUG( logger, "glTexImage2D( width=" << m_pow2Width << ", height=" << m_pow2Height << " ): " << glGetError() );
        LOG4CPP_INFO( logger, "initalized texture ( " << imgFormat << " ) OnGPU: " << image_isOnGPU);


        if (oclManager.isInitialized()) {

#ifdef HAVE_OPENCL
            //Get an image Object from the OpenGL texture
            cl_int err;
// windows specific or opencl version specific ??
#ifdef WIN32
            m_clImage = clCreateFromGLTexture2D( oclManager.getContext(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, m_texture, &err);
#else
            m_clImage = clCreateFromGLTexture( oclManager.getContext(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, m_texture, &err);
#endif
            if (err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clCreateFromGLTexture2D:" << err );
            }
#endif
        }
        m_bTextureInitialized = true;
    }
#endif // HAVE_OPENCV
}

/*
 * Update Texture - requires valid OpenGL Context
 */
void TextureUpdate::updateTexture(const Measurement::ImageMeasurement& image) {
#ifdef HAVE_OPENCV
    // access OCL Manager and initialize if needed
    Vision::OpenCLManager& oclManager = Vision::OpenCLManager::singleton();

    if (!image) {
        // LOG4CPP_WARN ??
        return;
    }


    // if OpenCL is enabled and image is on GPU, then use OCL codepath
    bool image_isOnGPU = oclManager.isInitialized() & image->isOnGPU();

    if ( m_bTextureInitialized )
    {
        // check if received image fits into the allocated texture

        // find out texture format
        int umatConvertCode = -1;
        GLenum imgFormat = GL_LUMINANCE;
        int numOfChannels = 1;
        getImageFormat(image, image_isOnGPU, umatConvertCode, imgFormat, numOfChannels);

        if (image_isOnGPU) {
#ifdef HAVE_OPENCL

            glBindTexture( GL_TEXTURE_2D, m_texture );

            if (umatConvertCode != -1) {
                cv::cvtColor(image->uMat(), m_convertedImage, umatConvertCode );
            } else {
                m_convertedImage = image->uMat();
            }

            cv::ocl::finish();
            glFinish();

            cl_command_queue commandQueue = oclManager.getCommandQueue();
            cl_int err;

            clFinish(commandQueue);

            err = clEnqueueAcquireGLObjects(commandQueue, 1, &(m_clImage), 0, NULL, NULL);
            if(err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clEnqueueAcquireGLObjects:" << err );
            }

            cl_mem clBuffer = (cl_mem) m_convertedImage.handle(cv::ACCESS_READ);
            cl_command_queue cv_ocl_queue = (cl_command_queue)cv::ocl::Queue::getDefault().ptr();

            size_t offset = 0;
            size_t dst_origin[3] = {0, 0, 0};
            size_t region[3] = {static_cast<size_t>(m_convertedImage.cols), static_cast<size_t>(m_convertedImage.rows), 1};

            err = clEnqueueCopyBufferToImage(cv_ocl_queue, clBuffer, m_clImage, offset, dst_origin, region, 0, NULL, NULL);
            if (err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clEnqueueCopyBufferToImage:" << err );
            }

            err = clEnqueueReleaseGLObjects(commandQueue, 1, &m_clImage, 0, NULL, NULL);
            if(err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clEnqueueReleaseGLObjects:" << err );
            }
            cv::ocl::finish();


#else // HAVE_OPENCL
            LOG4CPP_ERROR( logger, "Image isOnGPU but OpenCL is disabled!!");
#endif // HAVE_OPENCL
        } else {
            // load image from CPU buffer into texture
            glBindTexture( GL_TEXTURE_2D, m_texture );
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, image->width(), image->height(),
                    imgFormat, GL_UNSIGNED_BYTE, image->Mat().data );

        }

    }
#endif // HAVE_OPENCV
}

}} // Ubitrack::Vision