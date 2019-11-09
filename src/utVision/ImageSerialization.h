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
 * Image Serialization
 * @author Ulrich Eck <ulrich.eck@tum.de>
 */

#ifndef UBITRACK_IMAGE_SERIALIZATION_H
#define UBITRACK_IMAGE_SERIALIZATION_H

#include "utSerialization/Serialization.h"
#include "utVision/Image.h"


#include <msgpack.hpp>
#include "zstd.h"

namespace Ubitrack {
namespace Serialization {
namespace MsgpackArchive {


/*
 * Ubitrack::Vision::Image
 */

template<>
struct MsgpackSerializationFormat<Ubitrack::Measurement::ImageMeasurement> {

  template<typename Stream>
  inline static void write(Stream& pac, const Ubitrack::Measurement::ImageMeasurement& t)
  {
      pac.pack_unsigned_long_long(t.time());
      pac.pack_int(t->width());
      pac.pack_int(t->height());
      Ubitrack::Vision::Image::ImageFormatProperties fmt;
      t->getFormatProperties(fmt);
      pac.pack_int((int)fmt.imageFormat);
      pac.pack_int(fmt.depth);
      pac.pack_int(fmt.channels);
      pac.pack_int(fmt.matType);
      pac.pack_int(fmt.bitsPerPixel);
      pac.pack_int(fmt.origin);
      // compress all image buffers with zstd level 2 if not of type MJPEG
      if (fmt.imageFormat == Ubitrack::Vision::Image::PixelFormat ::MJPEG) {
          std::size_t bsize = t->Mat().total()*t->Mat().elemSize();
          pac.pack_bin(bsize);
          pac.pack_bin_body(reinterpret_cast<const char*>(t->Mat().data), bsize);
      } else {
          std::size_t bsize = t->Mat().total()*t->Mat().elemSize();
          // *2, because according to zstd documentation, increasing the size of the output buffer above a
          // bound should speed up the compression.
          int compressionLevel = 2; // fixed for now ...
          int cBuffSize = ZSTD_compressBound(bsize) * 2;
          std::vector<char> compressedBuffer(cBuffSize);
          int cSize = ZSTD_compress(compressedBuffer.data(), cBuffSize, reinterpret_cast<const char*>(t->Mat().data), bsize, compressionLevel);
          pac.pack_bin(cSize);
          pac.pack_bin_body(&compressedBuffer[0], bsize);
      }
  }

  template<typename Stream>
  inline static void read(Stream& pac, Ubitrack::Measurement::ImageMeasurement& t)
  {
      Ubitrack::Measurement::Timestamp ts;
      int width, height;
      Ubitrack::Vision::Image::ImageFormatProperties fmt;
      msgpack::object_handle oh;
      // @todo throw some exceptions if not complete ?
      bool invalid = false;
      if (pac.next(oh)) {
		  msgpack::adaptor::convert<unsigned long long>()(oh.get(), ts);
      } else { invalid = true; }
      if (pac.next(oh)) {
		  msgpack::adaptor::convert<int>()(oh.get(), width);
      } else { invalid = true; }
      if (pac.next(oh)) {
		  msgpack::adaptor::convert<int>()(oh.get(), height);
      } else { invalid = true; }
      if (pac.next(oh)) {
		  int imageFormat;
		  msgpack::adaptor::convert<int>()(oh.get(), imageFormat);
          fmt.imageFormat = static_cast<typename Ubitrack::Vision::Image::PixelFormat>(imageFormat);
      } else { invalid = true; }
      if (pac.next(oh)) {
		  msgpack::adaptor::convert<int>()(oh.get(), fmt.depth);
      } else { invalid = true; }
      if (pac.next(oh)) {
		  msgpack::adaptor::convert<int>()(oh.get(), fmt.channels);
      } else { invalid = true; }
      if (pac.next(oh)) {
		  msgpack::adaptor::convert<int>()(oh.get(), fmt.matType);
      } else { invalid = true; }
      if (pac.next(oh)) {
		  msgpack::adaptor::convert<int>()(oh.get(), fmt.bitsPerPixel);
      } else { invalid = true; }
      if (pac.next(oh)) {
		  msgpack::adaptor::convert<int>()(oh.get(), fmt.origin);
      } else { invalid = true; }
      // all images are compressed with zstd level=2 if not mjpeg
      boost::shared_ptr<Ubitrack::Vision::Image> img;
      if (pac.next(oh)) {
          msgpack::object obj = oh.get();
          if (fmt.imageFormat == Ubitrack::Vision::Image::PixelFormat::MJPEG) {
              cv::Mat rawData(1,obj.via.bin.size,CV_8SC1,const_cast<void*>(static_cast<const void*>(obj.via.bin.ptr)));
              cv::Mat decompressedImage = cv::imdecode(rawData,-CV_LOAD_IMAGE_COLOR);
              fmt.imageFormat = Ubitrack::Vision::Image::PixelFormat ::BGR;
              width = decompressedImage.cols;
              height = decompressedImage.rows;
              img.reset(new Ubitrack::Vision::Image(decompressedImage, fmt));
          } else {
              std::size_t bsize = ZSTD_getDecompressedSize(obj.via.bin.ptr, obj.via.bin.size);
              img.reset(new Ubitrack::Vision::Image(width, height, fmt));
              std::size_t unpacked_bytes = ZSTD_decompress(img->Mat().data, img->Mat().total()*img->Mat().elemSize(), obj.via.bin.ptr, obj.via.bin.size);
              if (unpacked_bytes != img->Mat().total()*img->Mat().elemSize()) {
                  invalid = true;
              }
          }
      } else { invalid = true; }
      if (!invalid) {
          t = Ubitrack::Measurement::ImageMeasurement(ts, img);
      } else {
          t.reset();
      }
  }


  inline static uint32_t maxSerializedLength(const Ubitrack::Measurement::ImageMeasurement& t)
  {
      return 0;
  }
};

} // MsgpackArchive
} // Serialization
} // Ubitrack



#endif //UBITRACK_IMAGE_SERIALIZATION_H
