#ifndef FREAK_FEATURE_H
#define FREAK_FEATURE_H

#include "FeatureDescriptor.h"

namespace Ubitrack
{
namespace Vision
{

class UTVISION_EXPORT cvFREAKFeature  :
    public OpenCVFeature
{
public:
    cvFREAKFeature(cv::KeyPoint &keypoint, cv::Mat &descriptor);

};

class UTVISION_EXPORT cvFREAKFeatureMatcher :
    public OpenCVFeatureMatcher
{
    virtual FeatureType featureType();
    virtual void match2D2D(FeaturePoint2DVector arg1, FeaturePoint2DVector arg2, std::vector<MatchPair2D2D> &matches);
    virtual void match2D3D(FeaturePoint2DVector arg1, FeaturePoint3DVector arg2, std::vector<MatchPair2D3D> &matches);
};

}
}

#endif /* FREAK_FEATURE_H */