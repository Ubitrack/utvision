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

#ifdef ENABLE_EVENT_TRACING
#include <utUtil/TracingProvider.h>
#endif


namespace Ubitrack { namespace Vision {



Image::Image( int nWidth, int nHeight, int nChannels, void* pImageData, int nDepth, int nOrigin, int nAlign )
	: m_bOwned( false )
	, m_cpuImage(0)
	, m_gpuImage(0)
	, m_uploadState(OnCPU)
	, m_width(nWidth)
	, m_height(nHeight)
	, m_channels(nChannels)
	, m_bitsPerPixel(nDepth)
	, m_origin(nOrigin)
{
	m_cpuImage = cvCreateImageHeader( cvSize( nWidth, nHeight ), nDepth, nChannels );
	m_cpuImage->origin = nOrigin;
	// opencv documentation says that this attribute is ignored in favor to widthStep
	// therefore this should be removed.
	m_cpuImage->align = nAlign;
	m_cpuImage->imageData = static_cast< char* >( pImageData );
}


Image::Image( int nWidth, int nHeight, int nChannels, int nDepth, int nOrigin, ImageUploadState nState)
	: m_bOwned( true )
	, m_cpuImage(0)
	, m_gpuImage(0)
	, m_uploadState(nState)
	, m_width(nWidth)
	, m_height(nHeight)
	, m_channels(nChannels)
	, m_bitsPerPixel(nDepth)
	, m_origin(nOrigin)
{

	if (isOnGPU()) {

#ifdef ENABLE_EVENT_TRACING
		#ifdef HAVE_DTRACE
			if (UBITRACK_VISION_ALLOCATE_GPU_ENABLED()) {
				UBITRACK_VISION_ALLOCATE_GPU(nWidth*nHeight*nChannels);
			}
#endif
#ifdef HAVE_ETW
			ETWUbitrackAllocateGpu(nWidth*nHeight*nChannels);
#endif
#endif
		m_gpuImage = new cv::UMat(height(), width(), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(IPL2CV_DEPTH(depth()), channels()));
	}

	if (isOnCPU()) {

#ifdef ENABLE_EVENT_TRACING
		#ifdef HAVE_DTRACE
			if (UBITRACK_VISION_ALLOCATE_CPU_ENABLED()) {
				UBITRACK_VISION_ALLOCATE_CPU(nWidth*nHeight*nChannels);
			}
#endif
#ifdef HAVE_ETW
			ETWUbitrackAllocateCpu(nWidth*nHeight*nChannels);
#endif
#endif
		m_cpuImage = cvCreateImage(cvSize(nWidth, nHeight), nDepth, nChannels);
		m_cpuImage->origin = nOrigin;
	}
}


Image::Image( IplImage* pIplImage, bool bDestroy )
	: m_bOwned( bDestroy )
	, m_cpuImage(0)
	, m_gpuImage(0)
	, m_uploadState(OnCPU)
	, m_width(pIplImage->width)
	, m_height(pIplImage->height)
	, m_channels(pIplImage->nChannels)
	, m_bitsPerPixel(pIplImage->depth)
	, m_origin(pIplImage->origin)
{
	m_cpuImage = cvCreateImageHeader(cvGetSize(pIplImage), pIplImage->depth, pIplImage->nChannels);
	m_cpuImage->origin = pIplImage->origin;
	m_cpuImage->imageData = pIplImage->imageData;
	if ( bDestroy )
		cvReleaseImageHeader( &pIplImage );
}

Image::Image(cv::UMat & img)
	: m_bOwned( true )
	, m_cpuImage(0)
	, m_gpuImage(0)
	, m_uploadState( OnGPU )
	, m_width(img.cols)
	, m_height(img.rows)
	, m_bitsPerPixel(img.depth())
	, m_origin(0)
{
	if (img.dims == 2) {
		m_channels = 1;
	} else if (img.dims == 3) {
		m_channels = img.size[2];
	} else {
		// ERROR dimensionality too high
	}

#ifdef ENABLE_EVENT_TRACING
	#ifdef HAVE_DTRACE
			if (UBITRACK_VISION_ALLOCATE_GPU_ENABLED()) {
				UBITRACK_VISION_ALLOCATE_GPU(nWidth*nHeight*nChannels);
			}
#endif
#ifdef HAVE_ETW
			ETWUbitrackAllocateGpu(nWidth*nHeight*nChannels);
#endif
#endif
	m_gpuImage = new cv::UMat(img);
}

Image::Image( cv::Mat & img )
	: m_bOwned( true )
	, m_cpuImage(0)
	, m_gpuImage(0)
	, m_uploadState(OnCPU)
{

#ifdef ENABLE_EVENT_TRACING
	#ifdef HAVE_DTRACE
			if (UBITRACK_VISION_ALLOCATE_CPU_ENABLED()) {
				UBITRACK_VISION_ALLOCATE_CPU(nWidth*nHeight*nChannels);
			}
#endif
#ifdef HAVE_ETW
			ETWUbitrackAllocateCpu(nWidth*nHeight*nChannels);
#endif
#endif
	IplImage* img_ptr = new IplImage(img);

	m_width = img_ptr->width;
	m_height = img_ptr->height;
	m_channels = img_ptr->nChannels;
	m_bitsPerPixel = img_ptr->depth;
	m_origin = img_ptr->origin;

	m_cpuImage = img_ptr;
}


Image::~Image()
{
	//destroy the buffers
	if (m_cpuImage) {
		if(m_bOwned) {
			IplImage* img = m_cpuImage;
			cvReleaseImage(&img);
		} else {
			IplImage* img = m_cpuImage;
			cvReleaseImageHeader( &img );
		}
	}
	if (m_gpuImage) {
		delete m_gpuImage;
	}
}


boost::shared_ptr< Image > Image::CvtColor( int nCode, int nChannels, int nDepth ) const
{
	// @todo the current image class is a kind of a mess really .. 
	// origin, imgFormat, pixelType should be annotations to the image class and only used from there
	// data storage should be managed in iplImage or uMat 
	boost::shared_ptr< Image > r( new Image( width(), height(), nChannels, nDepth ) );
	if (m_uploadState == OnCPUGPU || m_uploadState == OnGPU) {
		cv::cvtColor( *m_gpuImage, r->uMat(), nCode );
		// how does origin or channelSeq translate to uMat's ??
		//r->m_cpuImage->origin = origin();
	} else {
		cvCvtColor( m_cpuImage, *r, nCode );
		r->m_cpuImage->origin = origin();
	}
	return r;
}



boost::shared_ptr< Image > Image::AllocateNew() const
{
	// need imageFormat
    return ImagePtr( new Image( width(), height(), channels(), depth(), origin()) );
}

boost::shared_ptr< Image > Image::Clone() const
{
	if (isOnGPU())
	{
		// need imageFormat, ... as parameters to constructor
		cv::UMat m = this->m_gpuImage->clone();
		return boost::shared_ptr< Image >(new Image( m ));
	} else {
		// need imageFormat, ... as parameters to constructor
		cv::Mat m = cv::cvarrToMat(m_cpuImage, true).clone();
		return boost::shared_ptr< Image >(new Image( m ));
	}
	
}


boost::shared_ptr< Image > Image::PyrDown()
{
	// this method is only implemented on CPU!!
	checkOnCPU();

	boost::shared_ptr< Image > r( new Image( width() / 2, height() / 2, channels(), depth(), origin() ) );
	cvPyrDown( m_cpuImage, *r );
	return r;
}


boost::shared_ptr< Image > Image::Scale( int width, int height )
{
	// this method is only implemented on CPU!!
	checkOnCPU();

	boost::shared_ptr< Image > scaledImg( new Image( width, height, channels(), depth(), origin() ) );
	cvResize( m_cpuImage, *scaledImg );
	return scaledImg;
}

/** creates an image with the given scale factor 0.0 < f <= 1.0 */
boost::shared_ptr< Image > Image::Scale( double scale )
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
    return channels() == 1;
}

Image::Ptr Image::getGrayscale( void ) const
{
    if ( !isGrayscale() ) {
        // TODO: Has the image, if not grayscale, really three channels then and is RGB ?
        return CvtColor( CV_RGB2GRAY, 1, depth());
    } else {
        return Clone();
    }
}


// @todo adjusting contrast/brightness should be a done in a visioncomponent
// This function is copied from http://mehrez.kristou.org/opencv-change-contrast-and-brightness-of-an-image/
boost::shared_ptr< Image > Image::ContrastBrightness( int contrast, int brightness )
{
	// this method is only implemented on CPU!!
	checkOnCPU();

	if(contrast > 100) contrast = 100;
	if(contrast < -100) contrast = -100;
	if(brightness > 100) brightness = 100;
	if(brightness < -100) brightness = -100;

	uchar lut[256];

	CvMat* lut_mat;
	int hist_size = 256;
	float range_0[]={0,256};
	float* ranges[] = { range_0 };
	int i;

	IplImage * dest = cvCloneImage(this->m_cpuImage);
	
	IplImage * GRAY;
	if (this->channels() == 3)
	{
		//GRAY = cvCreateImage(cvGetSize(this),this->depth,1);
		GRAY = cvCreateImage(cvGetSize(m_cpuImage), this->depth(), 1);
		cvCvtColor(m_cpuImage,GRAY,CV_RGB2GRAY);
	}
	else
	{
		GRAY = cvCloneImage(m_cpuImage);
	}
    lut_mat = cvCreateMatHeader( 1, 256, CV_8UC1 );
    cvSetData( lut_mat, lut, 0 );
	/*
     * The algorithm is by Werner D. Streidt
     * (http://visca.com/ffactory/archives/5-99/msg00021.html)
     */
	if( contrast > 0 )
    {
        double delta = 127.* contrast/100;
        double a = 255./(255. - delta*2);
        double b = a*(brightness - delta);
        for( i = 0; i < 256; i++ )
        {
            int v = cvRound(a*i + b);

            if( v < 0 )
                v = 0;
            if( v > 255 )
                v = 255;
            lut[i] = v;
        }
    }
    else
    {
        double delta = -128.* contrast/100;
        double a = (256.-delta*2)/255.;
        double b = a* brightness + delta;
        for( i = 0; i < 256; i++ )
        {
            int v = cvRound(a*i + b);
            if( v < 0 )
                v = 0;

            if( v > 255 )
                v = 255;
            lut[i] = v;
        }
    }
	if (this->channels() ==3)
	{
		IplImage * R = cvCreateImage(cvGetSize(m_cpuImage),this->depth(),1);
		IplImage * G = cvCreateImage(cvGetSize(m_cpuImage),this->depth(),1);
		IplImage * B = cvCreateImage(cvGetSize(m_cpuImage),this->depth(),1);
		cvSplit(m_cpuImage,R,G,B,NULL);
		cvLUT( R, R, lut_mat );
		cvLUT( G, G, lut_mat );
		cvLUT( B, B, lut_mat );
		cvMerge(R,G,B,NULL,dest);
		cvReleaseImage(&R);
		cvReleaseImage(&G);
		cvReleaseImage(&B);
	}
	else
	{
		cvLUT( GRAY, dest, lut_mat );
	}
	cvReleaseImage(&GRAY);
	cvReleaseMat( &lut_mat);
	
	return boost::shared_ptr< Image >( new Image( dest, true ) );
}


void Image::saveAsJpeg( const std::string filename, int compressionFactor ) const
{
    std::vector< int > params;
    compressionFactor = std::min( 100, std::max ( 0, compressionFactor ) );
    params.push_back( CV_IMWRITE_JPEG_QUALITY );
    params.push_back( compressionFactor );
	if (m_uploadState == OnCPUGPU || m_uploadState == OnGPU) {
		cv::imwrite( filename, *m_gpuImage, params );
	} else {
		// @ todo how to write iplImages to disk ??
	}
}


void Image::encodeAsJpeg( std::vector< uchar >& buffer, int compressionFactor ) const
{
    std::vector< int > params;
    compressionFactor = std::min( 100, std::max ( 0, compressionFactor ) );
    params.push_back( CV_IMWRITE_JPEG_QUALITY );
    params.push_back( compressionFactor );
	if (m_uploadState == OnCPUGPU || m_uploadState == OnGPU) {
		cv::imencode( ".jpg", *m_gpuImage, buffer, params );
	} else {
		// @ todo how to encode iplImages ??
		//cv::imencode( ".jpg", m_cpuImage, buffer, params );
	}
}

void Image::checkOnGPU()
{
	if(m_uploadState == OnCPU){
#ifdef ENABLE_EVENT_TRACING
		#ifdef HAVE_DTRACE
			if (UBITRACK_VISION_GPU_UPLOAD_ENABLED()) {
				UBITRACK_VISION_GPU_UPLOAD(width()*height()*channels());
			}
#endif
#ifdef HAVE_ETW
			ETWUbitrackGpuUpload(width()*height()*channels());
#endif
#endif
		m_gpuImage = new cv::UMat();
		*m_gpuImage = cv::cvarrToMat(m_cpuImage, true).getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
		m_uploadState = OnCPUGPU;
	}
}

void Image::checkOnCPU()
{
	if(m_uploadState == OnGPU){
#ifdef ENABLE_EVENT_TRACING
		#ifdef HAVE_DTRACE
			if (UBITRACK_VISION_GPU_DOWNLOAD_ENABLED()) {
				UBITRACK_VISION_GPU_DOWNLOAD(width()*height()*channels());
			}
#endif
#ifdef HAVE_ETW
			ETWUbitrackGpuDownload(width()*height()*channels());
#endif
#endif
        m_cpuImage = cvCreateImage(cvSize(m_width, m_height), m_bitsPerPixel, m_channels);
		IplImage iplImage = m_gpuImage->getMat(0);
		cvCopy(&iplImage, m_cpuImage);
		m_uploadState = OnCPUGPU;
	}
}

} } // namespace Ubitrack::Vision
