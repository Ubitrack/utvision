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
 * A class for converting Ubitrack datatypes into OpenCV datatypes and backwards
 *
 * @author Christian Waechter <christian.waechter@in.tum.de>
 */

#ifndef __UBITRACK_VISION_UTIL_OPENCV_UTILS_H_INCLUDED__
#define __UBITRACK_VISION_UTIL_OPENCV_UTILS_H_INCLUDED__

// OpenCV
//#include <opencv/corex.h>			///< include for OpenCV 1.x (irrelevant, most likely)
//#include <opencv/version.h>		///< @todo could check for opencv version
#include <opencv2/core/core.hpp>	// include for OpenCV 2.x
#include <opencv2/core/core_c.h>	// new include for POD from OpenCV 1.x
#include <opencv2/core/types_c.h>	// new include for functions (e.g cvMat )


// Ubitrack
#include <utMath/Vector.h>
#include <utMath/Matrix.h>
#include <utMath/CameraIntrinsics.h>

#include <utUtil/StaticAssert.h>

namespace Ubitrack { namespace Vision { namespace Util {

	/// forward declaration to to type that contains information for data-type mapping
	template< typename UbitrackType >
	struct TypeMapping;	
	
	/// specialization of type mapping for ubitrack 2d-vectors
	template< typename T >
	struct TypeMapping< Math::Vector< T, 2 > >	
	{
		typedef CvPoint2D32f				opencv_1_type; ///< default type
		typedef CvPoint2D32f				opencv_1_type_32;
		typedef CvPoint2D64f				opencv_1_type_64;
	
		typedef typename cv::Point_< T >	opencv_2_type;
	};
	
	/// specialization of type mapping for ubitrack 3d-vectors
	template< typename T >
	struct TypeMapping< Math::Vector< T, 3 > >	
	{
		typedef CvPoint3D32f				opencv_1_type; ///< default type
		typedef CvPoint3D32f				opencv_1_type_32;
		typedef CvPoint3D64f				opencv_1_type_64;
	
		typedef typename cv::Point3_< T >	opencv_2_type;
	
	};
	
	/// specialization of type mapping for general ubitrack vectors
	template< typename T, std::size_t N >
	struct TypeMapping< Math::Vector< T, N > >	
	{
		typedef CvMat							opencv_1_type; ///< default type
		typedef typename cv::Matx< T, N, 1 >	opencv_2_type;
	};
	
	/// specialization of type mapping for ubitrack matrices
	template< typename T, std::size_t M , std::size_t N >
	struct TypeMapping< Math::Matrix< T, M, N > >	
	{
		typedef CvMat							opencv_1_type; ///< default type
		typedef typename cv::Matx< T, M, N >	opencv_2_type;
	};
	
namespace cv1 {

	template< typename ReturnType, typename ValueType >
	ReturnType make_opencv( const ValueType& )
	{
		UBITRACK_STATIC_ASSERT( false, REQUESTED_TYPE_CONVERSION_NOT_AVAILABLE );
	}

	/// generate \c OpenCV1 2-vector from provided \c Ubitrack 2-vector
	template< typename PrecisionType >
	inline typename TypeMapping< Math::Vector< PrecisionType, 2 > >::opencv_1_type make_opencv( const Math::Vector< PrecisionType, 2 > &vec )
	{
		typedef typename TypeMapping< Math::Vector< PrecisionType, 2 > >::opencv_1_type return_type;
		return_type value;
		value.x = vec( 0 );
		value.y = vec( 1 );
		return value;
	}
	
	/// generate \c OpenCV1 3-vector from provided \c Ubitrack 3-vector
	template< typename PrecisionType >
	inline typename TypeMapping< Math::Vector< PrecisionType, 3 > >::opencv_1_type make_opencv( const Math::Vector< PrecisionType, 3 > &vec )
	{
		typedef typename TypeMapping< Math::Vector< PrecisionType, 3 > >::opencv_1_type return_type;
		return_type value;
		value.x = vec( 0 );
		value.y = vec( 1 );
		value.z = vec( 2 );
		return value;
	}

	/// attention, no real copy is generated CVMat will just point to the data of the ubitrack vector. see the header opencv2/core/types_c.hpp
	template< typename PrecisionType, std::size_t N >
	inline typename TypeMapping< Math::Vector< PrecisionType, N > >::opencv_1_type make_opencv( Math::Vector< PrecisionType, N > &vec )
	{
		// (ueck) fixed compilation on OSX - first parameter is an int value representing the number of bytes of each element
		return cvMat( N, 1, CV_MAKETYPE( sizeof(PrecisionType), 1 ), reinterpret_cast< void* > ( vec.content() ) );
		//return cvMat( N, 1, typename cv::DataType< PrecisionType >::type, reinterpret_cast< void* > ( vec.content() ) );
	}
	
