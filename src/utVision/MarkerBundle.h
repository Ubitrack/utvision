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




#include <utMath/Vector.h>
#include <utMath/Matrix.h>

#include <utUtil/Logging.h>
#include <utUtil/CalibFile.h>


#include <map>
#include <string>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>


using namespace Ubitrack;
namespace Markers = Ubitrack::Vision::Markers;


#ifndef SCONF_H
#define SCONF_H

struct SConfig
{
	
	//void init();
	//void init(std::string sMFile,std::string sDFile,std::string sRFile);
	std::map< unsigned long long int, Markers::MarkerInfo > markers;
	std::string sMatrixFile;
	std::string sDistortionFile;
	std::string sResultFile;
	SConfig():sResultFile("multiMarker.utql"){}
	
	struct RefPoint
	{
		RefPoint()
			: pos( 0, 0, 0 )
		{}

		Math::Vector< double, 3 > pos;

		struct Meas
		{
			Meas( const std::string& i, const Math::Vector< double, 2 >& m )
				: image( i )
				, pos( m )
			{}

			std::string image;
			Math::Vector< double, 2 > pos;
		};
		std::vector< Meas > measurements;
	};
	
	// Setters
	UTVISION_EXPORT void setResultFile(std::string re);
	UTVISION_EXPORT void setMatrixFile(std::string mat);
	UTVISION_EXPORT void setDistortionFile(std::string dis);
	UTVISION_EXPORT void setMarkersInfo(unsigned long long int code, float size);
	UTVISION_EXPORT void setRefPositions(std::string id, double x, double y, double z);
	UTVISION_EXPORT void setRefPoints(std::string id, RefPoint::Meas measurement);
	
	// Getters
	UTVISION_EXPORT std::string getResultFile();
	UTVISION_EXPORT std::string getMatrixFile();
	UTVISION_EXPORT std::string getDistortionFile();	
	UTVISION_EXPORT std::map< unsigned long long int, Markers::MarkerInfo > getMarkers();
	
	
	
	UTVISION_EXPORT void parseMBConf(std::string conFile);
	//void setMarkersInfo(std::vector< MarkerMap > markersList);
	
	typedef std::map< std::string, RefPoint > RefPointMap;
	RefPointMap refPoints;
	

};
#endif



#ifndef BAMI_H
#define BAMI_H

struct BAMarkerInfo
{
	BAMarkerInfo()
		: bPoseComputed( false )
	{}

	std::size_t index;
	bool bPoseComputed;
	Math::Pose pose;
	double fSize;
	
	std::set< std::size_t > cameras;
};
#endif

#ifndef BACI_H
#define BACI_H
struct BACameraInfo
{
	BACameraInfo()
		: bPoseComputed( false )
	{}
	
	bool bPoseComputed;
	Math::Pose pose;
	std::string name;
	typedef std::map< unsigned long long int, Markers::MarkerInfo > MarkerMeasMap;
	MarkerMeasMap measMarker;
};
#endif

#ifndef BAI_H
#define BAI_H
struct BAInfo
{
	BAInfo( const Math::Matrix< float, 3, 3 >& _intrinsics, const Math::Vector< double, 4 >& _radial )
		: intrinsicMatrix( _intrinsics )
		, radialCoeffs( _radial )
	{
		intrinsics( 0 ) = _intrinsics( 0, 0 );
		intrinsics( 1 ) = _intrinsics( 0, 1 );
		intrinsics( 2 ) = _intrinsics( 0, 2 );
		intrinsics( 3 ) = _intrinsics( 1, 1 );
		intrinsics( 4 ) = _intrinsics( 1, 2 );
	}

	// marker and camera data
	typedef std::map< unsigned long long int, BAMarkerInfo > MarkerMap;
	MarkerMap markers;
	typedef std::vector< BACameraInfo > CameraList;
	CameraList cameras;
	std::map< std::string, std::size_t > imageToCam;
		
	
	// initialization routines for minimization
		
	UTVISION_EXPORT void initMarkers();	
	UTVISION_EXPORT void initRefPoints(bool undistorted = false);
	
	// functions required for levenberg-marquardt
	std::size_t size() const;
	
	std::size_t parameterSize() const
	{ return 6 * ( markers.size() + cameras.size() ); }
	
	template< class VT1, class VT2, class MT1 > 
	void evaluateWithJacobian( VT1& result, const VT2& v, MT1& jac ) const;

	// fill/update from vectors
	template< class VT >
	void genParameterVector( VT& v );
	
	template< class VT >
	void genTargetVector( VT& v );
	
	template< class VT >
	void updateParameters( VT& v );

	UTVISION_EXPORT void bundleAdjustment( bool bUseRefPoints);

	/** write configuration to console */
	UTVISION_EXPORT void printConfiguration();

	/** write residuals to console */
	UTVISION_EXPORT void printResiduals();

	/** write utql file with multi-marker configuration */
	UTVISION_EXPORT void writeUTQL( std::ostream& of );

	/** use the reference points in the minimization? */
	bool m_bUseRefPoints;

	// intrinsic camera parameters
	Math::Matrix< double, 3, 3 > intrinsicMatrix;
	Math::Vector< double, 4 > radialCoeffs;
	Math::Vector< double, 5 > intrinsics;
};

#endif



/*
#ifdef INSTANTIATE_SCONFIG
SConfig g_config;t
#else
extern SConfig g_config;
#endif
*/

UTVISION_EXPORT SConfig &get_config(); 
UTVISION_EXPORT std::ostringstream &getStream();
UTVISION_EXPORT double getStdDev();


void createImageList(std::vector< std::string >& l);