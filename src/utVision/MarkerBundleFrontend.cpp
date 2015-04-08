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

#include "MarkerBundle.h"

#include <utUtil/Logging.h>
#include <utVision/Undistortion.h>

#include <fstream>

// highgui includes windows.h with the wrong parameters
#ifdef _WIN32
#include <utUtil/CleanWindows.h>
#endif

#include <opencv/highgui.h>


namespace Markers = Ubitrack::Vision::Markers;

int main( int, char** )
{
	try
	{		
		
		Util::initLogging();

		// parse configuration file
		get_config().parseMBConf( "markerbundle.conf" );
		
		
		
		// load intrinsics
		Vision::Undistortion undistorter( get_config().sMatrixFile, get_config().sDistortionFile );

		Math::Matrix< float, 3, 3 > intrinsics;
		Math::Util::matrix_cast_assign( intrinsics, undistorter.getMatrix() );
		
		// find image files in directories
		std::vector< std::string > imageNames;
		
		//The image list should be provided as a parameter
		createImageList( imageNames );
		
		// open each file and search for markers
		BAInfo baInfo( intrinsics, undistorter.getRadialCoeffs() );
		for ( std::vector< std::string >::iterator itImage = imageNames.begin(); itImage != imageNames.end(); itImage++ )
		{
			const std::size_t camId( baInfo.cameras.size() );
			baInfo.cameras.push_back( BACameraInfo() );
			baInfo.cameras.back().name = *itImage;
			baInfo.imageToCam[ *itImage ] = camId;
			
			// load image
			boost::shared_ptr< Vision::Image > pImage( new Vision::Image( cvLoadImage( itImage->c_str(), CV_LOAD_IMAGE_GRAYSCALE ) ) );

			// undistort
			pImage = undistorter.undistort( pImage );
			
			// find markers
			std::map< unsigned long long int, Markers::MarkerInfo > markerMap( get_config().markers );
			
			Markers::detectMarkers( *pImage, markerMap, intrinsics, NULL, false, 8, 12 );
			
			// erase not-seen markers from map
			for( std::map< unsigned long long int, Markers::MarkerInfo >::iterator itMarker = markerMap.begin(); itMarker != markerMap.end();  )
				if ( itMarker->second.found != Vision::Markers::MarkerInfo::ENotFound )
				{
					baInfo.markers[ itMarker->first ].fSize = itMarker->second.fSize;
					baInfo.markers[ itMarker->first ].cameras.insert( camId );
					std::cout << "Found marker " << std::hex << itMarker->first << " in " << *itImage << " (camera " << std::dec << camId << ")" << std::endl;
					itMarker++;
				}
				else
					markerMap.erase( itMarker++ );
			
			// copy remaining marker infos
			baInfo.cameras[ camId ].measMarker.swap( markerMap );
		}
		
		// initialize poses
		baInfo.initMarkers();
		
		// do bundle adjustment
		baInfo.bundleAdjustment( false );
		baInfo.printConfiguration();
		baInfo.printResiduals();

		if ( !get_config().refPoints.empty() )
		{
			// add information from reference points
			baInfo.initRefPoints();
			baInfo.bundleAdjustment( true );
			baInfo.printConfiguration();
			baInfo.printResiduals();
		}

		std::ofstream outFile( get_config().sResultFile.c_str() );
		//baInfo.writeUTQL( outFile );
	}
	catch ( const std::string& s )
	{ std::cout << "Error: " << s << std::endl; }
	catch ( const std::runtime_error& e )
	{ std::cout << "Error: " << e.what() << std::endl; }


}