	/// attention: data is copied here, whereas in vector it is not. Use carefully
	template< typename PrecisionType, std::size_t M, size_t N >
	typename TypeMapping< Math::Matrix< PrecisionType, M, N >  >::opencv_1_type make_opencv( const Math::Matrix< PrecisionType, M, N > &mat )
	{
		PrecisionType* pData = new PrecisionType[ M*N ];
		for( std::size_t i = 0; i<M; ++i )
			for( std::size_t j = 0; i<N; ++j )
				*( pData + (j*M) + i ) = mat( i, j );

		// (ueck) fixed compilation on OSX - first parameter is an int value representing the number of bytes of each element
		return cvMat( M, N, CV_MAKETYPE( sizeof(PrecisionType), 1 ), reinterpret_cast< void* > ( pData ) );
	}
	
	/// copy elements from Ubitrack's intrinsic class to the corresponding OpenCV 1.x data structures
	template< typename PrecisionType >
	bool assign( const Math::CameraIntrinsics< PrecisionType >& camIn, CvMat& matOpenCV )
	{
		typedef typename Math::CameraIntrinsics< PrecisionType > type;
		const std::size_t n_rad = type::radial( camIn.calib_type );
		const std::size_t n_tan = type::tangential( camIn.calib_type );
		const std::size_t n = n_rad + n_tan;
		
		// do nothing if pointer is  zero
		if( !matOpenCV.data.fl )
			return false;
		
		// return if not enough space is reserved
		if( (matOpenCV.rows*matOpenCV.cols) != n )
			return false;
		
		// at least one dimension should be one (vector)
		if( matOpenCV.rows != 1 && matOpenCV.cols != 1 )
			return false;
	
		matOpenCV.data.fl[ 0 ] = static_cast< float >( camIn.radial_params( 0 ) );
		matOpenCV.data.fl[ 1 ] = static_cast< float >( camIn.radial_params( 1 ) );
		matOpenCV.data.fl[ 2 ] = static_cast< float >( camIn.tangential_params( 0 ) );
		matOpenCV.data.fl[ 3 ] = static_cast< float >( camIn.tangential_params( 1 ) );
		for( std::size_t i =  4; i< (n_rad+2); ++i )
			matOpenCV.data.fl[ i ] = static_cast< float >( camIn.radial_params( i-2 ) );
		
		return true;
	}
	
	/// copy an Ubitrack 3x3-matrix to a corresponding OpnCV matrix element wise 
	template< typename PrecisionType >
	bool assign( const Math::Matrix< PrecisionType, 3, 3 >& matrix, CvMat& matOpenCV )
	{	
		// do nothing if pointer is  zero
		if( !matOpenCV.data.fl )
			return false;
		
		// return if not enough space is reserved
		if( matOpenCV.rows != 3 || matOpenCV.cols != 3 )
			return false;
		
		float* pDest = matOpenCV.data.fl;
		const PrecisionType* pSource = matrix.content();
		//remark for column-major: (i/3) => columns, (i%3) => rows 
		for( std::size_t i = 0; i != 9; ++pDest, ++i )
			*pDest = static_cast< float > ( *( pSource + (i%3)*3  + (i/3) ) );
		return true;
	}
	
	/// copy elements from Ubitrack's intrinsic class to the corresponding OpenCV 1.x data structures
	template< typename PrecisionType >
	bool assign( const Math::CameraIntrinsics< PrecisionType >& camIn, CvMat& matDist, CvMat& camMat )
	{
		bool valid ( true );
		valid &= assign( camIn, matDist );
		valid &= assign( camIn.matrix, camMat );
		return valid;
	}

	
	/// copies the vector element-wise to the other vector
	template< typename PrecisionType, std::size_t N, typename PointerTypeOut >
	PointerTypeOut* assign_unsafe( const Math::Vector< PrecisionType, N >& vec, PointerTypeOut* pOut )
	{
		typedef PointerTypeOut target_precision_type;
		for( std::size_t i =0; i != N; ++i )
			(*pOut++) = static_cast< target_precision_type > ( vec( i ) );
		return pOut;
	}
	
