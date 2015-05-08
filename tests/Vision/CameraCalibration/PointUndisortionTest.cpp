
// Boost
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

//#include <opencv2/calib3d/calib3d.hpp> //cv::fisheye::undistortPoints <- for fisheye
#include <opencv2/imgproc/imgproc.hpp> //cv::undistortPoints

// Ubitrack

#include <utMath/Vector.h>
#include <utMath/Matrix.h>
#include <utMath/Blas1.h>	// vector norm2
#include <utMath/Blas2.h>	// matrix-vector product
#include <utMath/Random/Scalar.h>
#include <utMath/Random/Vector.h>
#include <utMath/CameraIntrinsics.h>
#include <utAlgorithm/LensDistortion.h>			// old undistortion using 2 radial coefficients
#include <utAlgorithm/CameraLens/Correction.h>	// new undistortion using 6 radial coefficients

#include <utVision/Util/OpenCV.h> // assign opencv elements from ubtirack

// some helper functions
namespace {
	
using namespace Ubitrack;
using namespace Ubitrack::Math;
	
	/// project a single opencv point
	template< typename T >
	Vector< T, 2 > project( const cv::Matx< T, 3, 3 >& mat, const cv::Point_< T >& pt2 )
	{
		cv::Point3_< T > pt3 ( pt2.x, pt2.y, 1 );
		pt3 = ( mat * pt3 );
		return Vector< T, 2 >( (pt3.x / pt3.z), (pt3.y / pt3.z) );
	}
	/* // alternative for opencv vector
	template< typename T >
	cv::Point2d project( const cv::Matx< T, 3, 3 >& mat, const cv::Point2d& pt2 )
	{
		cv::Point3d pt3 ( pt2.x, pt2.y, 1 );
		pt3 = ( mat * pt3 );
		return cv::Point2d( pt3.x / pt3.z, pt3.y / pt3.z );
	}*/


	/// project a single ubitrack point (sensor plane -> pixel plane )
	template< typename T >
	Math::Vector< T, 2 > project( const Math::Matrix< T, 3, 3 >& K, const Math::Vector< T, 2 >& dp )
	{
		Math::Vector< T, 3 > vec;
		Math::product( K, Math::Vector< T, 3 >( dp( 0 ), dp( 1 ), 1 ), vec );
		return Math::Vector< T, 2 >( vec( 0 ) / vec( 2 ), vec( 1 ) / vec( 2 ) );
	}

	///unporject a single ubitrack vector (pixel plane -> sensor plane )
	template< typename T >
	Math::Vector< T, 2 > unproject( const Math::Matrix< T, 3, 3 >& K, const Math::Vector< T, 2 >& p )
	{
		const T x2 = ( p( 1 ) - K( 1, 2 ) * K( 2, 2 ) ) / K( 1, 1 );
		const T x1 = ( p( 0 ) - K( 0, 1 ) * x2 - K( 0, 2 ) * K( 2, 2 ) ) / K( 0, 0 );
		return Math::Vector< T, 2 > ( x1, x2 );
	}

	/// generate a projection matrix for specified screen size and random focal length
	template< typename T >
	Math::Matrix< T, 3, 3 > generateRandomMatrix( const Math::Vector< T, 2 > &screenSize, const Math::Vector< T, 2 > &focalMinMax )
	{
		Math::Matrix< T, 3, 3 > cam( Matrix< T, 3, 3 >::identity() );
		cam( 0, 0 ) = Random::distribute_uniform< T >( focalMinMax( 0 ), focalMinMax( 1 ) );
		cam( 1, 1 ) = Random::distribute_uniform< T >( focalMinMax( 0 ), focalMinMax( 1 ) );
		cam( 0, 2 ) = -(screenSize( 0 ) - 1 ) * 0.5; // assume center in range [0;w-1] not within [1;w]
		cam( 1, 2 ) = -(screenSize( 1 ) - 1 ) * 0.5; // assume center in range [0;h-1] not within [1;h]
		cam( 2, 2 ) = -1;
		return cam;
	}

	/// generate random intrinsics parameters to test lens un-/distortion effects
	template< std::size_t N, typename T >
	CameraIntrinsics< T > generateRandomIntrinsics( const Vector< T, 2 > &screenSize, const Vector< T, 2 > &focalMinMax )
	{
		/// radial lens distortion usually is within a range of [-1;1]
		Math::Vector< T, N > radVec;
		radVec( 0 ) = Random::distribute_uniform< T >( -0.1, 0.1 );
		radVec( 1 ) = Random::distribute_uniform< T >( -0.1, 0.1 );
		for( std::size_t i = 2; i!= N; ++i )
			radVec( i ) = Random::distribute_uniform< T >( -1, 1 );
		
		/// tangential lens distortion usually is within a much smaller range ( e.g. [-0.01;0.01] )
		Math::Vector< T, 2 > tanVec;
		tanVec( 0 ) = Random::distribute_uniform< T >( -0.01, 0.01 );
		tanVec( 1 ) = Random::distribute_uniform< T >( -0.01, 0.01 );
		
		return CameraIntrinsics< T >( generateRandomMatrix( screenSize, focalMinMax ), radVec, tanVec );
	}
	
