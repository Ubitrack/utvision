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
 * A class for encapsulating the image undistortion logic
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 * @author Christian Waechter <christian.waechter@in.tum.de> (refactoring)
 */

#include "Undistortion.h"

// OpenCV
#include <opencv/cv.h>

// Ubitrack
#include "Image.h"
#include <utUtil/CalibFile.h>
#include <utUtil/Exception.h>
#include "Util/OpenCV.h"	// type conversion Ubitrack <-> OpenCV

// get a logger
#include <log4cpp/Category.hh>
static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Vision.Undistortion" ) );

namespace { 
	
	/// scales the intrinsic camera matrix parameters to used image resolution
	template< typename PrecisionType >	
	void inline correctForScale( const Ubitrack::Vision::Image& image, Ubitrack::Math::CameraIntrinsics< PrecisionType >& intrinsics )
	{
		// scale the intrinsic matrix up or down if image size is different ( does not work if image is cropped instead of scaled)
		const PrecisionType scaleX = image.width() / static_cast< PrecisionType > ( intrinsics.dimension( 0 ) );
		intrinsics.matrix ( 0, 0 ) *= scaleX;
		intrinsics.matrix ( 0, 2 ) *= scaleX;
		intrinsics.dimension ( 0 ) *= scaleX;
		
		const PrecisionType scaleY = image.height() / static_cast< PrecisionType > ( intrinsics.dimension( 1 ) );
		intrinsics.matrix ( 1, 1 ) *= scaleY;
		intrinsics.matrix ( 1, 2 ) *= scaleY;
		intrinsics.dimension ( 1 ) *= scaleY;
		
		intrinsics.reset(); // recalculate the inverse
	}

	/// corrects the Ubitrack intrinsics matrix to the corresponding (left-handed) OpenCV camera matrix
	template< typename PrecisionType >	
	void inline correctForOpenCV( Ubitrack::Math::CameraIntrinsics< PrecisionType >& intrinsics )
	{
		// compensate for left-handed OpenCV coordinate frame
		intrinsics.matrix ( 0, 2 ) *= -1;
		intrinsics.matrix ( 1, 2 ) *= -1;
		intrinsics.matrix ( 2, 2 ) *= -1;
		
		intrinsics.reset(); // recalculate the inverse
	}
	
	/// corrects the Ubitrack intrinsics parameters if the image is flipped upside-down
	template< typename PrecisionType >	
	void inline correctForOrigin( const Ubitrack::Vision::Image& image, Ubitrack::Math::CameraIntrinsics< PrecisionType >& intrinsics )
	{
		if ( image.origin() )
			return;
		
		{	// compensate if origin==0
			intrinsics.matrix( 1, 2 ) = image.height() - 1 - intrinsics.matrix( 1, 2 );
			intrinsics.tangential_params( 1 ) *= -1.0;
			
			intrinsics.reset(); // recalculate the inverse
		}
	}
	
	
}	// anonymous namespace

