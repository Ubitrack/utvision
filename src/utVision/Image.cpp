/*
 * Ubitrack - Library for Ubiquitous Tracking
 * Copyright 2006, Technische Universitaet Muenchen, and individual
 * contributors as indicated by the @authors tag. See the 
 * copyright.txt in the distribution for a full listing of individual
 * contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */

/**
 * @ingroup vision
 * @file
 * Implements basic routines for handling OpenCV images in a "Resource Acquisition Is Initialization" (RAII) fashion
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include <opencv/cv.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <utUtil/Exception.h>
#include "Image.h"
#include <log4cpp/Category.hh>

#include <opencv2/core/ocl.hpp>
#include <utUtil/TracingProvider.h>

// @todo add logging to image class ?

namespace Ubitrack { namespace Vision {


Image::Image( int nWidth, int nHeight, ImageFormatProperties& fmt, void* pImageData )
		: m_bOwned( false )
		, m_uploadState(OnCPU)
		, m_width(nWidth)
		, m_height(nHeight)
		, m_channels(fmt.channels)
		, m_origin(fmt.origin)
		, m_format(fmt.imageFormat)
		, m_bitsPerPixel(fmt.bitsPerPixel)
		, m_depth(fmt.depth)
{
	// @todo nAlign parameter is ignored for now - do we need it ?
	// @todo no tracing of allocation here ..
	m_cpuImage = cv::Mat(cv::Size( nWidth, nHeight), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(m_depth, m_channels),
			static_cast< char* >( pImageData ), cv::Mat::AUTO_STEP);
}


Image::Image( int nWidth, int nHeight, ImageFormatProperties& fmt, ImageUploadState nState)
		: m_bOwned( true )
		, m_uploadState(nState)
		, m_width(nWidth)
		, m_height(nHeight)
		, m_channels(fmt.channels)
		, m_origin(fmt.origin)
		, m_format(fmt.imageFormat)
		, m_bitsPerPixel(fmt.bitsPerPixel)
		, m_depth(fmt.depth)
{
	if (isOnGPU()) {

#ifdef ENABLE_EVENT_TRACING
		TRACEPOINT_VISION_ALLOCATE_GPU(m_width*m_height*m_channels)
#endif
		m_gpuImage = cv::UMat(height(), width(), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(m_depth, m_channels));
	} else if (isOnCPU()) {

#ifdef ENABLE_EVENT_TRACING
		TRACEPOINT_VISION_ALLOCATE_CPU(m_width*m_height*m_channels)
#endif
		m_cpuImage =  cv::Mat(height(), width(), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(m_depth, m_channels));
	} else {
		LOG4CPP_ERROR( imageLogger, "Trying to allocate CPU and GPU buffer at the same time !!!");
	}
}


Image::Image( int nWidth, int nHeight, int nChannels, void* pImageData, int nDepth, int nOrigin, int nAlign )
	: m_bOwned( false )
	, m_uploadState(OnCPU)
	, m_width(nWidth)
	, m_height(nHeight)
	, m_channels(nChannels)
	, m_depth(nDepth)
	, m_origin(nOrigin)
{
	ImageFormatProperties fmt;
	guessFormat(fmt, nChannels, nDepth);
	m_format = fmt.imageFormat;
	m_bitsPerPixel = fmt.bitsPerPixel;

	// @todo nAlign parameter is ignored for now - do we need it ?
	// @todo no tracing of allocation here ..
	m_cpuImage = cv::Mat(cv::Size( nWidth, nHeight), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(nDepth, nChannels),
			static_cast< char* >( pImageData ), cv::Mat::AUTO_STEP);
}

Image::Image( int nWidth, int nHeight, int nChannels, int nDepth, int nOrigin, ImageUploadState nState)
	: m_bOwned( true )
	, m_uploadState(nState)
	, m_width(nWidth)
	, m_height(nHeight)
	, m_channels(nChannels)
	, m_depth(nDepth)
	, m_origin(nOrigin)
{
	ImageFormatProperties fmt;
	guessFormat(fmt, nChannels, nDepth);
	m_format = fmt.imageFormat;
	m_bitsPerPixel = fmt.bitsPerPixel;

	if (isOnGPU()) {

#ifdef ENABLE_EVENT_TRACING
		TRACEPOINT_VISION_ALLOCATE_GPU(m_width*m_height*m_channels)
#endif
		m_gpuImage = cv::UMat(height(), width(), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(nDepth, nChannels));
	} else if (isOnCPU()) {

#ifdef ENABLE_EVENT_TRACING
		TRACEPOINT_VISION_ALLOCATE_CPU(m_width*m_height*m_channels)
#endif
		m_cpuImage =  cv::Mat(height(), width(), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(nDepth, nChannels));
	} else {
		LOG4CPP_ERROR( imageLogger, "Trying to allocate CPU and GPU buffer at the same time !!!");
	}
}


Image::Image( IplImage* pIplImage, bool bDestroy )
	: m_bOwned( bDestroy )
	, m_uploadState(OnCPU)
	, m_width(pIplImage->width)
	, m_height(pIplImage->height)
	, m_channels(pIplImage->nChannels)
	, m_origin(pIplImage->origin)
{
	ImageFormatProperties fmt;
	guessFormat(fmt, pIplImage);
	m_format = fmt.imageFormat;
	m_depth = fmt.depth;

	m_cpuImage = cv::cvarrToMat(pIplImage);
	if ( bDestroy )
		cvReleaseImageHeader( &pIplImage );
}

Image::Image(cv::UMat & img)
	: m_bOwned( true )
	, m_uploadState( OnGPU )
	, m_width(img.cols)
	, m_height(img.rows)
{

	ImageFormatProperties fmt;
	guessFormat(fmt, img);
	m_format = fmt.imageFormat;
	m_channels = fmt.channels;
	m_depth = fmt.depth;
	m_bitsPerPixel = fmt.bitsPerPixel;
	m_origin = fmt.origin;

	m_gpuImage = img;
}

Image::Image(cv::UMat & img, ImageFormatProperties& fmt)
		: m_bOwned( true )
		, m_uploadState( OnGPU )
		, m_width(img.cols)
		, m_height(img.rows)
{
	m_format = fmt.imageFormat;
	m_channels = fmt.channels;
	m_depth = fmt.depth;
	m_bitsPerPixel = fmt.bitsPerPixel;
	m_origin = fmt.origin;

	m_gpuImage = img;
}


Image::Image( cv::Mat & img )
	: m_bOwned( true )
	, m_uploadState(OnCPU)
	, m_width(img.cols)
	, m_height(img.rows)
{

	ImageFormatProperties fmt;
	guessFormat(fmt, img);
	m_format = fmt.imageFormat;
	m_channels = fmt.channels;
	m_depth = fmt.depth;
	m_bitsPerPixel = fmt.bitsPerPixel;
	m_origin = fmt.origin;

	m_cpuImage = img;
}

Image::Image( cv::Mat & img, ImageFormatProperties& fmt )
		: m_bOwned( true )
		, m_uploadState(OnCPU)
		, m_width(img.cols)
		, m_height(img.rows)
{
	m_format = fmt.imageFormat;
	m_channels = fmt.channels;
	m_depth = fmt.depth;
	m_bitsPerPixel = fmt.bitsPerPixel;
	m_origin = fmt.origin;

	m_cpuImage = img;
}


Image::~Image()
{
	//destroy the buffers

	// @todo image destruction should be traced as well !!!
}


void Image::guessFormat(ImageFormatProperties& result, int channels, int depth, int matType) {

	result.channels = channels;
	result.depth = CV_8U;
	result.bitsPerPixel = 8;
	result.imageFormat = Image::UNKNOWN_PIXELFORMAT;


	if (depth != -1) {

		// check CV Matrix Element type
		switch(depth) {
		case CV_8U:
			result.bitsPerPixel = 8 * channels;
            result.depth = depth;
			break;
		case CV_8S:
			result.bitsPerPixel = 8 * channels;
            result.depth = depth;
			break;
		case CV_16U:
			result.bitsPerPixel = 16 * channels;
            result.depth = depth;
			break;
		case CV_16S:
			result.bitsPerPixel = 16 * channels;
            result.depth = depth;
			break;
		case CV_32S:
			result.bitsPerPixel = 32 * channels;
            result.depth = depth;
			break;
		case CV_32F:
			result.bitsPerPixel = 32 * channels;
            result.depth = depth;
			break;
		case CV_64F:
			result.bitsPerPixel = 64 * channels;
            result.depth = depth;
			break;
		default:
    		LOG4CPP_WARN(imageLogger, "Unknown Matrix-Element Type: " << depth);
		}
	}


	if (matType != -1) {
		// check CV Matrix Type
		switch(matType) {
		case CV_8UC1:
			result.imageFormat = Image::LUMINANCE;
			result.bitsPerPixel = 8;
			result.depth = CV_8U;
			result.channels = 1;
			break;
		case CV_8UC3:
			result.imageFormat = Image::RGB;
			result.bitsPerPixel = 24;
			result.depth = CV_8U;
			result.channels = 3;
			break;
		case CV_8UC4:
			result.imageFormat = Image::RGBA;
			result.bitsPerPixel = 32;
			result.depth = CV_8U;
			result.channels = 4;
			break;
		case CV_8SC1:
			result.imageFormat = Image::LUMINANCE;
			result.bitsPerPixel = 8;
			result.depth = CV_8S;
			result.channels = 1;
			break;
		case CV_8SC3:
			result.imageFormat = Image::RGB;
			result.bitsPerPixel = 24;
			result.depth = CV_8S;
			result.channels = 3;
			break;
		case CV_8SC4:
			result.imageFormat = Image::RGBA;
			result.bitsPerPixel = 32;
			result.depth = CV_8S;
			result.channels = 4;
			break;
		case CV_16UC1:
			result.imageFormat = Image::LUMINANCE;
			result.bitsPerPixel = 16;
			result.depth = CV_16U;
			result.channels = 1;
			break;
		case CV_16UC3:
			result.imageFormat = Image::RGB;
			result.bitsPerPixel = 48;
			result.depth = CV_16U;
			result.channels = 3;
			break;
		case CV_16UC4:
			result.imageFormat = Image::RGBA;
			result.bitsPerPixel = 64;
			result.depth = CV_16U;
			result.channels = 4;
			break;
		case CV_16SC1:
			result.imageFormat = Image::LUMINANCE;
			result.bitsPerPixel = 16;
			result.depth = CV_16S;
			result.channels = 1;
			break;
		case CV_16SC3:
			result.imageFormat = Image::RGB;
			result.bitsPerPixel = 48;
			result.depth = CV_16S;
			result.channels = 3;
			break;
		case CV_16SC4:
			result.imageFormat = Image::RGBA;
			result.bitsPerPixel = 64;
			result.depth = CV_16S;
			result.channels = 4;
			break;
		case CV_32SC1:
			result.imageFormat = Image::LUMINANCE;
			result.bitsPerPixel = 32;
			result.depth = CV_32S;
			result.channels = 1;
			break;
		case CV_32SC3:
			result.imageFormat = Image::RGB;
			result.bitsPerPixel = 96;
			result.depth = CV_32S;
			result.channels = 3;
			break;
		case CV_32SC4:
			result.imageFormat = Image::RGBA;
			result.bitsPerPixel = 128;
			result.depth = CV_32S;
			result.channels = 4;
			break;
		case CV_32FC1:
			result.imageFormat = Image::LUMINANCE;
			result.bitsPerPixel = 32;
			result.depth = CV_32F;
			result.channels = 1;
			break;
		case CV_32FC3:
			result.imageFormat = Image::RGB;
			result.bitsPerPixel = 96;
			result.depth = CV_32F;
			result.channels = 3;
			break;
		case CV_32FC4:
			result.imageFormat = Image::RGBA;
			result.bitsPerPixel = 128;
			result.depth = CV_32F;
			result.channels = 4;
			break;
		case CV_64FC1:
			result.imageFormat = Image::LUMINANCE;
			result.bitsPerPixel = 64;
			result.depth = CV_64F;
			result.channels = 1;
			break;
		case CV_64FC3:
			result.imageFormat = Image::RGB;
			result.bitsPerPixel = 192;
			result.depth = CV_64F;
			result.channels = 3;
			break;
		case CV_64FC4:
			result.imageFormat = Image::RGBA;
			result.bitsPerPixel = 256;
			result.depth = CV_64F;
			result.channels = 4;
			break;
		default:
		LOG4CPP_WARN(imageLogger, "Unknown Matrix Type: " << matType);
		}
	}

	if (result.imageFormat == Image::UNKNOWN_PIXELFORMAT) {
		// guess some image format
		switch (channels) {
		case 1:
			result.imageFormat = Image::LUMINANCE;
			break;
		case 3:
			result.imageFormat = Image::RGB;
			break;
		case 4:
			result.imageFormat = Image::RGBA;
			break;
		default:
		LOG4CPP_WARN(imageLogger, "Unexpected number of channels: " << channels);
		}
	}

    LOG4CPP_TRACE(imageLogger, "Guessed Format: "
            << " imageFormat: " << result.imageFormat
            << " matType: " << result.matType
            << " origin: " << result.origin
            << " depth: " << result.depth
            << " bitsPerPixel: " << result.bitsPerPixel
            << " channels: " << result.channels
    )
}

void Image::guessFormat(ImageFormatProperties& result, cv::Mat m) {
	guessFormat(result, m.channels(), m.depth(), m.type());
}

void Image::guessFormat(ImageFormatProperties& result, cv::UMat m) {
	guessFormat(result, m.channels(), m.depth(), m.type());
}

void Image::guessFormat(ImageFormatProperties& result, IplImage* m) {
	// @todo can we extract the CvMat Type based on the information in IplImage??
	guessFormat(result, m->nChannels, IPL2CV_DEPTH(m->depth));
	// check m->channelSeq for RGB/BGR
}

void Image::getFormatProperties(ImageFormatProperties& result) const {
	result.imageFormat = pixelFormat();
	result.bitsPerPixel = bitsPerPixel();
	result.channels = channels();
	result.depth = depth();
	result.origin = origin();
	result.matType = CV_MAKE_TYPE(depth(), channels());
	// alignment is not stored in Image ..
}

void Image::setFormatProperties(ImageFormatProperties& fmt, unsigned char mask){
	if (mask & IMAGE_FORMAT) {
		m_format = fmt.imageFormat;
	}
	if (mask & IMAGE_DEPTH) {
		m_depth = fmt.depth;
	}
	if (mask & IMAGE_CHANNELS) {
		m_channels = fmt.channels;
	}
	if (mask & IMAGE_BITSPERPIXEL) {
		m_bitsPerPixel = fmt.bitsPerPixel;
	}
	if (mask & IMAGE_ORIGIN) {
		m_origin = fmt.origin;
	}
}

void Image::copyImageFormatFrom(const Image& img, unsigned char mask) {
	ImageFormatProperties fmt;
	img.getFormatProperties(fmt);
	setFormatProperties(fmt, mask);
}


int Image::cvMatType(void) const {
	if (m_uploadState == OnCPUGPU || m_uploadState == OnGPU) {
		return m_gpuImage.type();
	}
	else {
		return m_cpuImage.type();
	}
}



// @todo nChannels and nDepth are ignored .. should be removed.
Image::Ptr Image::CvtColor( int nCode, int nChannels, int nDepth ) const
{
	ImageFormatProperties fmt;
	getFormatProperties(fmt);

	switch(nCode) {
	case CV_BGR2GRAY:
	case CV_RGB2GRAY:
	case CV_BGRA2GRAY:
	case CV_RGBA2GRAY:
		fmt.imageFormat = LUMINANCE;
		fmt.bitsPerPixel = fmt.bitsPerPixel / fmt.channels;
		fmt.channels = 1;
		break;
	case CV_GRAY2RGB: // also matches GRAY2BGR
		fmt.imageFormat = RGB;
		fmt.bitsPerPixel = fmt.bitsPerPixel * 3;
		fmt.channels = 3;
		break;
	case CV_GRAY2RGBA: // also matches GRAY2BGRA
		fmt.imageFormat = RGBA;
		fmt.bitsPerPixel = fmt.bitsPerPixel * 4;
		fmt.channels = 4;
		break;
	default:
		LOG4CPP_WARN(imageLogger, "Unknown Image Transformation.");
	}

	Image::Ptr r;
	if (m_uploadState == OnCPUGPU || m_uploadState == OnGPU) {
		cv::UMat mat;
		cv::cvtColor( m_gpuImage, mat, nCode );
		r.reset( new Image( mat, fmt ) );
	} else {
		cv::Mat mat;
		cv::cvtColor( m_cpuImage, mat, nCode );
		r.reset(new Image( mat, fmt ) );
	}
	return r;
}



Image::Ptr Image::AllocateNew() const
{
	// need imageFormat
	// should generate CPU or GPU image depending on current instance state
	ImageFormatProperties fmt;
	getFormatProperties(fmt);
    return ImagePtr( new Image( width(), height(), fmt) );
}

Image::Ptr Image::Clone() const
{
	ImageFormatProperties fmt;
	getFormatProperties(fmt);
	if (isOnGPU())
	{
#ifdef ENABLE_EVENT_TRACING
        TRACEPOINT_VISION_ALLOCATE_GPU(m_width*m_height*m_channels)
#endif
		cv::UMat m = m_gpuImage.clone();
		Ptr ptr = Image::Ptr(new Image( m, fmt ));
		return ptr;
	} else {
#ifdef ENABLE_EVENT_TRACING
        TRACEPOINT_VISION_ALLOCATE_CPU(m_width*m_height*m_channels)
#endif
		cv::Mat m = m_cpuImage.clone();
		Ptr ptr = Image::Ptr(new Image( m, fmt ));
		return ptr;
	}
	
}


Image::Ptr Image::PyrDown()
{
	ImageFormatProperties fmt;
	getFormatProperties(fmt);
	Image::Ptr r;
	if (isOnGPU()) {
		cv::UMat mat;
		cv::pyrDown( m_gpuImage, mat, cv::Size( width() / 2, height() / 2 ) );
		r.reset( new Image( mat, fmt ) );
	} else {
		cv::Mat mat;
		cv::pyrDown( m_cpuImage, mat, cv::Size( width() / 2, height() / 2 ) );
		r.reset( new Image( mat, fmt ) );
	}
	return r;
}


Image::Ptr Image::Scale( int width, int height )
{
	ImageFormatProperties fmt;
	getFormatProperties(fmt);
	Image::Ptr r;
	if (isOnGPU()) {
		cv::UMat mat;
		cv::resize( m_gpuImage, mat, cv::Size(width, height) );
		r.reset( new Image( mat, fmt ) );
	} else {
		cv::Mat mat;
		cv::resize( m_cpuImage, mat, cv::Size(width, height) );
		r.reset( new Image( mat, fmt ) );
	}
	return r;
}

/** creates an image with the given scale factor 0.0 < f <= 1.0 */
Image::Ptr Image::Scale( double scale )
{
    if (scale <= 0.0 || scale > 1.0){
		std::cout << "invalid scale factor" << std::endl;
		// RAISE Exception here ??
	}
    return Scale( static_cast< int >( width() * scale ), static_cast< int > ( height() * scale ) );
}