	template< typename T, std::size_t N >
	T getRMS( const std::vector< Math::Vector< T, N > >& vec1, const std::vector< Math::Vector< T, N > > &vec2 )
	{
		typename std::vector< Math::Vector< T, N > >::const_iterator it1 = vec1.begin();
		const typename std::vector< Math::Vector< T, N > >::const_iterator ite = vec1.end();
		const std::size_t n = std::distance( it1, ite );
		
		
		typename std::vector< Math::Vector< T, N > >::const_iterator it2 = vec2.begin();
		T sum( 0 );
		for( ; it1 != ite; ++it1, ++it2 )
		{
			const Math::Vector< T, N > diff = ( *it1 - *it2 );
			const T dist = Math::norm_2( diff );
			sum += dist;
		}
		return std::sqrt( sum/n );
	}
	
	template< std::size_t Dist, typename T, std::size_t N > // Dist == number of radial distortion coefficients)
	void undistortOpenCV( const CameraIntrinsics< T > &camIntrins, const std::vector< Math::Vector< T, N > >& distCameraPoints, std::vector< Math::Vector< T, N > > &opencv_results )
	{
		//assign the camera coefficients to opencv format ( Dist == number of radial distortion coefficients)
		cv::Matx< T, Dist+2, 1 > cvCoeffs; // and do not forget space for the other two ( tangential distortion )
		cv::Matx< T, 3, 3 > cvMatrix;
		Vision::Util::cv2::assign( camIntrins, cvCoeffs, cvMatrix );
		
		// std::cout << "Applied OpenCV Matrix " << cvMatrix << "\n";
		// std::cout << "Applied OpenCV Coeffs " << cvCoeffs << "\n";
		
		// convert matrix to left-handed (OpenCV) matrix
		Vision::Util::cv2::flipHandiness( cvMatrix );
	
		// copy all points to a corresponding opencv vector
		std::vector< cv::Point_< T > > opencv_points_in;
		Vision::Util::cv2::assign( distCameraPoints, opencv_points_in );
		
		// resulting undistorted points
		std::vector< cv::Point_< T > > opencv_points_out;
		
		// opencv undistortion
		cv::undistortPoints( opencv_points_in, opencv_points_out, cvMatrix, cvCoeffs );// only for stereo ->, InputArray R=noArray(), InputArray P=noArray())
		//cvUndistortPoints(const CvMat* src, CvMat* dst, const CvMat* camera_matrix, const CvMat* dist_coeffs, const CvMat* R=0, const CvMat* P=0 )
		
		{	// finally project the undistorted points to the image plane again
			typename std::vector< cv::Point_< T > >::const_iterator itPt2 = opencv_points_out.begin();
			const typename std::vector< cv::Point_< T > >::const_iterator itEnd = opencv_points_out.end();
			opencv_results.reserve( distCameraPoints.size() );
			for( ; itPt2 != itEnd; ++itPt2 )
				opencv_results.push_back( project( cvMatrix, *itPt2 ) );
		}
		
	}

	template< typename T, std::size_t N >
	void undistortUbitrackLens( const CameraIntrinsics< T > &camIntrins, const std::vector< Math::Vector< T, N > >& distCameraPoints, std::vector< Math::Vector< T, N > > &results )	
	{
		const Math::Vector< T, 4 > coeffs( camIntrins.radial_params( 0 )
			, camIntrins.radial_params( 1 )
			, camIntrins.tangential_params( 0 )
			, camIntrins.tangential_params( 1 ) );
			
		typename std::vector< Math::Vector< T, N > >::const_iterator itp = distCameraPoints.begin();
		const typename std::vector< Math::Vector< T, N > >::const_iterator ite = distCameraPoints.end();
		
		const std::size_t n = std::distance( itp, ite );
		results.reserve( results.size() + n );
		
		for( ; itp != ite; ++itp )
		{
			const Vector< T, 2 > tmp = Ubitrack::Algorithm::lensUnDistort(  *itp, coeffs, camIntrins.matrix );
			results.push_back( tmp );
		}
	}	
}	// anonymous namespace

