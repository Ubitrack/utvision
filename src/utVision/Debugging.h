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

#ifndef __UBITRACK_DEBUGGING2_H_INCLUDED__
#define __UBITRACK_DEBUGGING2_H_INCLUDED__

// Ubitrack
#include <utVision.h>
#include <utMath/Matrix.h>
#include <utMath/Vector.h>

// OpenCV
#include <opencv2/core/core.hpp> // cv::Mat
#include <opencv2/core/types_c.h> // CvScalar


namespace Ubitrack { namespace Vision {

UTVISION_EXPORT void drawPoseCube( cv::Mat& img, const Math::Pose& pose, const Math::Matrix< float, 3, 3 >& K, double scale, CvScalar color, bool paintCoordSystem = false );
UTVISION_EXPORT Math::Vector< double, 3 > projectPoint ( const Math::Vector < double, 3 > &pt, const Math::Matrix< double, 3, 3 > &projection, int imageHeight);
UTVISION_EXPORT void drawPose ( cv::Mat& dbgImage, const Math::Pose& pose, const Math::Matrix< double, 3, 3 > &projection, double error );
UTVISION_EXPORT void drawPosition ( cv::Mat& dbgImage, const Math::Vector< double, 3 > &position, const Math::Matrix< double, 3, 3 > &projection, double error );

} } // namespace Ubitrack::Vision

#endif // __UBITRACK_DEBUGGING2_H_INCLUDED__
