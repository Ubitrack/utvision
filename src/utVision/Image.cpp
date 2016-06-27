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


//#define ALLOCATION_LOGGING
//#define UPLOAD_DOWNLOAD_LOGGING

#ifdef ALLOCATION_LOGGING
static int imageNumber = 0;
static int allocCounter = 0;
#endif


namespace Ubitrack { namespace Vision {


static int imageNumber = 0;


Image::Image( int nWidth, int nHeight, int nChannels, void* pImageData, int nDepth, int nOrigin, int nAlign )
	: m_bOwned( false )
	, m_cpuIplImage(cvCreateImageHeader( cvSize( nWidth, nHeight ), nDepth, nChannels ))
	, m_debugImageId(imageNumber)
	//, m_uploadState(OnCPUGPU)
	, m_uploadState(OnCPU)
{
	
#ifdef ALLOCATION_LOGGING
	imageNumber++;
	allocCounter++;
	LOG4CPP_INFO( imageLogger, "init1" << " imageNumber" << m_debugImageId);
#endif
	m_cpuIplImage->origin = nOrigin;
	m_cpuIplImage->align = nAlign;
	m_cpuIplImage->imageData = static_cast< char* >( pImageData );

	//m_uMat = cv::UMat(height(), width(), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(IPL2CV_DEPTH(depth()), channels()));

	//m_uMat = cv::cvarrToMat(m_cpuIplImage, true).getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
}


Image::Image( int nWidth, int nHeight, int nChannels, int nDepth, int nOrigin )
	: m_bOwned( true )
	, m_cpuIplImage(cvCreateImage(cvSize(nWidth, nHeight), nDepth, nChannels))
	, m_debugImageId(imageNumber)
	, m_uploadState(OnCPUGPU)
	//, m_uMat(cv::USAGE_ALLOCATE_DEVICE_MEMORY)
{
#ifdef ALLOCATION_LOGGING
	imageNumber++;
	allocCounter++;
	LOG4CPP_INFO( imageLogger,   "init2" << " imageNumber" << m_debugImageId);
#endif
	m_cpuIplImage->origin = nOrigin;
	m_uMat = cv::UMat(height(), width(), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(IPL2CV_DEPTH(depth()), channels()));
}


Image::Image( IplImage* pIplImage, bool bDestroy )
	: m_bOwned( bDestroy )
	, m_cpuIplImage(cvCreateImageHeader(cvGetSize(pIplImage), pIplImage->depth, pIplImage->nChannels))
	, m_debugImageId(imageNumber)
	//, m_uploadState(OnCPUGPU)
	, m_uploadState(OnCPU)
{
	m_cpuIplImage->origin = pIplImage->origin;
#ifdef ALLOCATION_LOGGING
	imageNumber++;
	allocCounter++;
	LOG4CPP_INFO( imageLogger, "init3" << " imageNumber" << m_debugImageId << "b_ownd " << m_bOwned);
#endif
	m_cpuIplImage->imageData = pIplImage->imageData;

	//m_uMat = cv::UMat(height(), width(), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(IPL2CV_DEPTH(depth()), channels()));
	
	//m_uMat = cv::cvarrToMat(m_cpuIplImage, true).getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
	if ( bDestroy )
		cvReleaseImageHeader( &pIplImage );
}

Image::Image(cv::UMat & img)
	: m_bOwned( true )
	, m_uploadState( OnGPU )
	, m_debugImageId(imageNumber)
{
#ifdef ALLOCATION_LOGGING
	imageNumber++;
	allocCounter++;
	LOG4CPP_INFO(imageLogger, "init4" << " imageNumber" << m_debugImageId);
#endif
	m_uMat = img;
	//IplImage iplImage = m_uMat.getMat(0);
	////m_cpuIplImage = cvCloneImage(&iplImage);
	//m_cpuIplImage = cvCreateImage(cvSize(iplImage.width, iplImage.height), iplImage.depth, iplImage.nChannels);
	//cvCopy(&iplImage, m_cpuIplImage);
}

Image::Image( cv::Mat & img )
	: m_bOwned( false )
	, m_cpuIplImage(new IplImage(img))
	, m_debugImageId(imageNumber)
	//, m_uploadState(OnCPUGPU)
	, m_uploadState(OnCPU)
{
#ifdef ALLOCATION_LOGGING
	imageNumber++;
	allocCounter++;
	LOG4CPP_INFO( imageLogger, "init4" << " imageNumber" << m_debugImageId );
#endif
	//m_uMat = img.getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
}


Image::~Image()
{
	//destroy it
#ifdef ALLOCATION_LOGGING
	allocCounter--;
	LOG4CPP_INFO( imageLogger,  "destroy: imageNumber" << m_debugImageId << "owned: " <<m_bOwned << " uploadstate: "<< m_uploadState << "left: " << allocCounter);
#endif
	if(m_bOwned){
		IplImage* img = m_cpuIplImage;
		cvReleaseImage(&img);
	}
}


boost::shared_ptr< Image > Image::CvtColor( int nCode, int nChannels, int nDepth ) const
{
	// @todo the current image class is a kind of a mess really .. 
	// origin, imgFormat, pixelType should be annotations to the image class and only used from there
	// data storage should be managed in iplImage or uMat 
	boost::shared_ptr< Image > r( new Image( width(), height(), nChannels, nDepth ) );
	if (m_uploadState == OnCPUGPU || m_uploadState == OnGPU) {
		cv::cvtColor( m_uMat, r->uMat(), nCode );
		// how does origin or channelSeq translate to uMat's ??
		//r->m_cpuIplImage->origin = origin();
	} else {
		cvCvtColor( m_cpuIplImage, *r, nCode );
		r->m_cpuIplImage->origin = origin();		
	}
	return r;
}


void Image::Invert()
{
	if ( channels() != 1 || depth() != IPL_DEPTH_8U )
		std::cout << "Operation only supported on 8-Bit grayscale images" << std::endl;

	for ( int i=0; i < width(); i++ )
	{
		for ( int j=0; j < height(); j++ )
		{
			unsigned char val = 255 - getPixel< unsigned char >( i, j );
			setPixel< unsigned char >( i, j, val );
		}
	}
}

boost::shared_ptr< Image > Image::AllocateNew() const
{
    return ImagePtr( new Image( width(), height(), channels(), depth(), origin()) );
}

boost::shared_ptr< Image > Image::Clone() const
{
	if (m_uploadState == OnCPU)
	{
		return boost::shared_ptr< Image >(new Image(cvCloneImage(this->m_cpuIplImage)));
	}
	else{
		cv::UMat m = this->m_uMat.clone();
        return boost::shared_ptr< Image >(new Image( m ));		
	}
	
}


boost::shared_ptr< Image > Image::PyrDown()
{
	boost::shared_ptr< Image > r( new Image( width() / 2, height() / 2, channels(), depth(), origin() ) );
	cvPyrDown( m_cpuIplImage, *r );
	return r;
}


boost::shared_ptr< Image > Image::Scale( int width, int height )
{
	boost::shared_ptr< Image > scaledImg( new Image( width, height, channels(), depth(), origin() ) );
	cvResize( m_cpuIplImage, *scaledImg );
	return scaledImg;
}

/** creates an image with the given scale factor 0.0 < f <= 1.0 */
boost::shared_ptr< Image > Image::Scale( double scale )
{
    if (scale <= 0.0 || scale > 1.0){
		std::cout << "invalided scale factor" << std::endl;
	}
    return Scale( static_cast< int >( width() * scale ), static_cast< int > ( height() * scale ) );
}

bool Image::isGrayscale() const
{
    return channels() == 1;
}

Image::Ptr Image::getGrayscale( void ) const
{
    if ( !isGrayscale() ) {
        // TODO: Has the image, if not grayscale, really three channels then?
        return CvtColor( CV_RGB2GRAY, 1, depth());
    } else {
        return Clone();
    }
}


// This function is copied from http://mehrez.kristou.org/opencv-change-contrast-and-brightness-of-an-image/
boost::shared_ptr< Image > Image::ContrastBrightness( int contrast, int brightness )
{
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

	IplImage * dest = cvCloneImage(this->m_cpuIplImage);
	
	IplImage * GRAY;
	if (this->channels() == 3)
	{
		//GRAY = cvCreateImage(cvGetSize(this),this->depth,1);
		GRAY = cvCreateImage(cvGetSize(m_cpuIplImage), this->depth(), 1);
		cvCvtColor(m_cpuIplImage,GRAY,CV_RGB2GRAY);
	}
	else
	{
		GRAY = cvCloneImage(m_cpuIplImage);
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
		IplImage * R = cvCreateImage(cvGetSize(m_cpuIplImage),this->depth(),1);
		IplImage * G = cvCreateImage(cvGetSize(m_cpuIplImage),this->depth(),1);
		IplImage * B = cvCreateImage(cvGetSize(m_cpuIplImage),this->depth(),1);
		cvSplit(m_cpuIplImage,R,G,B,NULL);
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
    cv::imwrite( filename, m_uMat, params );
}


void Image::encodeAsJpeg( std::vector< uchar >& buffer, int compressionFactor ) const
{
    std::vector< int > params;
    compressionFactor = std::min( 100, std::max ( 0, compressionFactor ) );
    params.push_back( CV_IMWRITE_JPEG_QUALITY );
    params.push_back( compressionFactor );
    cv::imencode( ".jpg", m_uMat, buffer, params );
}

void Image::checkOnGPU()
{
#ifdef UPLOAD_DOWNLOAD_LOGGING
	LOG4CPP_INFO( imageLogger, "checkOnGPU: " << m_debugImageId<< " state: " <<m_uploadState);
#endif
	if(m_uploadState == OnCPU){
		m_uMat = cv::cvarrToMat(m_cpuIplImage, true).getUMat(cv::ACCESS_RW, cv::USAGE_ALLOCATE_DEVICE_MEMORY);
		//m_uMat = cv::UMat(height(), width(), cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(IPL2CV_DEPTH(depth()), channels()));
#ifdef UPLOAD_DOWNLOAD_LOGGING
		LOG4CPP_INFO( imageLogger, "uploading " << m_debugImageId);
#endif
	}
	m_uploadState = OnGPU;
}

void Image::checkOnCPU()
{
#ifdef UPLOAD_DOWNLOAD_LOGGING
	LOG4CPP_INFO( imageLogger, "checkOnCPU: " << m_debugImageId<< " state: " << m_uploadState);
#endif
	if(m_uploadState == OnGPU){
		IplImage iplImage = m_uMat.getMat(0);
		cvCopy(&iplImage, m_cpuIplImage);
#ifdef UPLOAD_DOWNLOAD_LOGGING
		LOG4CPP_INFO( imageLogger, "downloading " << m_debugImageId);
#endif
	}
	m_uploadState = OnCPU;
}

bool Image::isOnGPU() {
	return m_uploadState == OnCPUGPU || m_uploadState == OnGPU;
}

} } // namespace Ubitrack::Vision
