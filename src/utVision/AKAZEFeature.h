#ifndef AKAZE_FEATURE_H
#define AKAZE_FEATURE_H

#include "FeatureDescriptor.h"

namespace Ubitrack
{
namespace Vision
{

class UTVISION_EXPORT cvAKAZEFeature  :
    public OpenCVFeature
{
public:
    cvAKAZEFeature(cv::KeyPoint &keypoint, cv::Mat &descriptor);

};

class UTVISION_EXPORT cvAKAZEFeatureMatcher :
    public OpenCVFeatureMatcher
{

    typedef std::pair<FeaturePoint2DVector::size_type, FeaturePoint3DVector::size_type> MatchPair2D3D;
    typedef std::pair<FeaturePoint2DVector::size_type, FeaturePoint2DVector::size_type> MatchPair2D2D;
    typedef std::pair<FeaturePoint3DVector::size_type, FeaturePoint3DVector::size_type> MatchPair3D3D;

    virtual FeatureType featureType();
    virtual void match2D2D(FeaturePoint2DVector arg1, FeaturePoint2DVector arg2, std::vector<MatchPair2D2D> &matches);
	virtual void match2D3D(FeaturePoint2DVector arg1, FeaturePoint3DVector arg2, std::vector<MatchPair2D3D> &matches, Math::Matrix3x4d projectionMatrix);

public:
    cv::Mat homography;
};

}
}

#endif /* AKAZE_FEATURE_H */