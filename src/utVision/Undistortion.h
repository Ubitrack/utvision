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

#ifndef __UBITRACK_VISION_UNDISTORTION_H_INCLUDED__
#define __UBITRACK_VISION_UNDISTORTION_H_INCLUDED__

// std
#include <string>

// Boost
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

// Ubitrack
#include "../utVision.h"	// UTVISION_EXPORT
#include <utMath/Vector.h>
#include <utMath/Matrix.h>
#include <utMath/CameraIntrinsics.h>

//forward declaration
namespace Ubitrack { namespace Vision {
	class Image;
}}

namespace Ubitrack { namespace Vision {

class UTVISION_EXPORT Undistortion
{
	/// type used throughout the class used for representing distortion lens distortion parameters
	typedef Math::CameraIntrinsics< double > intrinsics_type;
	
	/// new intrinsic parameters
	intrinsics_type m_intrinsics;
	
	/// old distortion parameters (opencv style)
	Math::Vector< double, 8 > m_coeffs;
	
	/// intrinsic camera matrix
	Math::Matrix< double, 3, 3 > m_intrinsicMatrix;
	
	// undistortion maps
	boost::scoped_ptr< Image > m_pMapX;
	boost::scoped_ptr< Image > m_pMapY;
	
public:
	/** standard constructor */
	Undistortion();
	
	/**
	 * Initialize from new camera intrinsics 
	 */
	Undistortion( const intrinsics_type& intrinsics );
	
	/**
	* Initialize from CameraIntrinsics file.
	*/
	Undistortion( const std::string& CameraIntrinsicsFile );
	
	/**
	 * Initialize from two filenames, left to comply with old code
	 */
	Undistortion( const std::string& intrinsicMatrixFile, const std::string& distortionFile );

	/// validates if the mapping is set correctly for the provided image
	bool isValid( const Vision::Image& image ) const;
	
	/// resets the intrinsic parameters from a provided intrinsics structure
	void reset( const intrinsics_type& intrinsics );

	/// resets the intrinsic parameters loading from a file
	void reset( const std::string& intrinsicsFile );
	
	/** initialize parameters from two filenames */
	void reset( const std::string& intrinsicMatrixFile, const std::string& distortionFile );

	/** initialize from matrix+vector */
	void reset( const Math::Matrix< double, 3, 3 >& intrinsicMatrix, const Math::Vector< double, 8 >& distortion );
	
	/** 
	 * Undistorts an image.
	 */
	boost::shared_ptr< Image > undistort( Image& image );

	/** 
	 * Undistorts an image.
	 * Returns the original pointer if no distortion.
	 */
	boost::shared_ptr< Image > undistort( boost::shared_ptr< Image > pImage );
	
	/** returns the intrinsic matrix */
	const Math::Matrix< double, 3, 3 >& getMatrix() const
	{
		return m_intrinsicMatrix;
	}
	
	/** returns the radial distortion coefficients */
	const Math::Vector< double, 8 >& getRadialCoeffs() const
	{
		return m_coeffs;
	}
	
	/** returns the camera model - should be renamed .. */
	const intrinsics_type& getIntrinsics() const
	{
		return m_intrinsics;
	}

protected:

	/// resets the mapping to the provided image parameters
	bool resetMapping( const int width, const int height, const intrinsics_type& intrinsics );

	/// overloaded function to reset undistortion maps using data from connected components
	bool resetMapping( const Vision::Image& image );	
};

} } // namespace Ubitrack::Vision

#endif