	///copy vectors Ubitrack-Vector vectors to a raw pointer array, be careful does not check for valid pointer
	template< template< typename ValueType, typename AllocType > class ContainerType, typename ValueType, typename AllocType, typename PointerTypeOut >
	PointerTypeOut* assign_unsafe( const ContainerType< ValueType, AllocType > &ubitrackVectorList, PointerTypeOut* pOut )
	{
		if( ubitrackVectorList.empty() )
			return pOut;
		
		typename ContainerType< ValueType, AllocType >::const_iterator itBegin = ubitrackVectorList.begin();
		const typename ContainerType< ValueType, AllocType >::const_iterator itFinal = ubitrackVectorList.end();
		
		for( ; itBegin != itFinal; ++itBegin )
		{
			typename ValueType::const_iterator itPoints = itBegin->begin();
			const typename ValueType::const_iterator itEnd = itBegin->end();
			
			// can also handle lists of lists in a recursive manner
			for( ; itPoints != itEnd; ++itPoints )
				pOut = assign_unsafe( *itPoints, pOut );
		}
		return pOut;
	}
	
	
}	// namespace ::cv1
	
// OpenCV introduced more c++ types with version 2.x
// these are using more templates, see 
//Mat A(30, 40, DataType<float>::type);
namespace cv2 {

	template< typename ReturnType, typename ValueType >
	ReturnType make_opencv( const ValueType& )
	{
		UBITRACK_STATIC_ASSERT( false, REQUESTED_TYPE_CONVERSION_NOT_AVAILABLE );
	}

	/// generate \c OpenCV2 2-vector from provided \c Ubitrack 2-vector
	template< typename PrecisionType >
	inline typename TypeMapping< Math::Vector< PrecisionType, 2 > >::opencv_2_type make_opencv( const Math::Vector< PrecisionType, 2 > &vec )
	{
		typedef Math::Vector< PrecisionType, 2 > type_to_map;
		typedef typename TypeMapping< type_to_map >::opencv_2_type return_type;
		return return_type( vec( 0 ), vec( 1 ) );
	}
	
	/// generate \c OpenCV2 3-vector from provided \c Ubitrack 3-vector
	template< typename PrecisionType >
	inline typename TypeMapping< Math::Vector< PrecisionType, 3 > >::opencv_2_type make_opencv( const Math::Vector< PrecisionType, 3 > &vec )
	{
		typedef Math::Vector< PrecisionType, 3 > type_to_map;
		typedef typename TypeMapping< type_to_map >::opencv_2_type return_type;
		return return_type( vec( 0 ), vec( 1 ), vec( 2 ) );
	}
	
	/// generate \c OpenCV2 vector from provided \c Ubitrack vector
	template< typename PrecisionType, size_t N >
	inline typename TypeMapping< Math::Vector< PrecisionType, N > >::opencv_2_type make_opencv( const Math::Vector< PrecisionType, N > &vec )
	{
		typedef Math::Vector< PrecisionType, N > type_to_map;
		typedef typename TypeMapping< type_to_map >::opencv_2_type return_type;
		return return_type( vec.content() );
		//return cv::Vec< PrecisionType, N >( vec.content() );
	}
	
	/// generate \c OpenCV2 matrix from provided \c Ubitrack matrix
	template< typename PrecisionType, std::size_t M >
	inline typename TypeMapping< Math::Matrix< PrecisionType, M, M > >::opencv_2_type make_opencv( const Math::Matrix< PrecisionType, M, M > &mat )
	{
		typedef Math::Matrix< PrecisionType, M, M > type_to_map;
		typedef typename TypeMapping< type_to_map >::opencv_2_type return_type;
		/// needs to return transposed works only  for symmetric matrices now
		return return_type( mat.content() ).t();
	}
	
	
	// template< typename PrecisionType, std::size_t M, size_t N >
	// inline typename TypeMapping< Math::::Matrix< PrecisionType, M, N > >::opencv_2_type make_opencv( const Math::Matrix< PrecisionType, M, N > &mat )
	// {
		// typedef Math::Matrix< PrecisionType, M, N > type_to_map;
		// return typename TypeMapping< type_to_map >::opencv_2_type( mat.content() );
	// }
	
	
	/// copy an Ubitrack matrix to a corresponding OpnCV matrix element wise 
	template< typename PrecisionType, std::size_t M, std::size_t N, int M2, int N2 >
	void assign( const Math::Matrix< PrecisionType, M, N >& matFrom, cv::Matx< PrecisionType, M2, N2 >& matOpenCV )
	{
		const cv::Matx< PrecisionType, N2, M2 > tmp ( matFrom.content() );
		matOpenCV = tmp.t();
	}
	
	/// assign contents of \c Ubitrack vector to \c OpenCV2 vector
	template< typename T, std::size_t N >
	void assign( const Math::Vector< T, N > vecFrom, typename cv::Matx< T, N, 1 >& vecTo )
	{
		for( std::size_t i ( 0 ); i<N; ++i )
			vecTo( i, 0 ) = vecFrom( i );
	}
	
