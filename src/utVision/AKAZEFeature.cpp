#include "AKAZEFeature.h"
#include "RobustFeatureMatching.h"
#include <utVision/RobustFeatureMatching.h>
#include <opencv2/core.hpp>

#include <utMath/Geometry/PointProjection.h>

namespace Ubitrack
{
namespace Vision
{

cvAKAZEFeature::cvAKAZEFeature(cv::KeyPoint &keypoint, cv::Mat &descriptor)
    : OpenCVFeature(keypoint, descriptor)
{
    if (!FeatureMatcher::hasMatcher<cvAKAZEFeature>())
    {
        boost::shared_ptr<FeatureMatcher> ptr(new cvAKAZEFeatureMatcher());
        FeatureMatcher::registerMatcher(ptr);
    }
}


FeatureType cvAKAZEFeatureMatcher::featureType()
{
    return (FeatureType)&typeid(cvAKAZEFeature);
}

void cvAKAZEFeatureMatcher::match2D2D(FeaturePoint2DVector arg1, FeaturePoint2DVector arg2, std::vector<MatchPair2D2D> &matches)
{
    Ubitrack::Vision::RobustFetureMatchingBitVecFeatureBase matcher(arg1, arg2, 0.65, 0.99, 1.0);
    matcher.run();
    matches = matcher.getMatches();
    homography = matcher.getHomography();
}


void cvAKAZEFeatureMatcher::match2D3D(FeaturePoint2DVector arg1, FeaturePoint3DVector arg2, std::vector<MatchPair2D3D> &matches, Math::Matrix3x4d projectionMatrix)
{
	/*
	std::vector<Math::Vector2d> point2D;

	std::vector<Math::Vector3d> point3D;

	std::cout << "asdf" << std::endl;

	for (size_t i(0); i< arg2.size(); i++)
	{

		point3D.push_back(arg2[i]);
	}

	std::cout << "1" << std::endl;

	Ubitrack::Math::Geometry::project_points(projectionMatrix, point3D.begin(), point3D.end(), std::back_inserter(point2D));

	std::cout << "2" << std::endl;

	FeaturePoint2DVector projectedPoints;
	for (size_t i(0); i< point2D.size(); i++)
	{
		std::cout << point2D[i](0) << " : " << point2D[i](1) << std::endl;
		projectedPoints.push_back(FeaturePoint2D(point2D[i], arg2[i].getFeature()));
	}

	std::cout << "3" << std::endl;
	//std::vector<MatchPair2D2D> tmpmatches;
	match2D2D(arg1, projectedPoints, matches);

	std::cout << "4" << std::endl;
	*/
}

}
}