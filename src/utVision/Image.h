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
 * A class for handling OpenCV images in a "Resource Acquisition Is Initialization" (RAII) fashion
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#ifndef __UBITRACK_VISION_IMAGE_H_INCLUDED__
#define __UBITRACK_VISION_IMAGE_H_INCLUDED__

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/utility.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/binary_object.hpp>
#include <opencv/cxcore.h>
#include <utVision.h>
#include <utMeasurement/Measurement.h>

#include <opencv2/opencv.hpp>
#include <log4cpp/Category.hh>

//#define CONVERSION_LOGGING

namespace Ubitrack { namespace Vision {

	static log4cpp::Category& imageLogger( log4cpp::Category::getInstance( "Ubitrack.Vision.Image" ) );


/**
 * @ingroup vision
 * A wrapper object for IplImages in a "Resource Acquisition Is Initialization" (RAII) fashion.
 *
 * We don't want to change the OpenCV feeling more than necessary, but we need images that
 * destroy themselves when they run out of scope!
 */
class UTVISION_EXPORT Image
	: //public IplImage
	 private boost::noncopyable
    //, public boost::enable_shared_from_this< Image >
{
public:
	
	/**
	 * Some abbreviations for shared pointers of Image
	 */
	typedef boost::shared_ptr< Image > Ptr;
	typedef boost::shared_ptr< const Image > ConstPtr;
    

    /**
     * Defining image dimensions
     */
    struct Dimension {
        Dimension( void ) {
            width = 0;
            height = 0;
        };
        Dimension( int width, int height ) {
            this->width = width;
            this->height = height;
        };
        int width;
        int height;
        bool operator== (Dimension& d) {
            return (d.width == width) && (d.height == height);
        };
        bool operator== (const Dimension& d) {
            return (d.width == width) && (d.height == height);
        };

    };


	enum PixelFormat {
	  UNKNOWN_PIXELFORMAT = 0,
	  LUMINANCE,
	  RGB,
	  BGR,
	  RGBA,
	  BGRA,
	  YUV422,
	  YUV411,
	  RAW,
	  DEPTH
	};

    /* Enum to relect memory location of image buffer */
    enum ImageUploadState {
      OnCPU,
      OnGPU,
      OnCPUGPU
    };


    /* Enum to mask ImageProperties when copying */
    enum ImageProperties {
      IMAGE_FORMAT = 1,
      IMAGE_DEPTH = 2,
      IMAGE_CHANNELS = 4,
      IMAGE_BITSPERPIXEL = 8,
      IMAGE_ORIGIN = 16
    };


    /*
     * Image Format Descriptor
     */
    struct ImageFormatProperties {
        ImageFormatProperties() {
            imageFormat = Image::UNKNOWN_PIXELFORMAT;
            depth = CV_8U;
            channels = 1;
            matType = CV_8UC1;
            bitsPerPixel = 8;
            origin = 0;
            align = 4;
        }

        /*
         * Image Format Enum
         */
        PixelFormat imageFormat;

        /*
         * CV Matrix Element Type (e.g. CV_8U)
         */
        int depth;

        /*
         * Number of channels in the image
         */
        int channels;

        /*
         * CV Matrix Type (e.g. CV_8UC1)
         */
        int matType;

        /*
         * Number of bits per Pixel
         */
        int bitsPerPixel;

        /*
         * Origin of image (0 if pixels start in top-left corner, 1 if in bottom-left)
         */
        int origin;

        /*
         * alignment of rows in bytes
         */
        int align;
    };



    /**
	 * Creates a new image and allocates memory for it.
	 *
	 * @param nWidth width of image in pixels
	 * @param nHeight height of image in pixels
	 * @param nFormat Image Format Properties
	 */
    Image( int nWidth, int nHeight, ImageFormatProperties& nFormat, ImageUploadState nState = OnCPU);

    /**
	 * Creates a new image and allocates memory for it. (DEPRECATED)
	 *
	 * @param nWidth width of image in pixels
	 * @param nHeight height of image in pixels
	 * @param nChannels number of channels (1=grey, 3=rgb)
	 * @param nDepth information about pixel representation (see OpenCV docs)
	 * @param nOrigin 0 if pixels start in top-left corner, 1 if in bottom-left
	 */
	Image( int nWidth = 0, int nHeight = 0, int nChannels = 1, int nDepth = CV_8U,
			int nOrigin = 0, ImageUploadState nState = OnCPU);


    /**
     * Creates a reference to an existing image array.
     * The resulting \c Image object does not own the data.
     *
     * @param nWidth width of image in pixels
     * @param nHeight height of image in pixels
	 * @param nFormat Image Format Properties
     * @param pImageData pointer to raw image data
     */
    Image( int nWidth, int nHeight, ImageFormatProperties& nFormat, void* pImageData );

	/**
	 * Creates a reference to an existing image array. (DEPRECATED)
	 * The resulting \c Image object does not own the data.
	 *
	 * @param nWidth width of image in pixels
	 * @param nHeight height of image in pixels
	 * @param nChannels number of channels (1=grey, 3=rgb)
	 * @param pImageData pointer to raw image data
	 * @param nDepth information about pixel representation (see OpenCV docs)
	 * @param nOrigin 0 if pixels start in top-left corner, 1 if in bottom-left
	 * @param nAlign alignment of rows in bytes
	 */
	Image( int nWidth, int nHeight, int nChannels, void* pImageData,
		int nDepth = CV_8U, int nOrigin = 0, int nAlign = 4 );

	/**
	 * Create from pointer to IplImage. (DEPRECATED)
	 *
	 * @param pIplImage pointer to existing IplImage
	 * @param bDestroy if true, the IplImage is destroyed (using cvReleaseImageHeader) and
	 * ownership of the data is taken. Otherwise the data is not owned.
	 */
	explicit Image( IplImage* pIplImage, bool bDestroy = true );

	/**
	 * Create from Mat object
	 */
	explicit Image( cv::Mat & img );

    /**
     * Create from Mat object with Properties
     */
    explicit Image( cv::Mat & img, ImageFormatProperties& fmt );

	/**
	* Create from UMat object
	*/
	explicit Image(cv::UMat & img);

    /**
    * Create from UMat object with Properties
    */
    explicit Image(cv::UMat & img, ImageFormatProperties& fmt);

    /** releases the image data if the data block is owned by this object. */
	~Image();

	cv::Mat& Mat()
	{
		checkOnCPU();
		return m_cpuImage;
	}


	cv::UMat& uMat()
	{
		checkOnGPU();
		return m_gpuImage;
	}


	/**
	 * Convert color space.
	 * Wraps \c cvCvtColor, but keeps the origin flag intact.
	 *
	 * @param nCode conversion Code (see OpenCV docs of \c cvCvtColor, e.g. CV_RGB2GRAY)
	 * @param nChannels number of channels (1=grey, 3=rgb)
	 * @param nDepth information about pixel representation (see OpenCV docs)
	 */
	Ptr CvtColor( int nCode, int nChannels, int nDepth = CV_8U ) const;

    /** Allocates empty memory of the size of the image */
	Ptr AllocateNew() const;

	/** Creates a copy of this image */
	Ptr Clone() const;
	
	/** creates an image which is half the size */
	Ptr PyrDown();// TODO: const;
	
	/** creates an image with the given size */
	Ptr Scale( int width, int height ); // TODO: const;

    /** creates an image with the given scale factor 0.0 < f <= 1.0 */
	Ptr Scale( double scale ); // TODO: const;

	/** Creates an image with adapted contrast and brightness */
//	Ptr ContrastBrightness( int contrast, int brightness ); // TODO:  const;


    /** Has the image only one channel?  */
    bool isGrayscale( void ) const;

    /** Convert to grayscale */
    Ptr getGrayscale( void ) const;

    /** Returns the dimension of the image */
    Dimension dimension( void ) const {
        return Dimension(width(), height());
    };

	// getters
	int channels( void ) const {
		return m_channels;
	}

	int width( void ) const {
		return m_width;
	}

	int height( void ) const {
		return m_height;
	}

	int depth( void ) const {
		return m_depth;
	}

	int bitsPerPixel( void ) const {
		return m_bitsPerPixel;
	}

	int origin( void ) const {
		return m_origin;
	}

	int cvMatType(void) const;

	PixelFormat pixelFormat( void ) const {
		return m_format;
	}

	// setters
	void set_channels( int v ) {
		m_channels = v;
	}

	void set_width( int v ) {
		m_width = v;
	}

	void set_height( int v ) {
		m_height = v;
	}

    void set_depth( int v ) {
        m_depth = v;
    }

    void set_bitsPerPixel( int v ) {
		m_bitsPerPixel = v;
	}

	void set_origin( int v ) {
		m_origin = v;
	}

	void set_pixelFormat( PixelFormat v ) {
		m_format = v;
	}


    /**
     * Guess Format of an Image based on the given properties
     * @param result
     * @param channels
     * @param depth = -1
     * @param matType = -1
     */
    static void guessFormat(ImageFormatProperties& result, int channels, int depth=-1, int matType=-1);
    static void guessFormat(ImageFormatProperties& result, cv::Mat m);
    static void guessFormat(ImageFormatProperties& result, cv::UMat m);
    static void guessFormat(ImageFormatProperties& result, IplImage* m);

    /*
     * Get the ImageFormatProperties from the current image
     */
    void getFormatProperties(ImageFormatProperties& result) const;

    /*
     * Set the ImageFormatProperties for the current image using a Mask (ImageProperties)
     */
    void setFormatProperties(ImageFormatProperties& fmt, unsigned char mask=255);

    /**
     * @brief Copy image format related properties from first argument using a mask (ImageProperties)
     * @param img
     * @param mask = 255 (copy all by default)
     */
    void copyImageFormatFrom(const Image& img, unsigned char mask=255);


    /**
     * @brief Saves an image as compressed JPEG
     * @param filename Filename of the image
     * @param compressionFactor Sets the quality of the compression from 0...100. Default is 95.
     * @warning The filename must end with 'jpg' to get the desired result.
     * 
     */
    void saveAsJpeg( const std::string filename, int compressionFactor = 95 ) const;

    /**
     * @brief Compresses an image in memory using JPEG compression
     * @param buffer A reference to a memory buffer holding the result. The buffer will be resized to fit the result.
     * @param compressionFactor Sets the quality of the compression from 0...100. Default is 95.
     */
    void encodeAsJpeg( std::vector< uchar >& buffer, int compressionFactor = 95 ) const;



    /**
     * @brief get current ImageState
     */
	ImageUploadState getImageState() const {
		return m_uploadState;
	}

	/**
	 * @brief Check if image is in GPU memory
	 */
    bool isOnGPU() const {
        return m_uploadState == OnCPUGPU || m_uploadState == OnGPU;
    }

    /**
     * @brief Check if image is in CPU memory
     */
    bool isOnCPU() const {
        return m_uploadState == OnCPUGPU || m_uploadState == OnCPU;
    }

private:

	// ensure that image is on CPU (copy if needed)
	void checkOnCPU();

    // ensure that image is on GPU (copy if needed)
	void checkOnGPU();

    // the current imageState
	ImageUploadState m_uploadState;

    // does this object own the data imageData points to?
	bool m_bOwned;

    // the image width in pixels
    int m_width;

    // the image height in pixels
    int m_height;

    // the number of channels per pixel
    int m_channels;

    // OpenCV Matrix Element Type (CV_8U/CV_16U/...)
    int m_depth;

    // the number of bits per pixel (for one channel)
    int m_bitsPerPixel;

    // the origin of the image
    int m_origin;

    // the image format, e.g. RGB, ..
    PixelFormat m_format;


    // the GPU buffer
	cv::UMat m_gpuImage;

    // the CPU buffer
	cv::Mat  m_cpuImage;

    // maybe we need a mutex to guard allocation of buffers ?

	friend class ::boost::serialization::access;
	
	/** boost serialization helper from https://cheind.wordpress.com/2011/12/06/serialization-of-cvmat-objects-using-boost/ */
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const
	{	
		//TODO OnCPU should be checked, does not work right now
		//checkOnCPU();
		//LOG4CPP_INFO(imageLogger, "save w:" << m_width << " h: " << m_height << " depth: " << m_bitsPerPixel << " channels: " << m_channels << " total:" << m_cpuImage.total() << " elemSize:" << m_cpuImage.elemSize())

        ar &  (int)m_format;
		if (isOnCPU()) {
			ar & boost::serialization::make_binary_object(m_cpuImage.data, m_cpuImage.total() * m_cpuImage.elemSize());
		} else if (isOnGPU()) {
			cv::Mat tmp = m_gpuImage.getMat(0);
			ar & boost::serialization::make_binary_object(tmp.data, tmp.total() * tmp.elemSize());
		}
	}

	template<class Archive>
	void load(Archive & ar, const unsigned int version)
	{
		m_cpuImage = cv::Mat(m_height, m_width, cv::Mat::MAGIC_VAL + CV_MAKE_TYPE(m_depth, m_channels));
		//LOG4CPP_INFO(imageLogger, "save w:" << m_width << " h: " << m_height << " depth: " << m_bitsPerPixel << " channels: " << m_channels << " total:" << m_cpuImage.total() << " elemSize:" << m_cpuImage.elemSize())
        int fmt;
        ar & fmt;
        m_format = (PixelFormat)fmt;

		boost::serialization::binary_object data(m_cpuImage.data,  m_cpuImage.total() * m_cpuImage.elemSize());
		ar & data;		
        m_uploadState = OnCPU;
	}

	template< class Archive >
	void serialize(Archive& ar, const unsigned int file_version)
	{

		ar &  m_width;          
		ar &  m_height;
        ar &  m_depth;
        ar &  m_bitsPerPixel;
		ar &  m_channels;
		ar &  m_origin;

		boost::serialization::split_member(ar, *this, file_version);
	}
};

typedef Image::Ptr ImagePtr;
typedef Image::ConstPtr ConstImagePtr;
typedef const Image ConstImage;

} // namespace Vision



namespace Measurement {
/** define a shortcut for image measurements */
typedef Measurement< Ubitrack::Vision::Image > ImageMeasurement;
typedef Measurement< Ubitrack::Vision::ConstImage > ConstImageMeasurement;
} // namespace Measurement
} // namespace Ubitrack




#endif