	/// copy elements from Ubitrack's intrinsic class to the corresponding OpenCV 2.x data structures
	template< typename PrecisionType, int N >
	void assign( const Math::CameraIntrinsics< PrecisionType >& camIn, cv::Matx< PrecisionType, N, 1 >& matOpenCV )
	{
		matOpenCV( 0, 0 ) = static_cast< float >( camIn.radial_params( 0 ) );
		matOpenCV( 1, 0 ) = static_cast< float >( camIn.radial_params( 1 ) );
		matOpenCV( 2, 0 ) = static_cast< float >( camIn.tangential_params( 0 ) );
		matOpenCV( 3, 0 ) = static_cast< float >( camIn.tangential_params( 1 ) );
		//for( std::size_t i =  4; i< std::min( static_cast< std::size_t > (N), static_cast< int > (camIn.radial_size+2) ); ++i )
		for( std::size_t i =  4; i< std::min( static_cast< std::size_t > (N),(camIn.radial_size+2u) ); ++i )
			matOpenCV( i, 0 ) = static_cast< float >( camIn.radial_params( i-2 ) );
	}
	
	/// copy elements from Ubitrack's intrinsic class to the corresponding OpenCV 2.x data structures
	template< typename PrecisionType, int N >
	void assign( const Math::CameraIntrinsics< PrecisionType >& camIn, cv::Matx< PrecisionType, N, 1 >& dist, cv::Matx< PrecisionType, 3, 3 >& mat  )
	{
		assign( camIn, dist );
		assign( camIn.matrix, mat );
	}
	
	/// negate last row of OpenCV matrix to flip the (left/right) handiness of a OpenCV's intrinsic matrix
	template< typename PrecisionType >	
	inline void flipHandiness( cv::Matx< PrecisionType, 3, 3 >& intrinsics )
	{
		// compensate for right-handed Ubitracks' coordinate frame
		intrinsics ( 0, 2 ) *= -1;
		intrinsics ( 1, 2 ) *= -1;
		intrinsics ( 2, 2 ) *= -1;
	}
}	// namespace ::cv2

	
/** 
 * this function \c must not be overloaded
 * It is used in generic functions (e.g. \c std::transform) and it should not be overloaded, otherwise compilation will most likely fail.
 */
template< typename UbitrackType > 
inline typename TypeMapping< UbitrackType >::opencv_2_type makeOpenCV2( const UbitrackType& value )
{
	return cv2::make_opencv( value );
}

/** 
 * this function \c must not be overloaded within this namespace
 * It can be used for all types
 */
template< typename UbitrackType, typename MappedType > 
inline void assign( const UbitrackType& value, MappedType& other )
{
	using namespace Vision::Util::cv1;
	using namespace Vision::Util::cv2;
	assign( value, other );
}

namespace cv2 {
	///copy a single vector of Ubitrack types into a vector corresponding of OpenCV types
	template< template< typename ElementTypeIn, typename AllocTypeIn > class ContainerTypeIn, typename ElementTypeIn, typename AllocTypeIn
		, template< typename ElementTypeOut, typename AllocTypeOut > class ContainerTypeOut, typename ElementTypeOut, typename AllocTypeOut >
	void assign( const ContainerTypeIn< ElementTypeIn, AllocTypeIn > &ubitrackList, ContainerTypeOut< ElementTypeOut, AllocTypeOut >& otherPointList )
	{
		typedef ContainerTypeIn< ElementTypeIn, AllocTypeIn >					container_in;
		typedef ContainerTypeOut< ElementTypeOut, AllocTypeOut >				container_out;
		typedef ElementTypeOut													opencv_type;
		//typedef typename std::iterator_traits< container_in >::value_type		ubitrack_type; /// does not compile on msvc 10.0,iterato_category is missing on container_type
		typedef ElementTypeIn													ubitrack_type;
		
		//UBITRACK_STATIC_ASSERT( (typename OpenCV::TypeMapping< ubitrack_type >::opencv_2_type == opencv_type), TYPES_DO_NOT_MATCH_EACH_OTHER );
		
		if( ubitrackList.empty() )
			return;
		
		typename container_in::const_iterator itBegin = ubitrackList.begin();
		const typename container_in::const_iterator itEnd = ubitrackList.end();
		
		// sum up incoming and already existing elements to reserve enough space
		const std::size_t n_values = std::distance( itBegin, itEnd ) + std::distance( otherPointList.begin(), otherPointList.end() );
		otherPointList.reserve( n_values );
		
		// define the type of conversion function
		typedef opencv_type														(*convert_function_ptr) ( const ubitrack_type& );
		convert_function_ptr func = Util::makeOpenCV2< ubitrack_type >;
		
		// transform the points to the final container
		std::transform( itBegin, itEnd, std::back_inserter( otherPointList ), func );
	}
}	// namespace ::cv2

}}} // namespace Ubitrack::Vision::Util

#endif	//__UBITRACK_VISION_UTIL_OPENCV_UTILS_H_INCLUDED__
