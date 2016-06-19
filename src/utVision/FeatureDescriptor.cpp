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

#include "FeatureDescriptor.h"

#include "utUtil/Exception.h"

#include "log4cpp/Category.hh"

namespace Ubitrack { namespace Vision {

static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Vision.FeatureDescriptor" ) );

/*
 * FeatureBase
 */

FeatureType FeatureBase::getType() {
	return &typeid(*this);
}

/*
 * Feature 
 */

Feature::Feature()
	: FeatureBase(),
	m_pose()
{ 
	/* empty */
}

bool Feature::isSame(FeatureType featureType) {
	return featureType == getType() || (featureType && (std::string(featureType->name()) == std::string(getType()->name())));
}

std::pair <unsigned long long int, unsigned char> Feature::getParameters()
{
	return std::make_pair(200,200);
}

/*
 *FeatureList 
 */

bool FeatureList::isSame(boost::shared_ptr<FeatureBase> other) {
	if (other && (other->getType() == getType() || std::string(other->getType()->name()) == std::string(getType()->name()))) {
		/* compare complete list */
		FeatureList *otherlist = static_cast<FeatureList*>(other.get());
		if (otherlist->m_features.size() != m_features.size())
			return false;
		for (std::vector<boost::shared_ptr<Feature> >::size_type i = 0; i < m_features.size(); i++)
			if (!m_features[i]->isSame(otherlist->m_features[i]))
				return false;
		/* all entries matched */
		return true;
	} else {
		/* is one of the features compatible? */
		for (std::vector<boost::shared_ptr<Feature> >::iterator itr = m_features.begin(); itr != m_features.end(); itr++)
			if ((*itr)->isSame(other))
				return true;
		/* no entry matched */
		return false;
	}
}

bool FeatureList::isSame(FeatureType featureType) {
	if (featureType == getType() || (featureType && (std::string(featureType->name()) == std::string(getType()->name()))))
		/* we can not check whether an element in the list is compatible :( */
		return false;
	else
	{
		/* is one of the features compatible? */
		for (std::vector<boost::shared_ptr<Feature> >::iterator itr = m_features.begin(); itr != m_features.end(); itr++)
			if ((*itr)->isSame(featureType))
				return true;
		/* no entry matched */
		return false;
	}
}


std::pair <unsigned long long int, unsigned char> FeatureList::getParameters()
{
	return std::make_pair(100,100);
}

/*
 * FeatureMatcher
 */

void FeatureMatcher::filterFeatures(FeaturePoint2DVector points_in, FeaturePoint2DVector& points_out, std::vector<IndexMapping2D>& mapping_out)
{
	for (FeaturePoint2DVector::size_type i = 0; i < points_in.size(); i++) {
		FeaturePoint<double, 2>& itr = points_in.at(i);
		if (itr.getFeature()->isSame(featureType()))
		{
			mapping_out.push_back(IndexMapping2D(i, points_out.size()));
			points_out.push_back(itr);
		}
	}

	if (points_in.size() != 0)
		LOG4CPP_DEBUG(logger, "Filtered 2D list from " << points_in.size() << " to " << points_out.size());
}

void FeatureMatcher::filterFeatures(FeaturePoint3DVector points_in, FeaturePoint3DVector& points_out, std::vector<IndexMapping3D>& mapping_out)
{
	for (FeaturePoint3DVector::size_type i = 0; i < points_in.size(); i++) {
		FeaturePoint<double, 3>& itr = points_in.at(i);
		if (itr.getFeature()->isSame(featureType()))
		{
			mapping_out.push_back(IndexMapping3D(i, points_out.size()));
			points_out.push_back(itr);
		}
	}

	if (points_in.size() != 0)
		LOG4CPP_DEBUG(logger, "Filtered 3D list from " << points_in.size() << " to " << points_out.size());
}

bool FeatureMatcher::compareIndexMapping3DFirst(IndexMapping3D left, IndexMapping3D right) {
	return (left.first < right.first);
}

bool FeatureMatcher::compareIndexMapping2DFirst(IndexMapping2D left, IndexMapping2D right) {
	return (left.first < right.first);
}

bool FeatureMatcher::compareIndexMapping3DSecond(IndexMapping3D left, IndexMapping3D right) {
	return (left.second < right.second);
}

bool FeatureMatcher::compareIndexMapping2DSecond(IndexMapping2D left, IndexMapping2D right) {
	return (left.second < right.second);
}

bool FeatureMatcher::compareMatchPair2D2DFirst(MatchPair2D2D left, MatchPair2D2D right) {
	return (left.first < right.first);
}

bool FeatureMatcher::compareMatchPair2D2DSecond(MatchPair2D2D left, MatchPair2D2D right) {
	return (left.second < right.second);
}

bool FeatureMatcher::compareMatchPair2D3DFirst(MatchPair2D3D left, MatchPair2D3D right) {
	return (left.first < right.first);
}

bool FeatureMatcher::compareMatchPair2D3DSecond(MatchPair2D3D left, MatchPair2D3D right) {
	return (left.second < right.second);
}

std::vector<boost::shared_ptr<FeatureMatcher> > FeatureMatcher::matchers;

void FeatureMatcher::registerMatcher(boost::shared_ptr<FeatureMatcher> matcher) 
{
	if (!matcher)
		UBITRACK_THROW("'matcher' is Null");

	FeatureType type = matcher->featureType();

	/* do we already have a matcher for this feature type? */
	for (std::vector<boost::shared_ptr<FeatureMatcher> >::iterator itr = matchers.begin(); itr != matchers.end(); itr++)
		if (itr->get()->featureType() == type)
			UBITRACK_THROW("Matcher for this feature type already registered");

	LOG4CPP_DEBUG(logger, "Registering matcher for feature type " << type->name());

	matchers.push_back(matcher);
}

void FeatureMatcher::unregisterMatcher(boost::shared_ptr<FeatureMatcher> matcher)
{
	if (!matcher)
		return;
	for (std::vector<boost::shared_ptr<FeatureMatcher> >::iterator itr = matchers.begin(); itr != matchers.end(); itr++)
		if (*itr == matcher) {
			matchers.erase(itr);
			return;
		}
}

void FeatureMatcher::unregisterMatcher(FeatureType featureType)
{
	for (std::vector<boost::shared_ptr<FeatureMatcher> >::iterator itr = matchers.begin(); itr != matchers.end(); itr++)
		if (itr->get()->featureType() == featureType) {
			matchers.erase(itr);
			return;
		}
}

bool FeatureMatcher::hasMatcher(FeatureType featureType)
{        
	LOG4CPP_DEBUG(logger, "Checking for matcher for feature type " << featureType->name());
	
	for (std::vector<boost::shared_ptr<FeatureMatcher> >::iterator itr = matchers.begin(); itr != matchers.end(); itr++)
		if (itr->get()->featureType() == featureType)
			return true;

	/* for some reason the pointers to the same type information might be different, so do a name comparison as well */
	std::string featurename = std::string(featureType->name());
	for (std::vector<boost::shared_ptr<FeatureMatcher> >::iterator itr = matchers.begin(); itr != matchers.end(); itr++)
		if (std::string(itr->get()->featureType()->name()) == featurename)
			return true;

	return false;
}

boost::shared_ptr<FeatureMatcher> FeatureMatcher::getMatcher(FeatureType featureType)
{
	LOG4CPP_DEBUG(logger, "Trying to retrieve matcher for feature type " << featureType->name() << " " << (void*)featureType);

	for (std::vector<boost::shared_ptr<FeatureMatcher> >::iterator itr = matchers.begin(); itr != matchers.end(); itr++) {
		if (itr->get()->featureType() == featureType)
			return *itr;
	}

	/* for some reason the pointers to the same type information might be different, so do a name comparison as well */
	std::string featurename = std::string(featureType->name());
	for (std::vector<boost::shared_ptr<FeatureMatcher> >::iterator itr = matchers.begin(); itr != matchers.end(); itr++)
		if (std::string(itr->get()->featureType()->name()) == featurename)
			return *itr;

	return boost::shared_ptr<FeatureMatcher>();
}

/*
 * specific features
 */

OpenCVFeature::OpenCVFeature(cv::KeyPoint& keypoint, cv::Mat& descriptor)
	: Feature(),
	m_keypoint(keypoint),
	m_descriptor(descriptor)
{ }

void OpenCVFeatureMatcher::match2D2D(FeaturePoint2DVector arg1, FeaturePoint2DVector arg2, std::vector<MatchPair2D2D>& matches)
{
	std::vector<Vision::FeaturePoint<double, 2> > features2D1, features2D2;
	std::vector<IndexMapping2D> mapping2D1, mapping2D2;

	/* collect all feature points that are of our feature type */
	filterFeatures(arg1, features2D1, mapping2D1);
	filterFeatures(arg2, features2D2, mapping2D2);

	if (features2D1.size() == 0 || features2D2.size() == 0)
		return;

	/* now convert the 2D & 3D lists to OpenCV matrices */

	Vision::OpenCVFeature *cvfeature;

	cvfeature = ((Vision::OpenCVFeature*)(features2D1[0].getFeature()).get());

	int cols = cvfeature->m_descriptor.cols;
	int type = cvfeature->m_descriptor.type();

	LOG4CPP_INFO(logger, "columns: " << cvfeature->m_descriptor.cols << " type: " << cvfeature->m_descriptor.type());

	cv::Mat descriptors2D1(features2D1.size(), cols, type);

	for (FeaturePoint2DVector::size_type i = 0; i < features2D1.size(); i++)
		((Vision::OpenCVFeature*)(features2D1[i].getFeature()).get())->m_descriptor.row(0).copyTo(descriptors2D1.row(i));

	cv::Mat descriptors2D2(features2D2.size(), cols, type);

	for (FeaturePoint3DVector::size_type i = 0; i < features2D2.size(); i++)
		((Vision::OpenCVFeature*)(features2D2[i].getFeature().get()))->m_descriptor.row(0).copyTo(descriptors2D2.row(i));

	/* do the matching */

	cv::BFMatcher matcher(cv::NORM_HAMMING);
	std::vector<cv::DMatch> cvmatches;
	matcher.match(descriptors2D1, descriptors2D2, cvmatches);

	LOG4CPP_DEBUG(logger, "Found " << cvmatches.size() << " 2D<->2D correspondences");

	std::sort(mapping2D1.begin(), mapping2D1.end(), compareIndexMapping2DSecond);
	std::sort(mapping2D2.begin(), mapping2D2.end(), compareIndexMapping2DSecond);

	/* now convert the match indices back to the unmodified indices */
	for (std::vector<cv::DMatch>::iterator itrcv = cvmatches.begin(); itrcv != cvmatches.end(); itrcv++) 
	{
		MatchPair2D2D entry;
		bool found;

		LOG4CPP_INFO(logger, "query: " << itrcv->queryIdx << " train: " << itrcv->trainIdx << " distance: " << itrcv->distance);

		found = false;
		for (std::vector<IndexMapping2D>::iterator itr2D = mapping2D1.begin(); itr2D != mapping2D1.end(); itr2D++)
			if (itr2D->second == itrcv->queryIdx) {
				entry.first = itr2D->first;
				found = true;
				break;
			}
		if (!found)
			UBITRACK_THROW("Index not found in first 2D mapping");

		found = false;
		for (std::vector<IndexMapping2D>::iterator itr2D = mapping2D2.begin(); itr2D != mapping2D2.end(); itr2D++)
			if (itr2D->second == itrcv->trainIdx) {
				entry.first = itr2D->first;
				found = true;
				break;
			}
		if (!found)
			UBITRACK_THROW("Index not found in second 2D mapping");

		matches.push_back(entry);
	}
}

void OpenCVFeatureMatcher::match2D3D(FeaturePoint2DVector arg1, FeaturePoint3DVector arg2, std::vector<MatchPair2D3D>& matches, Math::Matrix3x4d projectionMatrix)
{
	std::vector<Vision::FeaturePoint<double, 3> > features3D;
	std::vector<Vision::FeaturePoint<double, 2> > features2D;
	std::vector<IndexMapping3D> mapping3D;
	std::vector<IndexMapping2D> mapping2D;

	/* collect all feature points that are of our feature type */
	filterFeatures(arg1, features2D, mapping2D);
	filterFeatures(arg2, features3D, mapping3D);

	if (features2D.size() == 0 || features3D.size() == 0)
		return;

	/* now convert the 2D & 3D lists to OpenCV matrices */

	Vision::OpenCVFeature *cvfeature;

	cvfeature = ((Vision::OpenCVFeature*)(features2D[0].getFeature()).get());

	int cols = cvfeature->m_descriptor.cols;
	int type = cvfeature->m_descriptor.type();

	cv::Mat descriptors2D(features2D.size(), cols, type);

	for (FeaturePoint2DVector::size_type i = 0; i < features2D.size(); i++)
		((Vision::OpenCVFeature*)(features2D[i].getFeature()).get())->m_descriptor.row(0).copyTo(descriptors2D.row(i));

	cv::Mat descriptors3D(features3D.size(), cols, type);

	for (FeaturePoint3DVector::size_type i = 0; i < features3D.size(); i++)
	{
		/* for now we use simply the first feature with same feature type; later we need to implement a more intelligent mapping */
		boost::shared_ptr<FeatureList> featurelist = boost::static_pointer_cast<FeatureList>(features3D[i].getFeature());
		for (std::vector<boost::shared_ptr<Feature> >::iterator itr = featurelist->m_features.begin(); itr != featurelist->m_features.end(); itr++)
		{
			if ((*itr)->isSame(features2D[0].getFeature()))
			{
				boost::static_pointer_cast<OpenCVFeature>(*itr)->m_descriptor.row(0).copyTo(descriptors3D.row(i));
				break;
			}
		}
		//((Vision::OpenCVFeature*)(features3D[i].getFeature().get()))->m_descriptor.row(0).copyTo(descriptors3D.row(i));
	}

	/* do the matching */

	//cv::FlannBasedMatcher matcher;
	cv::BFMatcher matcher;
	
	std::vector<cv::DMatch> cvmatches;
	matcher.match(descriptors2D, descriptors3D, cvmatches);

	LOG4CPP_DEBUG(logger, "Found " << cvmatches.size() << " 2D<->3D correspondences");

	std::sort(mapping2D.begin(), mapping2D.end(), compareIndexMapping2DSecond);
	std::sort(mapping3D.begin(), mapping3D.end(), compareIndexMapping3DSecond);

	/* now convert the match indices back to the unmodified indices */
	for (std::vector<cv::DMatch>::iterator itrcv = cvmatches.begin(); itrcv != cvmatches.end(); itrcv++) 
	{
		MatchPair2D3D entry;
		bool found;

		found = false;
		for (std::vector<IndexMapping2D>::iterator itr2D = mapping2D.begin(); itr2D != mapping2D.end(); itr2D++)
			if (itr2D->second == itrcv->queryIdx) {
				entry.first = itr2D->first;
				found = true;
				break;
			}
		if (!found)
			UBITRACK_THROW("Index not found in 2D mapping");

		found = false;
		for (std::vector<IndexMapping3D>::iterator itr3D = mapping3D.begin(); itr3D != mapping3D.end(); itr3D++)
			if (itr3D->second == itrcv->trainIdx) {
				entry.first = itr3D->first;
				found = true;
				break;
			}
		if (!found)
			UBITRACK_THROW("Index not found in 3D mapping");

		matches.push_back(entry);
	}
}

cvSURFFeature::cvSURFFeature(cv::KeyPoint& keypoint, cv::Mat& descriptor)
	: OpenCVFeature(keypoint, descriptor)
{ 
	if (!FeatureMatcher::hasMatcher<cvSURFFeature>()) {
		boost::shared_ptr<FeatureMatcher> ptr(new cvSURFFeatureMatcher());
		FeatureMatcher::registerMatcher(ptr);
	}
}

FeatureType cvSURFFeatureMatcher::featureType()
{
	return (FeatureType)&typeid(cvSURFFeature);
}

cvGFTTFeature::cvGFTTFeature(cv::KeyPoint& keypoint, cv::Mat& descriptor)
	: OpenCVFeature(keypoint, descriptor)
{ 
	if (!FeatureMatcher::hasMatcher<cvGFTTFeature>()) {
		boost::shared_ptr<FeatureMatcher> ptr(new cvGFTTFeatureMatcher());
		FeatureMatcher::registerMatcher(ptr);
	}
}

FeatureType cvGFTTFeatureMatcher::featureType()
{
	return &typeid(cvGFTTFeature);
}


} // namespace Vision

} // namespace Ubitrack
