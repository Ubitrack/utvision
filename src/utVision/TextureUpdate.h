//
// Created by Ulrich Eck on 20/07/16.
//

#ifndef UBITRACK_VISION_TEXTUREUPDATE_H
#define UBITRACK_VISION_TEXTUREUPDATE_H

#include <utVision/OpenGLPlatform.h>

#include <boost/utility.hpp>

#include <utVision.h>
#include <utMeasurement/Measurement.h>
#include <utVision/Image.h>


#ifdef HAVE_OPENCL
#include <opencv2/core/ocl.hpp>
#ifdef __APPLE__
#pragma OPENCL EXTENSION CL_APPLE_gl_sharing : enable
#include "OpenCL/opencl.h"
#include "OpenCL/cl_gl.h"
#else
#include "CL/cl.h"
#include "CL/cl_gl.h"
#endif
#endif



namespace Ubitrack {
namespace Vision {

class UTVISION_EXPORT TextureUpdate : private boost::noncopyable {
public:
    TextureUpdate()
            : m_bTextureInitialized(false)
            , m_bIsExternalTexture(true)
            , m_texture(0)
            , m_pbo(0)
            , m_textureWidth(0)
            , m_textureHeight(0)
    { }

    bool getImageFormat(const Image::ImageFormatProperties& fmtSrc,
            Image::ImageFormatProperties& fmtDst,
            bool use_gpu, int& umatConvertCode,
            GLenum& glFormat, GLenum& glDatatype);

    void initializeTexture(const Measurement::ImageMeasurement& image);
    void initializeTexture(const Measurement::ImageMeasurement& image, const GLuint tex_id);
    void cleanupTexture();

    void updateTexture(const Measurement::ImageMeasurement& image);


	// @todo these names should be removed and the "textureWidth/textureHeight" should be used instead,
	//  then we can hide the "implementation detail" of using pow2 textures and/or drop it altogether ..
	unsigned int pow2width() {
		return m_textureWidth;
	}

	unsigned int pow2height() {
		return m_textureHeight;
	}
	// end of depricated methods

	unsigned int textureWidth() {
		return m_textureWidth;
	}

	unsigned int textureHeight() {
		return m_textureHeight;
	}


	bool isInitialized() {
		return m_bTextureInitialized;
	}
    GLuint textureId() {
        return m_texture;
    }

private:

    unsigned m_textureWidth;
    unsigned m_textureHeight;

    bool m_bTextureInitialized;

    GLuint m_texture;
    GLuint m_pbo;

    bool m_bIsExternalTexture;


#ifdef HAVE_OPENCL
	//OpenCL
	cl_mem m_clImage;
	cv::UMat m_convertedImage;
#endif

};

}} // Ubitrack::Vision
#endif //UBITRACK_TEXTUREUPDATE_H
