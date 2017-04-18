//
// Created by Ulrich Eck on 20/07/16.
//

#ifndef UBITRACK_VISION_TEXTUREUPDATE_H
#define UBITRACK_VISION_TEXTUREUPDATE_H

#include <boost/utility.hpp>

#include <utVision.h>
#include <utMeasurement/Measurement.h>
#include <utVision/Image.h>

#ifdef HAVE_OPENCL
#include <opencv2/core/ocl.hpp>
#ifdef __APPLE__
#include "OpenCL/opencl.h"
#include "OpenCL/cl_gl.h"
#else
#include "CL/cl.h"
#include "CL/cl_gl.h"
#include "GL/gl.h"
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
            , m_pow2Width(0)
            , m_pow2Height(0)
    { }

    bool getImageFormat(const Image::ImageFormatProperties& fmtSrc,
            Image::ImageFormatProperties& fmtDst,
            bool use_gpu, int& umatConvertCode,
            GLenum& glFormat, GLenum& glDatatype);

    void initializeTexture(const Measurement::ImageMeasurement& image);
    void initializeTexture(const Measurement::ImageMeasurement& image, const GLuint tex_id);
    void cleanupTexture();

    void updateTexture(const Measurement::ImageMeasurement& image);

	unsigned int pow2width() {
		return m_pow2Width;
	}

	unsigned int pow2height() {
		return m_pow2Height;
	}

	bool isInitialized() {
		return m_bTextureInitialized;
	}
    GLuint textureId() {
        return m_texture;
    }

private:

    // should be private - needs to update dependencies (utvisualization, h3dintegration)
    unsigned m_pow2Width;
    unsigned m_pow2Height;

    bool m_bTextureInitialized;
    bool m_bIsExternalTexture;
    GLuint m_texture;

#ifdef HAVE_OPENCL
    //OpenCL
  cl_mem m_clImage;
  cv::UMat m_convertedImage;
#endif

};

}} // Ubitrack::Vision
#endif //UBITRACK_TEXTUREUPDATE_H
