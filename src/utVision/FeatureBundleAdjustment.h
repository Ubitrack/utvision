#ifndef FEATURE_BUNDLE_ADJUSTMENT_H
#define FEATURE_BUNDLE_ADJUSTMENT_H

#include <utMath/Vector.h>
#include <utMath/Matrix.h>

namespace Ubitrack
{
namespace Vision
{

template <typename T>
class FeatureBundleAdjustment
{
public:
    FeatureBundleAdjustment(){};
    virtual bool run(const Ubitrack::Math::Vector< unsigned, 2 > &screenResolution,
                     std::vector < Ubitrack::Math::Vector < T, 3 > > &points,
                     std::vector < std::vector< Ubitrack::Math::Vector< T, 2 > > > &imagePoints,
                     std::vector < Ubitrack::Math::Matrix< T, 3, 3 > > &cameraMatrix,
                     std::vector < Ubitrack::Math::Matrix< T, 3, 3 > > &rotation,
                     std::vector < Ubitrack::Math::Vector< T, 3 > > &translation) = 0;
    virtual ~FeatureBundleAdjustment(){};
};

}
}

#endif /* FEATURE_BUNDLE_ADJUSTMENT_H */