namespace Ubitrack { namespace Vision {
	
Undistortion::Undistortion(){};

Undistortion::Undistortion( const std::string& intrinsicMatrixFile, const std::string& distortionFile )
{
	reset( intrinsicMatrixFile, distortionFile );
}

Undistortion::Undistortion( const std::string& cameraIntrinsicsFile )
{
	reset( cameraIntrinsicsFile );
}

Undistortion::Undistortion( const intrinsics_type& intrinsics )
{
	reset( intrinsics );
}

void Undistortion::reset( const std::string& CameraIntrinsicsFile )
{
	Measurement::CameraIntrinsics measMat;
	measMat.reset( new intrinsics_type() );
	Ubitrack::Util::readCalibFile( CameraIntrinsicsFile, measMat );
	reset( *measMat );
}

void Undistortion::reset( const intrinsics_type& camIntrinsics )
{
	m_intrinsics = camIntrinsics;
	m_intrinsicMatrix = m_intrinsics.matrix;
	
	m_coeffs( 0 ) = m_intrinsics.radial_params ( 0 );
	m_coeffs( 1 ) = m_intrinsics.radial_params ( 1 );
	m_coeffs( 2 ) = m_intrinsics.tangential_params ( 0 );
	m_coeffs( 3 ) = m_intrinsics.tangential_params ( 1 );
	
	for( std::size_t i = 4; i < (m_intrinsics.radial_size+2); ++i )
		m_coeffs( i ) = m_intrinsics.radial_params( i-2 );
	
	
}

void Undistortion::reset( const std::string& intrinsicMatrixFile, const std::string& distortionFile )
{
	// read intrinsics
	if ( !intrinsicMatrixFile.empty() )
	{
		
		Measurement::Matrix3x3 measMat;
		measMat.reset( new Ubitrack::Math::Matrix< double, 3, 3 >() );
		Ubitrack::Util::readCalibFile( intrinsicMatrixFile, measMat );
		m_intrinsicMatrix = *measMat;
		LOG4CPP_DEBUG(logger, "Loaded calibration file : " << m_intrinsicMatrix);
	}
	else
	{
		m_intrinsicMatrix( 0, 0 ) = 400;
		m_intrinsicMatrix( 0, 1 ) = 0;
		m_intrinsicMatrix( 0, 2 ) = -160;
		m_intrinsicMatrix( 1, 0 ) = 0;
		m_intrinsicMatrix( 1, 1 ) = 400;
		m_intrinsicMatrix( 1, 2 ) = -120;
		m_intrinsicMatrix( 2, 0 ) = 0;
		m_intrinsicMatrix( 2, 1 ) = 0;
		m_intrinsicMatrix( 2, 2 ) = -1;
	}
	
	
	// initialize to zero 
	m_coeffs = Ubitrack::Math::Vector< double, 8 >::zeros();
	// read coefficients
	
	if ( !distortionFile.empty() )
	{
        // first try new, eight element distortion file
		Measurement::Vector8D measVec;
		measVec.reset( new Ubitrack::Math::Vector< double, 8 >() );
        try {
		    Ubitrack::Util::readCalibFile( distortionFile, measVec );
            m_coeffs = *measVec;
        }
		catch ( Ubitrack::Util::Exception )
		{
            LOG4CPP_ERROR( logger, "Cannot read new image distortion model. Trying old format." );
            // try old format, this time without exception handling
            Measurement::Vector4D measVec4D;
            measVec4D.reset( new Ubitrack::Math::Vector< double, 4 >() );
            Ubitrack::Util::readCalibFile( distortionFile, measVec4D );
            m_coeffs = Ubitrack::Math::Vector< double, 8 >::zeros();
            boost::numeric::ublas::subrange( m_coeffs, 0, 4 ) = *measVec4D;
        }
	}
		
	
	{	// set the intrinsics format from the old format
		intrinsics_type::radial_type radVec;
		radVec( 0 ) = m_coeffs( 0 );
		radVec( 1 ) = m_coeffs( 1 );
		radVec( 2 ) = m_coeffs( 4 );
		radVec( 3 ) = m_coeffs( 5 );
		radVec( 4 ) = m_coeffs( 6 );
		radVec( 5 ) = m_coeffs( 7 );
		intrinsics_type::tangential_type tanVec = intrinsics_type::tangential_type( m_coeffs( 2 ), m_coeffs( 3 ) );
		
		m_intrinsics = intrinsics_type( m_intrinsicMatrix, radVec, tanVec );
	}
}

/// resets the mapping to the provided image parameters
bool Undistortion::resetMapping( const int width, const int height, const intrinsics_type& intrinsics )
{
	LOG4CPP_INFO( logger, "initialize undistortion mapping with intrinsics:\n" << intrinsics );
	
	// reset the map images
	m_pMapX.reset( new Image( width, height, 1, CV_32F ) );
	m_pMapY.reset( new Image( width, height, 1, CV_32F ) );
	
	// copy the values to the corresponding opencv data-structures
	// CvMat cvIntrinsics;
	// CvMat cvCoeffs;
	CvMat* cvCoeffs =  cvCreateMat( 1, intrinsics.radial_size + 2, CV_32FC1 );
	CvMat* cvIntrinsics = cvCreateMat( 3, 3, CV_32FC1 );
	
	Util::cv1::assign( intrinsics, *cvCoeffs, *cvIntrinsics );
	
	// set values to the mapping
	// @todo should be upgraded to modern opencv
	CvMat pX = m_pMapX->Mat();
	CvMat pY = m_pMapY->Mat();
	cvInitUndistortMap( cvIntrinsics, cvCoeffs, &pX, &pY );
	
	// explicitly release the allocated memory
	cvReleaseMat( &cvCoeffs );
	cvReleaseMat( &cvIntrinsics );
	
	// alternative undistortion using special fisheye calibration 8 camera should be really fishy, seems buggy in 2.4.11 (CW@2015-03-24)
	//cv::fisheye::initUndistortRectifyMap( cv::Mat( pCvIntrinsics ), cv::Mat( pCvCoeffs ), cv::Mat::eye( 3, 3, CV_32F ), cv::Mat::eye( 3, 3, CV_32F ), cv::Size( width, height ), CV_32FC1, cv::Mat( *m_pMapX ), cv::Mat( *m_pMapY ) );
	LOG4CPP_INFO( logger, "Initialization of distortion maps finished." );
	
	return true;
}

bool Undistortion::resetMapping( const Vision::Image& image )
{
	// generate a local copy first
	intrinsics_type camIntrinsics = m_intrinsics;
	
	// TODO: image size different than intrinsics ? -> scale
	//correctForScale( image, camIntrinsics );
	// always right-hand -> left-hand
	correctForOpenCV( camIntrinsics );
	// upside down? -> flip intrinsics and tangential param
	correctForOrigin( image, camIntrinsics );
	// set new lookup maps
	if( !resetMapping( image.width(), image.height(), camIntrinsics ) )
		return false;
	
	return isValid( image );
}

Vision::Image::Ptr Undistortion::undistort( Vision::Image::Ptr pImage )
{
	return undistort( *pImage );
}

Vision::Image::Ptr Undistortion::undistort( Image& image )
{
	// check if old values are still valid for the image size, only reset mapping if not
	if( !isValid( image ) )
		if( !resetMapping( image ) )
			return image.Clone();

	// undistort
	Vision::Image::ImageFormatProperties fmt;
	image.getFormatProperties(fmt);
	Vision::Image::Ptr pImgUndistorted( new Image( image.width(), image.height(), fmt, image.getImageState() ) );

	if (image.isOnGPU())	{
		cv::UMat& distortedUMat = image.uMat();
		cv::UMat& undistortedUMat = pImgUndistorted->uMat();
		cv::remap( distortedUMat, undistortedUMat, m_pMapX->uMat(), m_pMapY->uMat(), cv::INTER_LINEAR );
	} else {
		cv::Mat& distortedMat = image.Mat();
		cv::Mat& undistortedMat = pImgUndistorted->Mat();
		cv::remap( distortedMat, undistortedMat, m_pMapX->Mat(), m_pMapY->Mat(), cv::INTER_LINEAR );
	}

	return pImgUndistorted;
}

bool Undistortion::isValid( const Vision::Image& image ) const
{
	if ( !m_pMapX || !m_pMapY )
		return false;
	
	if( m_pMapX->width() != image.width() || m_pMapX->height() != image.height() )
		return false;
	
	if( m_pMapY->width() != image.width() || m_pMapY->height() != image.height() )
		return false;
	
	return true;
}

} } // namespace Ubitrack::Vision
