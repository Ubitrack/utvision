#include "RobustFeatureMatching.h"
#include "FeatureDescriptor.h"

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/core/core.hpp>

#include "log4cpp/Category.hh"

namespace Ubitrack
{
namespace Vision
{

static log4cpp::Category &logger( log4cpp::Category::getInstance( "Ubitrack.Vision.RobustFetureMatching" ) );

RobustFetureMatching::RobustFetureMatching(Vision::FeaturePoint2DVector &iSrcFeaturepoints,
        Vision::FeaturePoint2DVector &iDescfeaturePoints,
        double iRatio,
        double iConfidence,
        double iDistance,
        double iRansacReprojThreshold,
        int iThresholdNumFeatures) :
    srcFeaturepoints(iSrcFeaturepoints),
    descFeaturePoints(iDescfeaturePoints),
    ratio(iRatio),
    confidence(iConfidence),
    distance(iDistance),
    ransacReprojThreshold(iRansacReprojThreshold),
    thresholdNumFeatures(iThresholdNumFeatures)
{
}


RobustFetureMatching::~RobustFetureMatching()
{

}

bool RobustFetureMatching::BFMatching(int normType,
                                      bool crossCheck,
                                      std::vector <std::vector <cv::DMatch> > &matches1,
                                      std::vector <std::vector <cv::DMatch> > &matches2)
{
	//LOG4CPP_INFO(logger, "srcFeaturepoints Size: " << srcFeaturepoints.size());
	//LOG4CPP_INFO(logger, "descFeaturePoints Size: " << descFeaturePoints.size());
    const int k = 2;
    cv::BFMatcher matcher(normType, crossCheck);

    cv::Mat srcDescriptors;
    cv::Mat descDescriptors;
    Vision::OpenCVFeature *cvFeature;

    for (std::size_t i(0); i < srcFeaturepoints.size(); i++)
    {
        cvFeature = ((Vision::OpenCVFeature *)(srcFeaturepoints[i].getFeature()).get());
        srcDescriptors.push_back(cvFeature->m_descriptor);
    }

    for (std::size_t i(0); i < descFeaturePoints.size(); i++)
    {
        cvFeature = ((Vision::OpenCVFeature *)(descFeaturePoints[i].getFeature()).get());
        descDescriptors.push_back(cvFeature->m_descriptor);
    }
    LOG4CPP_INFO(logger, "Source Descriptors Size: " << srcDescriptors.size());
    LOG4CPP_INFO(logger, "Destination Descriptors Size: " << descDescriptors.size());

    matcher.knnMatch(srcDescriptors, descDescriptors, matches1, k);
    LOG4CPP_INFO(logger, "Size of KNN Match Between Left to Right: " << matches1.size());
    matcher.knnMatch(descDescriptors, srcDescriptors, matches2, k);
    LOG4CPP_INFO(logger, "Size of KNN Match Between Right to Left: " << matches2.size());

    if (matches1.size() >= thresholdNumFeatures && matches2.size() >= thresholdNumFeatures)
        return true;
    else
        return false;
}


bool RobustFetureMatching::nearestNeighbourDistance(std::vector <std::vector <cv::DMatch> > &matches1,
        std::vector <std::vector <cv::DMatch> > &matches2,
        std::vector <cv::DMatch> &NNMatches1,
        std::vector <cv::DMatch> &NNMatches2)
{

    for (size_t i(0); i < matches1.size(); i++)
    {
        // cv::DMatch current = matches1[i][0];
        float dist1 = matches1[i][0].distance;
        float dist2 = matches1[i][1].distance;

        if (dist1 < ratio * dist2)
        {
            NNMatches1.push_back(matches1[i][0]);
        }
    }

    for (size_t i(0); i < matches2.size(); i++)
    {
        float dist1 = matches2[i][0].distance;
        float dist2 = matches2[i][1].distance;

        if ( dist1 < ratio * dist2)
        {
            NNMatches2.push_back(matches2[i][0]);
        }
    }


    // for (std::vector <std::vector <cv::DMatch> >::iterator matchIterator = matches1.begin(); matchIterator != matches1.end(); matchIterator ++)
    // {
    //     if (matchIterator->size() > 1)
    //     {
    //         if ( ((*matchIterator)[0].distance / (*matchIterator)[1].distance) > ratio)
    //         {
    //             matchIterator->clear(); // remove match
    //         }
    //     }
    //     else
    //     {
    //         matchIterator->clear(); // remove match
    //     }
    // }
    LOG4CPP_INFO(logger, "Size of NND Match Between Left to Right: " << NNMatches1.size());

    // for (std::vector <std::vector <cv::DMatch> >::iterator matchIterator = matches2.begin(); matchIterator != matches2.end(); matchIterator ++)
    // {
    //     if (matchIterator->size() > 1)
    //     {
    //         // check distance ratio
    //         if ( ((*matchIterator)[0].distance / (*matchIterator)[1].distance) > ratio)
    //         {
    //             matchIterator->clear(); // remove match
    //         }
    //     }
    //     else
    //     {
    //         // does not have 2 neighbours
    //         matchIterator->clear(); // remove match
    //     }
    // }
    LOG4CPP_INFO(logger, "Size of NND Match Between Right to Left: " << NNMatches2.size());

    if (NNMatches1.size() >= thresholdNumFeatures && NNMatches2.size() >= thresholdNumFeatures)
        return true;
    else
        return false;
}

bool RobustFetureMatching::symmetricMatching( std::vector <cv::DMatch> &NNMatches1,
        std::vector <cv::DMatch> &NNMatches2,
        std::vector<cv::DMatch> &symMatches)
{

    for (size_t i(0); i < NNMatches1.size(); i++)
    {
        for (size_t j(0); j < NNMatches2.size(); j++)
        {
            if (NNMatches1[i].queryIdx == NNMatches2[j].trainIdx && NNMatches1[i].trainIdx == NNMatches2[j].queryIdx)
            {
                symMatches.push_back(cv::DMatch(NNMatches1[i].queryIdx,
                                                NNMatches1[i].trainIdx,
                                                NNMatches1[i].distance));
                break;
            }
        }
    }


    // for (std::vector <std::vector <cv::DMatch> >::iterator matchIterator1 = matches1.begin(); matchIterator1 != matches1.end(); matchIterator1 ++)
    // {
    //     // ignore deleted matches
    //     if (matchIterator1->size() < 2)
    //         continue;
    //     // for all matches image 2 -> image 1
    //     for (std::vector <std::vector <cv::DMatch> >::iterator matchIterator2 = matches2.begin(); matchIterator2 != matches2.end(); matchIterator2 ++)
    //     {
    //         // ignore deleted matches
    //         if (matchIterator2->size() < 2)
    //             continue;
    //         // Match symmetry test
    //         if ((*matchIterator1)[0].queryIdx ==
    //                 (*matchIterator2)[0].trainIdx  &&
    //                 (*matchIterator2)[0].queryIdx ==
    //                 (*matchIterator1)[0].trainIdx)
    //         {
    //             // add symmetrical match
    //             symMatches.push_back(
    //                 cv::DMatch((*matchIterator1)[0].queryIdx,
    //                            (*matchIterator1)[0].trainIdx,
    //                            (*matchIterator1)[0].distance));
    //             break; // next match in image 1 -> image 2
    //         }
    //     }
    // }
    LOG4CPP_INFO(logger, "Size of Symmetric Matches: " << symMatches.size());

    if (symMatches.size() >= thresholdNumFeatures)
        return true;
    else
        return false;
}

bool RobustFetureMatching::fundamentalMatching(std::vector<cv::DMatch> &symMatches)
{
    std::vector<cv::Point2f> points1;
    std::vector<cv::Point2f> points2;
    findVecPointFromCvMatch(symMatches, points1, points2);
    std::vector<uchar> inliers(points1.size(), 0);
    cv::Mat fundemental = cv::findFundamentalMat(
                              cv::Mat(points1), cv::Mat(points2), // matching points
                              cv::FM_RANSAC, // RANSAC method
                              distance,     // distance to epipolar line
                              confidence,   // confidence probability
                              inliers);    // match status (inlier or outlier)

    std::vector<cv::Point2f> inlierPoints1, inlierPoints2;

    int inxMatcher = 0;
    MatchPair2D2D entry;
    for (std::vector<uchar>::const_iterator inlierIterator = inliers.begin(); inlierIterator != inliers.end(); inlierIterator ++)
    {
        if (*inlierIterator)
        {
            entry.first = symMatches[inxMatcher].queryIdx;
            entry.second = symMatches[inxMatcher].trainIdx;
            matches.push_back(entry);
            inlierPoints1.push_back(points1[inxMatcher]);
            inlierPoints2.push_back(points2[inxMatcher]);
        }
        inxMatcher++;
    }
    LOG4CPP_INFO(logger, "Size of Fundamental Restriction Matching: " << matches.size());

    try
    {
        homography = cv::findHomography(cv::Mat(inlierPoints1),
                                        cv::Mat(inlierPoints2),
                                        cv::FM_RANSAC,
                                        ransacReprojThreshold);

    }
    catch (std::exception &e)
    {
        LOG4CPP_WARN(logger, "exception caught: " << e.what());
    }

    if (matches.size() >= thresholdNumFeatures)
        return true;
    else
        return false;
}


std::vector < std::pair<FeaturePoint2DVector::size_type, FeaturePoint3DVector::size_type> > RobustFetureMatching::getMatches()
{
    return matches;
}

cv::Mat RobustFetureMatching::getHomography()
{
    return homography;
}

void RobustFetureMatching::findVecPointFromCvMatch (std::vector<cv::DMatch> &cvMatches,
        std::vector<cv::Point2f> &points1,
        std::vector<cv::Point2f> &points2)
{
    // std::vector<cv::Point2f> points1, points2;
    Vision::OpenCVFeature *cvFeature;
    for (std::size_t i(0); i < cvMatches.size(); i++)
    {
        int queryIdx = cvMatches[i].queryIdx;
        cvFeature = ((Vision::OpenCVFeature *)(srcFeaturepoints[queryIdx].getFeature()).get());
        cv::KeyPoint srcKeypoints = cvFeature->m_keypoint;
        // Get the position of left keypoints
        float x = srcKeypoints.pt.x;
        float y = srcKeypoints.pt.y;
        points1.push_back(cv::Point2f(x, y));

        int trainIdx = cvMatches[i].trainIdx;
        cvFeature = ((Vision::OpenCVFeature *)(descFeaturePoints[trainIdx].getFeature()).get());
        cv::KeyPoint descKeypoints = cvFeature->m_keypoint;
        // Get the position of right keypoints
        x = descKeypoints.pt.x;
        y = descKeypoints.pt.y;
        points2.push_back(cv::Point2f(x, y));
    }
}

void RobustFetureMatching::convertCvMatch2MatchPair2D2D (std::vector<cv::DMatch> &cvMatches)
{
    for (size_t i(0); i < cvMatches.size(); i++)
    {
        MatchPair2D2D entry;
        entry.first = cvMatches[i].queryIdx;
        entry.second = cvMatches[i].trainIdx;
        matches.push_back(entry);
    }

    std::vector<cv::Point2f> inlierPoints1;
    std::vector<cv::Point2f> inlierPoints2;
    findVecPointFromCvMatch(cvMatches, inlierPoints1, inlierPoints2);
    homography = cv::findHomography(cv::Mat(inlierPoints1),
                                    cv::Mat(inlierPoints2),
                                    cv::FM_RANSAC,
                                    ransacReprojThreshold);
}

RobustFetureMatchingBitVecFeatureBase::RobustFetureMatchingBitVecFeatureBase(Vision::FeaturePoint2DVector &iSrcFeaturepoints,
        Vision::FeaturePoint2DVector &iDescfeaturePoints,
        double iRatio,
        double iConfidence,
        double iDistance,
        double iRansacReprojThreshold,
        int iThresholdNumFeatures):
    RobustFetureMatching (iSrcFeaturepoints, iDescfeaturePoints, iRatio, iConfidence, iDistance, iRansacReprojThreshold, iThresholdNumFeatures)
{

}


RobustFetureMatchingBitVecFeatureBase::~RobustFetureMatchingBitVecFeatureBase()
{

}


void RobustFetureMatchingBitVecFeatureBase::run()
{
    int normType = cv::NORM_HAMMING2;
    bool crossCheck = false;
    std::vector <std::vector <cv::DMatch> > matches1;
    std::vector <std::vector <cv::DMatch> > matches2;
    std::vector <cv::DMatch> NNMatches1;
    std::vector <cv::DMatch> NNMatches2;
    // BFMatching(normType, crossCheck, matches1, matches2);
    // nearestNeighbourDistance (matches1, matches2);
    // std::vector<cv::DMatch> symMatches;
    // symmetricMatching (matches1, matches2, symMatches);
    // fundamentalMatching (symMatches);

    if (BFMatching(normType, crossCheck, matches1, matches2))
    {
        if (nearestNeighbourDistance (matches1, matches2, NNMatches1, NNMatches2))
        {
            std::vector<cv::DMatch> symMatches;
            if (symmetricMatching (NNMatches1, NNMatches2, symMatches))
            {
                fundamentalMatching (symMatches);
            }
            else
            {
                convertCvMatch2MatchPair2D2D(symMatches);
            }
        }
        else
        {
            convertCvMatch2MatchPair2D2D(NNMatches1);
        }
    }
    else
    {
        LOG4CPP_WARN(logger, "Error in loading of images or not enough features in one of images");
    }
}

RobustFetureMatchingFloatFeatureBase::RobustFetureMatchingFloatFeatureBase(Vision::FeaturePoint2DVector &iSrcFeaturepoints,
        Vision::FeaturePoint2DVector &iDescfeaturePoints,
        double iRatio,
        double iConfidence,
        double iDistance,
        double iRansacReprojThreshold,
        int iThresholdNumFeatures):
    RobustFetureMatching (iSrcFeaturepoints, iDescfeaturePoints, iRatio, iConfidence, iDistance, iRansacReprojThreshold, iThresholdNumFeatures)
{

}


RobustFetureMatchingFloatFeatureBase::~RobustFetureMatchingFloatFeatureBase()
{

}


void RobustFetureMatchingFloatFeatureBase::run()
{
    int normType = cv::NORM_L2;
    bool crossCheck = false;
    std::vector <std::vector <cv::DMatch> > matches1;
    std::vector <std::vector <cv::DMatch> > matches2;
    std::vector <cv::DMatch> NNMatches1;
    std::vector <cv::DMatch> NNMatches2;
    if (BFMatching(normType, crossCheck, matches1, matches2))
    {
        if (nearestNeighbourDistance (matches1, matches2, NNMatches1, NNMatches2))
        {
            std::vector<cv::DMatch> symMatches;
            if (symmetricMatching (NNMatches1, NNMatches2, symMatches))
            {
                fundamentalMatching (symMatches);
            }
            else
            {
                convertCvMatch2MatchPair2D2D(symMatches);
            }
        }
        else
        {
            convertCvMatch2MatchPair2D2D(NNMatches1);
        }
    }
    else
    {
        LOG4CPP_WARN(logger, "Error in loading of images or not enough features in one of images");
    }
}

}
}