bool Image::isGrayscale() const
{
	// use imageFormat here
    return m_format == LUMINANCE;
}

Image::Ptr Image::getGrayscale( void ) const
{
    if ( !isGrayscale() ) {
        return CvtColor( CV_RGB2GRAY, 1, depth());
    } else {
        return Clone();
    }
}


// @todo adjusting contrast/brightness should be a done in a visioncomponent
// This function is copied from http://mehrez.kristou.org/opencv-change-contrast-and-brightness-of-an-image/
//Image::Ptr Image::ContrastBrightness( int contrast, int brightness )
//{
//	// this method is only implemented on CPU!!
//	checkOnCPU();
//
//	if(contrast > 100) contrast = 100;
//	if(contrast < -100) contrast = -100;
//	if(brightness > 100) brightness = 100;
//	if(brightness < -100) brightness = -100;
//
//	uchar lut[256];
//
//	CvMat* lut_mat;
//	int hist_size = 256;
//	float range_0[]={0,256};
//	float* ranges[] = { range_0 };
//	int i;
//
//	IplImage * dest = cvCloneImage(this->m_cpuImage);
//	
//	IplImage * GRAY;
//	if (this->channels() == 3)
//	{
//		//GRAY = cvCreateImage(cvGetSize(this),this->depth,1);
//		GRAY = cvCreateImage(cvGetSize(m_cpuImage), this->depth(), 1);
//		cvCvtColor(m_cpuImage,GRAY,CV_RGB2GRAY);
//	}
//	else
//	{
//		GRAY = cvCloneImage(m_cpuImage);
//	}
//    lut_mat = cvCreateMatHeader( 1, 256, CV_8UC1 );
//    cvSetData( lut_mat, lut, 0 );
//	/*
//     * The algorithm is by Werner D. Streidt
//     * (http://visca.com/ffactory/archives/5-99/msg00021.html)
//     */
//	if( contrast > 0 )
//    {
//        double delta = 127.* contrast/100;
//        double a = 255./(255. - delta*2);
//        double b = a*(brightness - delta);
//        for( i = 0; i < 256; i++ )
//        {
//            int v = cvRound(a*i + b);
//
//            if( v < 0 )
//                v = 0;
//            if( v > 255 )
//                v = 255;
//            lut[i] = v;
//        }
//    }
//    else
//    {
//        double delta = -128.* contrast/100;
//        double a = (256.-delta*2)/255.;
//        double b = a* brightness + delta;
//        for( i = 0; i < 256; i++ )
//        {
//            int v = cvRound(a*i + b);
//            if( v < 0 )
//                v = 0;
//
//            if( v > 255 )
//                v = 255;
//            lut[i] = v;
//        }
//    }
//	if (this->channels() ==3)
//	{
//		IplImage * R = cvCreateImage(cvGetSize(m_cpuImage),this->depth(),1);
//		IplImage * G = cvCreateImage(cvGetSize(m_cpuImage),this->depth(),1);
//		IplImage * B = cvCreateImage(cvGetSize(m_cpuImage),this->depth(),1);
//		cvSplit(m_cpuImage,R,G,B,NULL);
//		cvLUT( R, R, lut_mat );
//		cvLUT( G, G, lut_mat );
//		cvLUT( B, B, lut_mat );
//		cvMerge(R,G,B,NULL,dest);
//		cvReleaseImage(&R);
//		cvReleaseImage(&G);
//		cvReleaseImage(&B);
//	}
//	else
//	{
//		cvLUT( GRAY, dest, lut_mat );
//	}
//	cvReleaseImage(&GRAY);
//	cvReleaseMat( &lut_mat);
//	
//	return Image::Ptr( new Image( dest, true ) );
//}


