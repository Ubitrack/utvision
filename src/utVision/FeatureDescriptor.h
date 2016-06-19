/*
 * Ubitrack - Library for Ubiquitous Tracking
 * Copyright 2006, Technische Universitaet Muenchen, and individual
 * contributors as indicated by the @authors tag. See the
 * copyright.txt in the distribution for a full listing of individual
 * contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */

/**
 * @ingroup vision
 * @file
 * Measurement function for edge-based tracking.
 *
 * @author
 */

#ifndef __UBITRACK_VISION_FEATUREDESCRIPTOR_H_INCLUDED__
#define __UBITRACK_VISION_FEATUREDESCRIPTOR_H_INCLUDED__

#include <typeinfo>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/core.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <utMeasurement/Measurement.h>
#include <utMath/Pose.h>
#include <utVision.h>

namespace Ubitrack
{
namespace Vision
{

#define FEATURE_MAX_DIFFERNT 0x7ffffff;

typedef const std::type_info *FeatureType;

class FeatureMatcher;

class UTVISION_EXPORT FeatureBase
{
public:
    /* Note: we need at least one virtual method to be able to use dynamic_cast<>... */

    /* to avoid a warning */
    virtual ~FeatureBase() {};

    virtual bool isSame(boost::shared_ptr<FeatureBase> other)
    {
        return ((other) ? isSame(other->getType()) : false);
    }
    virtual bool isSame(FeatureType featureType) = 0;

    FeatureType getType();
};

class UTVISION_EXPORT Feature :
    public FeatureBase
{
public:
    Feature();

    Math::Pose m_pose;

    virtual bool isSame(boost::shared_ptr<FeatureBase> other)
    {
        return ((other) ? isSame(other->getType()) : false);
    };
    virtual bool isSame(FeatureType featureType);

    virtual std::pair <unsigned long long int, unsigned char> getParameters();

protected:
    friend class ::boost::serialization::access;
    template< class Archive >
    void serialize( Archive &ar, const unsigned int version )
    {
        ar &m_pose;
    }
};

class UTVISION_EXPORT FeatureList :
    public Feature
{
public:
    std::vector<boost::shared_ptr<Feature> > m_features;

    virtual bool isSame(boost::shared_ptr<FeatureBase> other);
    virtual bool isSame(FeatureType featureType);

    virtual std::pair <unsigned long long int, unsigned char> getParameters();
};

template< class T, int N > class FeaturePoint
    : public Math::Vector< T, N >
{
public:
    FeaturePoint() : Math::Vector<T, N>()
        , m_feature()
    { }

    FeaturePoint(boost::shared_ptr<FeatureBase> feature) : Math::Vector<T, N>()
        , m_feature(feature)
    { }

    FeaturePoint(Math::Vector<T, N> &point, boost::shared_ptr<FeatureBase> feature) : Math::Vector<T, N>(point)
        , m_feature(feature)
    { }

    FeaturePoint(const FeaturePoint<T, N> &feature) : Math::Vector<T, N>(feature)
        , m_feature(feature.m_feature)
    { }

private:
    boost::shared_ptr<FeatureBase> m_feature;

public:
    boost::shared_ptr<FeatureBase> getFeature()
    {
        return m_feature;
    }

    bool isSameFeature(boost::shared_ptr<FeatureBase> other)
    {
        return m_feature->isSame(other);
    }

protected:
    friend class ::boost::serialization::access;
    template< class Archive >
    void serialize( Archive &ar, const unsigned int version )
    {
        // Ubitrack::Vision::MarkerFeature temp;
        FeatureBase *data = m_feature.get();
        FeatureList *temp = dynamic_cast<FeatureList *> (data);
        std::vector<boost::shared_ptr<Feature> > rawdata = temp->m_features;
        // for (int i = 0; i < rawdata.size(); i++)
        // {
        Feature *localData = rawdata[0].get();
        std::pair <unsigned long long int, unsigned char> params = localData->getParameters();
        // char buffer [20];
        // std::itoa (params.first, buffer, 16);
        std::stringstream sstream;
        sstream << std::hex << params.first;
        std::string result = sstream.str();
        ar &result &params.second;
        // }
        // MarkerFeature * markerData;
        // markerData = dynamic_cast<MarkerFeature *>(data)
        // ar &params.first &params.second;
        ar &boost::serialization::base_object< Math::Vector< T, N > >(*this);
        // ar &temp->m_pose; // & temp->m_markerID & temp->m_cornerID;
    }

};

/* convenience typedefs */
typedef FeaturePoint<double, 3> FeaturePoint3D;
typedef FeaturePoint<double, 2> FeaturePoint2D;
typedef std::vector<FeaturePoint<double, 3> > FeaturePoint3DVector;
typedef std::vector<FeaturePoint<double, 2> > FeaturePoint2DVector;

class UTVISION_EXPORT FeatureMatcher
{
public:
    /* to avoid a warning */
    virtual ~FeatureMatcher() {};

    typedef std::pair<FeaturePoint2DVector::size_type, FeaturePoint3DVector::size_type> MatchPair2D3D;
    typedef std::pair<FeaturePoint2DVector::size_type, FeaturePoint2DVector::size_type> MatchPair2D2D;
    typedef std::pair<FeaturePoint3DVector::size_type, FeaturePoint3DVector::size_type> MatchPair3D3D;

