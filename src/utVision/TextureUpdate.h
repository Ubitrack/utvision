//
// Created by Ulrich Eck on 20/07/16.
//

#ifndef UBITRACK_VISION_TEXTUREUPDATE_H
#define UBITRACK_VISION_TEXTUREUPDATE_H

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

class UTVISION_EXPORT TextureUpdate {
public:
    TextureUpdate() : m_bTextureInitialized(false), m_texture(0), m_pow2Width(0), m_pow2Height(0) { }

    bool getImageFormat(const Measurement::ImageMeasurement& image, bool use_gpu, int& umatConvertCode,
            GLenum& imgFormat, int& numOfChannels);

    void initializeTexture(const Measurement::ImageMeasurement& image);
    void cleanupTexture();

    void updateTexture(const Measurement::ImageMeasurement& image);

    bool m_bTextureInitialized;
    GLuint m_texture;

#ifdef HAVE_OPENCL
    //OpenCL
  cl_mem m_clImage;
  cv::UMat m_convertedImage;
#endif

    unsigned m_pow2Width;
    unsigned m_pow2Height;
};

}} // Ubitrack::Vision
#endif //UBITRACK_TEXTUREUPDATE_H
