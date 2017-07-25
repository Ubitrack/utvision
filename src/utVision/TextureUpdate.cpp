//
// Created by Ulrich Eck on 20/07/16.
//

#ifdef HAVE_OPENCL
#ifdef WIN32
#include <utUtil/CleanWindows.h>
#endif

#include <utVision/TextureUpdate.h>

#include <utVision/OpenCLManager.h>
#include <utUtil/Exception.h>

#include <log4cpp/Category.hh>


#ifdef _WIN32
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <utUtil/CleanWindows.h>
#elif __APPLE__
    #include <OpenGL/OpenGL.h>
    #include <OpenGL/glu.h>
#else
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glx.h>
#endif


// get a logger
static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Vision.TextureUpdate" ) );

namespace Ubitrack {
namespace Vision {

bool TextureUpdate::getImageFormat(const Image::ImageFormatProperties& fmtSrc,
        Image::ImageFormatProperties& fmtDst,
        bool use_gpu, int& umatConvertCode,
        GLenum& glFormat, GLenum& glDatatype)
{
    bool ret = true;

    // determine datatype
    switch(fmtSrc.depth) {
    case CV_8U:
        glDatatype = GL_UNSIGNED_BYTE;
        break;
    case CV_16U:
        glDatatype = GL_UNSIGNED_SHORT;
        break;
    case CV_32F:
        glDatatype = GL_FLOAT;
        break;
    case CV_64F:
        glDatatype = GL_DOUBLE;
        break;
    default:
        // assume unsigned byte
        glDatatype = GL_UNSIGNED_BYTE;
        // Log Error ?
        ret = false;
        break;
    }

    // determine image properties
    switch (fmtSrc.imageFormat) {
    case Image::LUMINANCE:
        glFormat = GL_LUMINANCE;
        fmtDst.channels = 1;
        break;
    case Image::RGB:
        glFormat = use_gpu ? GL_RGBA : GL_RGB;
        fmtDst.channels = use_gpu ? 4 : 3;
        fmtDst.imageFormat = use_gpu ? Image::RGBA : Image::RGB;
        umatConvertCode = cv::COLOR_RGB2RGBA;
        break;
#ifndef GL_BGR_EXT
    case Image::BGR:
        fmtDst.channels = use_gpu ? 4 : 3;
        glFormat = image_isOnGPU ? GL_RGBA : GL_RGB;
        fmtDst.imageFormat = use_gpu ? Image::RGBA : Image::BGR;
        umatConvertCode = cv::COLOR_BGR2RGBA;
        break;
    case Image::BGRA:
        fmt.channels = 4;
        glFormat = use_gpu ? GL_RGBA : GL_BGRA;
        fmtDst.imageFormat = use_gpu ? Image::RGBA : Image::BGRA;
        umatConvertCode = cv::COLOR_BGRA2RGBA;
        break;
#else
    case Image::BGR:
        fmtDst.channels = use_gpu ? 4 : 3;
        glFormat = use_gpu ? GL_RGBA : GL_BGR_EXT;
        fmtDst.imageFormat = use_gpu ? Image::RGBA : Image::BGR;
        umatConvertCode = cv::COLOR_BGR2RGBA;
        break;
    case Image::BGRA:
        fmtDst.channels = 4;
        glFormat = use_gpu ? GL_RGBA : GL_BGRA_EXT;
        fmtDst.imageFormat = use_gpu ? Image::RGBA : Image::BGRA;
        umatConvertCode = cv::COLOR_BGRA2RGBA;
        break;
#endif
    case Image::RGBA:
        fmtDst.channels = 4;
        glFormat = GL_RGBA;
        fmtDst.imageFormat = Image::RGBA;
        break;
    default:
        // Log Error ?
        ret = false;
        break;
    }

    // update dependent parameters
    fmtDst.bitsPerPixel = fmtSrc.bitsPerPixel / fmtSrc.channels * fmtDst.channels;
    fmtDst.matType = CV_MAKE_TYPE(fmtDst.depth, fmtDst.channels);

    return ret;
}

void TextureUpdate::cleanupTexture() {
    if (m_bTextureInitialized ) {
        glBindTexture( GL_TEXTURE_2D, 0 );
        glDisable( GL_TEXTURE_2D );
        if (!m_bIsExternalTexture) {
            glDeleteTextures( 1, &(m_texture) );
        }
    }
}

void TextureUpdate::initializeTexture(const Measurement::ImageMeasurement& image) {
    if (!image) {
        return;
    }

    if (!m_bTextureInitialized) {
        GLuint tex_id;
        glGenTextures( 1, &(tex_id) );
        m_bIsExternalTexture = false;
        initializeTexture(image, tex_id);
    }
}

void TextureUpdate::initializeTexture(const Measurement::ImageMeasurement& image, const GLuint tex_id) {

#ifdef HAVE_OPENCV
    if (!image) {
        return;
    }

    if ( !m_bTextureInitialized )
    {
        // access OCL Manager and initialize if needed
        Vision::OpenCLManager& oclManager = Vision::OpenCLManager::singleton();

        // if OpenCL is enabled and image is on GPU, then use OCL codepath
        bool image_isOnGPU = oclManager.isEnabled() & image->isOnGPU();

        // find out texture format
        int umatConvertCode = -1;
        GLenum glFormat = GL_LUMINANCE;
        GLenum glDatatype = GL_UNSIGNED_BYTE;
        int numOfChannels = 1;
        Image::ImageFormatProperties fmtSrc, fmtDst;
        image->getFormatProperties(fmtSrc);
        image->getFormatProperties(fmtDst);

        getImageFormat(fmtSrc, fmtDst, image_isOnGPU, umatConvertCode, glFormat, glDatatype);

        // generate power-of-two sizes
        m_pow2Width = 1;
        while ( m_pow2Width < (unsigned)image->width() )
            m_pow2Width <<= 1;

        m_pow2Height = 1;
        while ( m_pow2Height < (unsigned)image->height() )
            m_pow2Height <<= 1;

        m_texture = tex_id;

        glBindTexture( GL_TEXTURE_2D, m_texture );

        // define texture parameters
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );

        // load empty texture image (defines texture size)
        glTexImage2D( GL_TEXTURE_2D, 0, fmtDst.channels, m_pow2Width, m_pow2Height, 0, glFormat, glDatatype, 0 );
        LOG4CPP_DEBUG( logger, "glTexImage2D( width=" << m_pow2Width << ", height=" << m_pow2Height << " ): " << glGetError() );
        LOG4CPP_INFO( logger, "initalized texture ( " << glFormat << " ) OnGPU: " << image_isOnGPU);


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
                LOG4CPP_ERROR( logger, "error at  clCreateFromGLTexture2D:" << getOpenCLErrorString(err) );
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
        GLenum glFormat = GL_LUMINANCE;
        GLenum glDatatype = GL_UNSIGNED_BYTE;
        int numOfChannels = 1;
        Image::ImageFormatProperties fmtSrc, fmtDst;
        image->getFormatProperties(fmtSrc);
        image->getFormatProperties(fmtDst);

        getImageFormat(fmtSrc, fmtDst, image_isOnGPU, umatConvertCode, glFormat, glDatatype);

        if (image_isOnGPU) {
#ifdef HAVE_OPENCL

            glBindTexture( GL_TEXTURE_2D, m_texture );

            // @todo this probably causes unwanted delay - .. except when executed on gpu ...
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
                LOG4CPP_ERROR( logger, "error at  clEnqueueAcquireGLObjects:" << getOpenCLErrorString(err) );
            }

            cl_mem clBuffer = (cl_mem) m_convertedImage.handle(cv::ACCESS_READ);
            cl_command_queue cv_ocl_queue = (cl_command_queue)cv::ocl::Queue::getDefault().ptr();

            size_t offset = 0;
            size_t dst_origin[3] = {0, 0, 0};
            size_t region[3] = {static_cast<size_t>(m_convertedImage.cols), static_cast<size_t>(m_convertedImage.rows), 1};

            err = clEnqueueCopyBufferToImage(cv_ocl_queue, clBuffer, m_clImage, offset, dst_origin, region, 0, NULL, NULL);
            if (err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clEnqueueCopyBufferToImage:" << getOpenCLErrorString(err) );
            }

            err = clEnqueueReleaseGLObjects(commandQueue, 1, &m_clImage, 0, NULL, NULL);
            if(err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clEnqueueReleaseGLObjects:" << getOpenCLErrorString(err) );
            }
            cv::ocl::finish();


#else // HAVE_OPENCL
            LOG4CPP_ERROR( logger, "Image isOnGPU but OpenCL is disabled!!");
#endif // HAVE_OPENCL
        } else {
            // load image from CPU buffer into texture
            glBindTexture( GL_TEXTURE_2D, m_texture );
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, image->width(), image->height(),
                    glFormat, glDatatype, image->Mat().data );

        }

    }
#endif // HAVE_OPENCV
}

}} // Ubitrack::Vision
#endif //HAVE_OPENCL