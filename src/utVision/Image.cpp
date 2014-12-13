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

namespace Ubitrack { namespace Vision {


Image::Image( int nWidth, int nHeight, int nChannels, void* pImageData, int nDepth, int nOrigin, int nAlign )
	: m_bOwned( false )
{
	//cvInitImageHeader( this, cvSize( nWidth, nHeight ), nDepth, nChannels, nOrigin, nAlign );
	//imageData = static_cast< char* >( pImageData );

	// TODO: überprüfe
	
	cvInitImageHeader( this->m_cpuIplImage.get(), cvSize( nWidth, nHeight ), nDepth, nChannels, nOrigin, nAlign );
	m_cpuIplImage->imageData = static_cast< char* >( pImageData );
	*m_cpuMat = cv::cvarrToMat(m_cpuIplImage.get(), false);
}


Image::Image( int nWidth, int nHeight, int nChannels, int nDepth, int nOrigin )
	: m_bOwned( true )
{
	cvInitImageHeader( this->m_cpuIplImage.get(), cvSize( nWidth, nHeight ), nDepth, nChannels, nOrigin );
	
	// TODO: memory allocation/deallocation ok with that?
	m_cpuIplImage->imageDataOrigin = m_cpuIplImage->imageData = static_cast< char* >( cvAlloc( imageSize ) );
	*m_cpuMat = cv::cvarrToMat(m_cpuIplImage.get(), false);
}


Image::Image( IplImage* pIplImage, bool bDestroy )
	: m_bOwned( bDestroy )
{
	memcpy( m_cpuIplImage.get(), pIplImage, sizeof( IplImage ) );
	*m_cpuMat = cv::cvarrToMat(m_cpuIplImage.get(), false);
	if ( bDestroy )
		cvReleaseImageHeader( &pIplImage );
}

Image::Image( cv::Mat & img )
	: m_bOwned( false )
{
	//IplImage imgIpl = img;
	//memcpy( static_cast< IplImage* >( this ), &imgIpl, sizeof( IplImage ) );
	*m_cpuMat = img;
	*m_cpuIplImage = img;
}


Image::~Image()
{
	
	if ( m_cpuMat )
		m_cpuMat->release();

	if ( m_bOwned )
		cvFree( reinterpret_cast< void** >( &imageDataOrigin ) );

	if ( m_gpuMat )
		m_gpuMat->release();

}


boost::shared_ptr< Image > Image::CvtColor( int nCode, int nChannels, int nDepth )
{
	checkCPUMat();
	boost::shared_ptr< Image > r( new Image( width(), height(), nChannels, nDepth, m_cpuIplImage->origin ) );
	cvCvtColor( this, *r, nCode ); 
	return r;
}


void Image::Invert()
{
	if ( channels() != 1 || depth() != IPL_DEPTH_8U )
		UBITRACK_THROW ("Operation only supported on 8-Bit grayscale images");
	cv::UMat mat;

	cvCvtColor(&mat, &mat, 1);
	
	for ( int i=0; i < width(); i++ )
	{
		for ( int j=0; j < height(); j++ )
		{
			unsigned char val = 255 - getPixel< unsigned char >( i, j );
			setPixel< unsigned char >( i, j, val );
		}
	}
}

boost::shared_ptr< Image > Image::AllocateNew()
{
    return ImagePtr( new Image( width(), height(), channels(), depth(), origin()) );
}

boost::shared_ptr< Image > Image::Clone()
{
	return boost::shared_ptr< Image >( new Image( cvCloneImage( this->m_cpuIplImage.get() ) ) );
}


boost::shared_ptr< Image > Image::PyrDown()
{
	checkCPUMat();
	boost::shared_ptr< Image > r( new Image( width() / 2, height() / 2, channels(), depth(), origin() ) );
	cvPyrDown( m_cpuIplImage.get(), *r );
	return r;
}


boost::shared_ptr< Image > Image::Scale( int width, int height )
{
	checkCPUMat();
	boost::shared_ptr< Image > scaledImg( new Image( width, height, channels(), depth(), origin() ) );
	cvResize( m_cpuIplImage.get(), *scaledImg );
	return scaledImg;
}

/** creates an image with the given scale factor 0.0 < f <= 1.0 */
boost::shared_ptr< Image > Image::Scale( double scale )
{
    if (scale <= 0.0 || scale > 1.0)
        UBITRACK_THROW( "Invalid scale factor" );
    return Scale( static_cast< int >( width() * scale ), static_cast< int > ( height() * scale ) );
}

bool Image::isGrayscale() const
{
    return channels() == 1;
}

Image::Ptr Image::getGrayscale( void )
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
	checkCPUMat();
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

	IplImage * dest = cvCloneImage(this->m_cpuIplImage.get());
	
	IplImage * GRAY;
	if (this->channels() == 3)
	{
		//GRAY = cvCreateImage(cvGetSize(this),this->depth,1);
		GRAY = cvCreateImage(cvGetSize(m_cpuIplImage.get()), this->depth(), 1);
		cvCvtColor(m_cpuIplImage.get(),GRAY,CV_RGB2GRAY);
	}
	else
	{
		GRAY = cvCloneImage(m_cpuIplImage.get());
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
		IplImage * R = cvCreateImage(cvGetSize(m_cpuIplImage.get()),this->depth(),1);
		IplImage * G = cvCreateImage(cvGetSize(m_cpuIplImage.get()),this->depth(),1);
		IplImage * B = cvCreateImage(cvGetSize(m_cpuIplImage.get()),this->depth(),1);
		cvSplit(this,R,G,B,NULL);
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
    cv::imwrite( filename, cv::Mat( cv::cvarrToMat(*this) ), params );
}


void Image::encodeAsJpeg( std::vector< uchar >& buffer, int compressionFactor ) const
{
    std::vector< int > params;
    compressionFactor = std::min( 100, std::max ( 0, compressionFactor ) );
    params.push_back( CV_IMWRITE_JPEG_QUALITY );
    params.push_back( compressionFactor );
    cv::imencode( ".jpg", cv::Mat( cv::cvarrToMat(*this) ), buffer, params );
}

void Image::checkCPUMat( ) {
	if( m_cpuMat != nullptr && m_cpuIplImage != nullptr )
		return;
	m_gpuMat->download( *m_cpuMat );
	*m_cpuIplImage = *m_cpuMat;
}

void Image::checkGPUMat( ) {
	if( m_gpuMat != nullptr )
		return;
	m_gpuMat->upload( *m_cpuMat );
}

} } // namespace Ubitrack::Vision
