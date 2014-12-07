#include "FreakFeature.h"
#include "RobustFeatureMatching.h"
#include <utVision/RobustFeatureMatching.h>

namespace Ubitrack { namespace Vision {

cvFREAKFeature::cvFREAKFeature(cv::KeyPoint& keypoint, cv::Mat& descriptor)
	: OpenCVFeature(keypoint, descriptor)
{ 
	if (!FeatureMatcher::hasMatcher<cvFREAKFeature>()) {
		boost::shared_ptr<FeatureMatcher> ptr(new cvFREAKFeatureMatcher());
		FeatureMatcher::registerMatcher(ptr);
	}
}


FeatureType cvFREAKFeatureMatcher::featureType()
{
	return (FeatureType)&typeid(cvFREAKFeature);
}

void cvFREAKFeatureMatcher::match2D2D(FeaturePoint2DVector arg1, FeaturePoint2DVector arg2, std::vector<MatchPair2D2D>& matches)
{
	Ubitrack::Vision::RobustFetureMatchingBitVecFeatureBase matcher(arg1, arg2, 0.65, 0.99, 1.0);
    matcher.run();
    matches = matcher.getMatches();	
}


void cvFREAKFeatureMatcher::match2D3D(FeaturePoint2DVector arg1, FeaturePoint3DVector arg2, std::vector<MatchPair2D3D>& matches)
{
	//TODO 
}

}}