void Image::saveAsJpeg( const std::string filename, int compressionFactor ) const
{
    std::vector< int > params;
    compressionFactor = std::min( 100, std::max ( 0, compressionFactor ) );
    params.push_back( CV_IMWRITE_JPEG_QUALITY );
    params.push_back( compressionFactor );
	if (isOnGPU()) {
		cv::imwrite( filename, m_gpuImage, params );
	} else {
		cv::imwrite( filename, m_cpuImage, params );
	}
}


void Image::encodeAsJpeg( std::vector< uchar >& buffer, int compressionFactor ) const
{
    std::vector< int > params;
    compressionFactor = std::min( 100, std::max ( 0, compressionFactor ) );
    params.push_back( CV_IMWRITE_JPEG_QUALITY );
    params.push_back( compressionFactor );
	if (isOnGPU()) {
		cv::imencode( ".jpg", m_gpuImage, buffer, params );
	} else {
		cv::imencode( ".jpg", m_cpuImage, buffer, params );
	}
}

void Image::checkOnGPU()
{
	if(m_uploadState == OnCPU){
#ifdef ENABLE_EVENT_TRACING
		TRACEPOINT_VISION_GPU_UPLOAD(width()*height()*channels())
#endif
        // how about being more explicit by specifiying:
		// cv::ACCESS_READ, cv::USAGE_ALLOCATE_DEVICE_MEMORY
		// cv::ACCESS_WRITE, cv::USAGE_ALLOCATE_DEVICE_MEMORY
		m_gpuImage = m_cpuImage.getUMat(0);
		m_uploadState = OnCPUGPU;
	}
}

void Image::checkOnCPU()
{
	if(m_uploadState == OnGPU){
#ifdef ENABLE_EVENT_TRACING
		TRACEPOINT_VISION_GPU_DOWNLOAD(width()*height()*channels())
#endif
		m_cpuImage = m_gpuImage.getMat(0);
		m_uploadState = OnCPUGPU;
	}
}

} } // namespace Ubitrack::Vision
