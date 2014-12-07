#ifndef ROBUST_FEATURE_MATCHING_H
#define ROBUST_FEATURE_MATCHING_H

#include "FeatureDescriptor.h"

namespace Ubitrack
{
namespace Vision
{

class UTVISION_EXPORT RobustFetureMatching
{
public:

    typedef std::pair<FeaturePoint2DVector::size_type, FeaturePoint3DVector::size_type> MatchPair2D3D;
    typedef std::pair<FeaturePoint2DVector::size_type, FeaturePoint2DVector::size_type> MatchPair2D2D;
    typedef std::pair<FeaturePoint3DVector::size_type, FeaturePoint3DVector::size_type> MatchPair3D3D;

    RobustFetureMatching(Vision::FeaturePoint2DVector &iSrcFeaturepoints,
                         Vision::FeaturePoint2DVector &iDescfeaturePoints,
                         double iRatio = 0.8,
                         double iConfidence = 0.99,
                         double iDistance = 1.0,
                         double iRansacReprojThreshold = 2.5,
                         int iThresholdNumFeatures = 4);

    virtual ~RobustFetureMatching();
    virtual bool BFMatching(int normType,
                            bool crossCheiKk,
                            std::vector <std::vector <cv::DMatch> > &matches1,
                            std::vector <std::vector <cv::DMatch> > &matches2);

    virtual bool nearestNeighbourDistance(std::vector <std::vector <cv::DMatch> > &matches1,
                                          std::vector <std::vector <cv::DMatch> > &matches2,
                                          std::vector <cv::DMatch> &NNMatches1,
                                          std::vector <cv::DMatch> &NNMAtches2);

    virtual bool symmetricMatching(std::vector <cv::DMatch> &NNMatches1,
                                   std::vector <cv::DMatch> &NNMatches2,
                                   std::vector<cv::DMatch> &symMatches);

    virtual bool fundamentalMatching(std::vector<cv::DMatch> &symMatches);
    virtual void run() = 0;
    virtual void convertCvMatch2MatchPair2D2D (std::vector<cv::DMatch> &cvMatches);
    virtual std::vector < MatchPair2D2D > getMatches();
    cv::Mat getHomography();

    void findVecPointFromCvMatch (std::vector<cv::DMatch> &cvMatches,
                                  std::vector<cv::Point2f> &points1,
                                  std::vector<cv::Point2f> &points2);

private:
    Vision::FeaturePoint2DVector srcFeaturepoints;
    Vision::FeaturePoint2DVector descFeaturePoints;
    std::vector < MatchPair2D2D > matches;
    cv::Mat homography;
    double ratio;
    double confidence;
    double distance;
    double ransacReprojThreshold;
    int thresholdNumFeatures;
};

class UTVISION_EXPORT RobustFetureMatchingBitVecFeatureBase : public RobustFetureMatching
{
public:
    RobustFetureMatchingBitVecFeatureBase (Vision::FeaturePoint2DVector &iSrcFeaturepoints,
                                           Vision::FeaturePoint2DVector &iDescfeaturePoints,
                                           double iRatio = 0.8,
                                           double iConfidence = 0.99,
                                           double iDistance = 1.0,
                                           double iRansacReprojThreshold = 2.5,
                                           int iThresholdNumFeatures = 4);


    virtual void run();

    virtual ~RobustFetureMatchingBitVecFeatureBase();
};

class UTVISION_EXPORT RobustFetureMatchingFloatFeatureBase : public RobustFetureMatching
{
public:
    RobustFetureMatchingFloatFeatureBase (Vision::FeaturePoint2DVector &iSrcFeaturepoints,
                                          Vision::FeaturePoint2DVector &iDescfeaturePoints,
                                          double iRatio = 0.8,
                                          double iConfidence = 0.99,
                                          double iDistance = 1.0,
                                          double iRansacReprojThreshold = 2.5,
                                          int iThresholdNumFeatures = 4);
    virtual void run();

    virtual ~RobustFetureMatchingFloatFeatureBase();
};

}
}

#endif /* ROBUST_FEATURE_MATCHING_H */