template< typename T >
void TestPointUndistorion( const std::size_t n_runs, const T epsilon )
{
	using namespace Ubitrack::Vision;
	// random intrinsics matrix, only small changes in focal length every time
	const Vector< T, 2 > screenResolution( 640, 480 );
	const Vector< T, 2 > focalLengthRange( 500, 800  );
	typename Random::Vector< T, 2 >::Uniform randVector( 0, 640 );
	
	const std::size_t n_p2d = 100;//3 + ( iRun % 28);//( Random::distribute_uniform< std::size_t >( 3, 30 ) );
	
	T rms1 = 0, rms2 = 0, rms3 = 0;
	std::size_t counter1 = 0 , counter2 = 0, counter3 = 0;
	for ( std::size_t iRun = 0; iRun < n_runs; iRun++ )
	{
		// generate some random points, roughly within camera screen
		std::vector< Vector< T, 2 > > gtCameraPoints;
		gtCameraPoints.reserve( n_p2d );
		std::generate_n ( std::back_inserter( gtCameraPoints ), n_p2d,  randVector );

		{
			Math::CameraIntrinsics< T > camIntrins2rad = generateRandomIntrinsics< 2 >( screenResolution, focalLengthRange );
			
			// distort the points using 2 radial distortion parameters( for old undistortion)
			std::vector< Vector< T, 2 > > disCameraPoints2rad( n_p2d );
			Ubitrack::Algorithm::CameraLens::distort( camIntrins2rad, gtCameraPoints, disCameraPoints2rad );
		
			// undistort via opencv for 2 radial distortion parameters
			std::vector < Math::Vector< T, 2 > > opencvResults2rad;
			undistortOpenCV< 2 >( camIntrins2rad, disCameraPoints2rad, opencvResults2rad );
			
			// undistort with old ubitrack version
			std::vector < Math::Vector< T, 2 > > old_ubitrack_results;
			undistortUbitrackLens( camIntrins2rad, disCameraPoints2rad, old_ubitrack_results );
			
			// undistort using newer ubitrack version (and 2 radial distortion parameters )
			std::vector< Math::Vector< T, 2 > > ubitrackResults2rad( n_p2d );
			Ubitrack::Algorithm::CameraLens::undistort( camIntrins2rad, disCameraPoints2rad, ubitrackResults2rad );
			
			const T rmsOCV2 = getRMS( gtCameraPoints, opencvResults2rad );
			const T rmsUTO2 = getRMS( gtCameraPoints, old_ubitrack_results );
			const T rmsUTN2 = getRMS( gtCameraPoints, ubitrackResults2rad );
			// BOOST_CHECK( abs(rmsUTO2 - rmsOCV2 ) < 1 );
			// BOOST_CHECK( abs(rmsUTN2 - rmsOCV2 ) < 1 );
			rms1 += rmsUTO2;
			rms2 += rmsUTN2;
			// std::cout << "rad 2 " << rmsOCV2 << " vs. " << rmsUTO2 << " vs. " << rmsUTN2 << "\n";
			if( rmsUTO2 >  rmsOCV2 )
				++counter1;
			if( rmsUTN2 >  rmsOCV2 )
				++counter2;
			
		}
		{
			Math::CameraIntrinsics< T > camIntrins6rad = generateRandomIntrinsics< 6 >( screenResolution, focalLengthRange );
			
			// distort the points using 6 radial distortion parameters
			std::vector< Vector< T, 2 > > disCameraPoints6rad( n_p2d );
			Ubitrack::Algorithm::CameraLens::distort( camIntrins6rad, gtCameraPoints, disCameraPoints6rad );
			
			// undistort via opencv for 6 radial distortion parameters
			std::vector < Math::Vector< T, 2 > > opencvResults6rad;
			undistortOpenCV< 6 >( camIntrins6rad, disCameraPoints6rad, opencvResults6rad );
			
			// undistort using newer ubitrack version (and 6 radial distortion parameters )
			std::vector< Math::Vector< T, 2 > > ubitrackResults6rad( n_p2d );
			Ubitrack::Algorithm::CameraLens::undistort( camIntrins6rad, disCameraPoints6rad, ubitrackResults6rad );
			
			const T rmsOCV6 = getRMS( gtCameraPoints, opencvResults6rad );
			const T rmsUbi6 = getRMS( gtCameraPoints, ubitrackResults6rad );
			// BOOST_CHECK( abs(rmsUbi6 - rmsOCV6 ) < 1 );
			// BOOST_CHECK( rmsUbi6 < rmsOCV6 );
			// std::cout << "rad 6 " << rmsOCV6 << " vs. " << rmsUbi6 << "\n";
			rms3 += rmsUbi6;
			if( rmsUbi6 >  rmsOCV6 )
				++counter3;
		}
	}
	BOOST_CHECK( (rms1 / n_runs) < epsilon );
	BOOST_CHECK( (rms2 / n_runs) < epsilon );
	BOOST_CHECK( (rms3 / n_runs) < epsilon );
	std::cout << "after " << n_runs << " runs using precision type \"" << typeid( T ).name() << "\" with " << n_p2d << " points each :"
		<< "\nold ubitrack (2rad) : " << counter1 << " out of " << n_runs << " worse than OpenCV, average rms=" << (rms1 / n_runs)
		<< "\nnew ubitrack (2rad) : " << counter2 << " out of " << n_runs << " worse than OpenCV, average rms=" << (rms2 / n_runs)
		<< "\nnew ubitrack (6rad) : " << counter3 << " out of " << n_runs << " worse than OpenCV, average rms=" << (rms3 / n_runs) << "\n";
}

void TestPointUndistorion()
{
	// do some iterations of random tests
	TestPointUndistorion< float >( 10000, 1.f );
	TestPointUndistorion< double >( 10000, 1. );
	
	BOOST_CHECK( 0 == 0 ); // to avoid boost message
}
