#ifndef __DEBUGGING2_UBITRACK_H_INCLUDED__
#define __DEBUGGING2_UBITRACK_H_INCLUDED__

#include <opencv2/core/core.hpp>
#include <utMath/Matrix.h>
#include <utMath/MatrixOperations.h>
#include <utMath/Vector.h>
#include <utVision.h>


namespace Ubitrack { namespace Vision {

using namespace cv;

UTVISION_EXPORT void drawPoseCube( Mat & img, const Math::Pose& pose, const Math::Matrix< 3, 3, float >& K, double scale, CvScalar color, bool paintCoordSystem = false );
UTVISION_EXPORT Math::Vector< double, 3 > projectPoint ( Math::Vector < double, 3 > pt, Math::Matrix < 3, 3 > projection, int imageHeight);
UTVISION_EXPORT void drawPose ( cv::Mat & dbgImage, Math::Pose pose, Math::Matrix < 3, 3 > projection, double error );
UTVISION_EXPORT void drawPosition ( cv::Mat & dbgImage, Math::Vector< double, 3 > position, Math::Matrix < 3, 3 > projection, double error );

}} // End of namespace Ubitrack::Vision

#endif
