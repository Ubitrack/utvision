#include "Debugging.h"

#include <utVision/Colors.h>

#include <boost/numeric/ublas/operations.hpp>
#include <opencv2/core/core_c.h> // cv::Scalar
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <utVision/Colors.h>

namespace Ubitrack { namespace Vision {

void drawPoseCube( cv::Mat& img, const Math::Pose& pose, const Math::Matrix< float, 3, 3 >& K, double scale, cv::Scalar color, bool paintCoordSystem )
{
	namespace ublas = boost::numeric::ublas;

	// the corner points
	static float points3D[ 12 ][ 3 ] = {
		{ 0.5f, 0.5f, 0.0f }, { -0.5f, 0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f },
		{ 0.5f, 0.5f, 1.0f }, { -0.5f, 0.5f, 1.0f }, { -0.5f, -0.5f, 1.0f }, { 0.5f, -0.5f, 1.0f },
		{ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }
	};

	Math::Matrix< double, 3, 3 > mat = Math::Matrix< double, 3, 3 >::identity();
	Math::Pose pose2 = Math::Pose(Math::Quaternion(mat), Math::Vector< double, 3 >(0.0, 0.0, -5.0));

	// project points
	Math::Vector< double, 3 > p2D[ 12 ];
	for ( int i = 0; i < 12; i++ )
	{
		p2D[ i ] = ublas::prod( K, pose * Math::Vector< double, 3 >( scale * Math::Vector< float, 3 >( points3D[ i ] ) ) );
		p2D[ i ] /= p2D[ i ][ 2 ];

		p2D[ i ](1) = img.rows  - p2D[ i ](1)  -1;

	}

	int fac = 16;

	// draw some lines
	for ( int c = 0; c < 8; c += 4 )
		for ( int i = 0; i < 4; i++ )
			line( img, cvPoint( cvRound( p2D[ c + i ][ 0 ] * fac ), cvRound( p2D[ c + i ][ 1 ] * fac ) ),
				cvPoint( cvRound( p2D[ c + ( i + 1 ) % 4 ][ 0 ] * fac ), cvRound( p2D[ c + ( i + 1 ) % 4 ][ 1 ] * fac ) ),
				color, 1, CV_AA, 4 );
	for ( int i = 0; i < 4; i++ )
			line( img, cvPoint( cvRound( p2D[ i ][ 0 ] * fac ), cvRound( p2D[ i ][ 1 ] * fac ) ),
			cvPoint( cvRound( p2D[ i + 4 ][ 0 ] * fac ), cvRound( p2D[ i + 4 ][ 1 ] * fac ) ),
			i == 0 ? cv::Scalar( 0, 0, 0 ) : color, 1, CV_AA, 4 );

	if ( paintCoordSystem ) {

		line( img, cvPoint( cvRound( p2D[ 8 ][ 0 ] * fac ), cvRound( p2D[ 8 ][ 1 ] * fac ) ),
			cvPoint( cvRound( p2D[ 9 ][ 0 ] * fac ), cvRound( p2D[ 9 ][ 1 ] * fac ) ),
			cv::Scalar( 255, 0, 0 ), 4, CV_AA, 4 );

		line( img, cvPoint( cvRound( p2D[ 8 ][ 0 ] * fac ), cvRound( p2D[ 8 ][ 1 ] * fac ) ),
			cvPoint( cvRound( p2D[ 10 ][ 0 ] * fac ), cvRound( p2D[ 10 ][ 1 ] * fac ) ),
			cv::Scalar( 0, 255, 0 ), 4, CV_AA, 4 );

		line( img, cvPoint( cvRound( p2D[ 8 ][ 0 ] * fac ), cvRound( p2D[ 8 ][ 1 ] * fac ) ),
			cvPoint( cvRound( p2D[ 11 ][ 0 ] * fac ), cvRound( p2D[ 11 ][ 1 ] * fac ) ),
			cv::Scalar( 0, 0, 255 ), 4, CV_AA, 4 );
	}
}


Math::Vector< double, 3 > projectPoint ( const Math::Vector< double, 3 >& pt, const Math::Matrix< double, 3, 3 >& projection, int imageHeight )
{
	Math::Vector< double, 3 > p2D = boost::numeric::ublas::prod( projection, pt );
	p2D /= p2D(2);
	// flip y
	if (imageHeight > 0)
		p2D(1) = imageHeight - p2D(1) - 1;
	return p2D;
}


void drawPose ( cv::Mat & dbgImage, const Math::Pose& pose, const Math::Matrix< double, 3, 3 > &projection, double error )
{
	

    // the corner points
	static float points3D[ 4 ][ 3 ] = {
		{ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },
	};

	// project points
	Math::Vector< double, 3 > p2D[ 4 ];
	for ( int i = 0; i < 4; i++ )
	{
		p2D[ i] = projectPoint ( pose * ( Math::Vector< float, 3 >( points3D[ i ] ) * 0.07), projection, dbgImage.rows);
	}

	cv::Scalar colors[3] = {cv::Scalar(255, 0, 0), cv::Scalar(0, 255, 0), cv::Scalar(0, 0, 255)};

	// draw some lines
	for ( int i = 1; i < 4; i++ ) {
		cv::line ( dbgImage, cvPoint ( p2D[0](0), p2D[0](1)), cvPoint ( p2D[i](0), p2D[i](1)),
			colors[i-1], 4);
	}

	// draw circle representing uncertainty
	cv::circle ( dbgImage, cvPoint ( p2D[0](0), p2D[0](1)), cvRound( dbgImage.cols / 50.0 ), Ubitrack::Vision::getGradientRampColor (error, 0.0, 100.0), 4);
}

void drawPosition ( cv::Mat& dbgImage, const Math::Vector< double, 3 > &position, const Math::Matrix< double, 3, 3 >& projection, double error )
{
// the corner points
	static float points3D[ 6 ][ 3 ] = {
		{ -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f },
	};

    const Math::Pose pose ( Math::Quaternion(), position );

	// project points
	Math::Vector< double, 3 > p2D[ 6 ];
	for ( int i = 0; i < 6; i++ )
	{
		p2D[ i] = projectPoint ( pose * (Math::Vector< float, 3 >( points3D[ i ] ) * 0.07), projection, dbgImage.rows);
	}


	// draw 3D-crosshair with color representing uncertainty
	for ( int i = 0; i < 3; i++ ) {
		cv::line ( dbgImage, cvPoint ( p2D[i*2](0), p2D[i*2](1)), cvPoint ( p2D[i*2+1](0), p2D[i*2+1](1)),
			Ubitrack::Vision::getGradientRampColor (error, 0.0, 100.0), 4);
	}	
		
}

}} // namespace Ubitrack::Vision