    virtual void match2D2D(FeaturePoint2DVector arg1, FeaturePoint2DVector arg2, std::vector<MatchPair2D2D> &matches) = 0;

    virtual void match2D3D(FeaturePoint2DVector arg1, FeaturePoint3DVector arg2, std::vector<MatchPair2D3D> &matches, Math::Matrix3x4d projectionMatrix) = 0;

    virtual FeatureType featureType() = 0;

    /* methods for registering */
    static void registerMatcher(boost::shared_ptr<FeatureMatcher> matcher);
    static void unregisterMatcher(boost::shared_ptr<FeatureMatcher> matcher);
    static void unregisterMatcher(FeatureType featureType);
    static bool hasMatcher(FeatureType featureType);
    static boost::shared_ptr<FeatureMatcher> getMatcher(FeatureType featureType);

    /* convenience overloads */
    template<class T>
    static void unregisterMatcher()
    {
        unregisterMatcher(&typeid(T));
    }
    template<class T>
    static bool hasMatcher()
    {
        return hasMatcher(&typeid(T));
    }
    template<class T>
    static boost::shared_ptr<FeatureMatcher> getMatcher()
    {
        return getMatcher(&typeid(T));
    }

    /* sort helpers */
    static bool compareMatchPair2D2DFirst(MatchPair2D2D left, MatchPair2D2D right);
    static bool compareMatchPair2D2DSecond(MatchPair2D2D left, MatchPair2D2D right);
    static bool compareMatchPair2D3DFirst(MatchPair2D3D left, MatchPair2D3D right);
    static bool compareMatchPair2D3DSecond(MatchPair2D3D left, MatchPair2D3D right);

protected:
    typedef std::pair<FeaturePoint2DVector::size_type, FeaturePoint2DVector::size_type> IndexMapping2D;
    typedef std::pair<FeaturePoint3DVector::size_type, FeaturePoint3DVector::size_type> IndexMapping3D;

    void filterFeatures(FeaturePoint2DVector points_in, FeaturePoint2DVector &points_out, std::vector<IndexMapping2D> &mapping_out);
    void filterFeatures(FeaturePoint3DVector points_in, FeaturePoint3DVector &points_out, std::vector<IndexMapping3D> &mapping_out);

    /* sort helpers */
    static bool compareIndexMapping3DFirst(IndexMapping3D left, IndexMapping3D right);
    static bool compareIndexMapping2DFirst(IndexMapping2D left, IndexMapping2D right);
    static bool compareIndexMapping3DSecond(IndexMapping3D left, IndexMapping3D right);
    static bool compareIndexMapping2DSecond(IndexMapping2D left, IndexMapping2D right);

private:
    static std::vector<boost::shared_ptr<FeatureMatcher> > matchers;
};

/*
 * specific features
 */

class UTVISION_EXPORT OpenCVFeature :
    public Feature
{
public:

    OpenCVFeature(cv::KeyPoint &keypoint, cv::Mat &descriptor);

    /// change to private
public:

    cv::KeyPoint m_keypoint;
    cv::Mat m_descriptor;

    friend class OpenCVFeatureMatcher;

};

class UTVISION_EXPORT OpenCVFeatureMatcher :
    public FeatureMatcher
{
public:
    virtual void match2D2D(FeaturePoint2DVector arg1, FeaturePoint2DVector arg2, std::vector<MatchPair2D2D> &matches);

	virtual void match2D3D(FeaturePoint2DVector arg1, FeaturePoint3DVector arg2, std::vector<MatchPair2D3D> &matches, Math::Matrix3x4d projectionMatrix);

    // override in Feature specific subclass!
    //virtual FeatureType featureType();
};

class UTVISION_EXPORT cvSURFFeatureMatcher :
    public OpenCVFeatureMatcher
{
    virtual FeatureType featureType();
};

class UTVISION_EXPORT cvSURFFeature :
    public OpenCVFeature
{
public:
    cvSURFFeature(cv::KeyPoint &keypoint, cv::Mat &descriptor);

};

class UTVISION_EXPORT cvGFTTFeatureMatcher :
    public OpenCVFeatureMatcher
{
    virtual FeatureType featureType();
};

class UTVISION_EXPORT cvGFTTFeature :
    public OpenCVFeature
{
public:
    cvGFTTFeature(cv::KeyPoint &keypoint, cv::Mat &descriptor);

};

} // namespace Vision

namespace Measurement
{
/** define a shortcut for image measurements */
typedef Measurement< Ubitrack::Vision::FeaturePoint<double, 2> > FeaturePoint2D;
typedef Measurement< Ubitrack::Vision::FeaturePoint<double, 3> > FeaturePoint3D;
typedef Measurement< std::vector<Ubitrack::Vision::FeaturePoint<double, 2> > > FeaturePoint2DList;
typedef Measurement< std::vector<Ubitrack::Vision::FeaturePoint<double, 3> > > FeaturePoint3DList;
} // namespace Measurement

} // namespace Ubitrack

#endif
