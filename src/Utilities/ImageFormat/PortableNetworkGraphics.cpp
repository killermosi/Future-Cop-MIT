#include "PortableNetworkGraphics.h"

#include "Config.h"

#include <cstring>

std::filesystem::path Utilities::ImageFormat::PortableNetworkGraphics::getExtension() const {
    return FILE_EXTENSION;
}

bool Utilities::ImageFormat::PortableNetworkGraphics::isFormat( const Buffer& buffer ) const {
    auto reader = buffer.getReader();
    uint32_t FIRST = 0x89504E47; // \211 P N G
    uint32_t LAST  = 0x0D0A1A0A; // \r \n \032 \n
    if( reader.totalSize() < 0x8 )
        return false;
    else
        return (reader.readU32( Buffer::Endian::BIG ) == FIRST) &
               (reader.readU32( Buffer::Endian::BIG ) ==  LAST);
}


const std::filesystem::path Utilities::ImageFormat::PortableNetworkGraphics::FILE_EXTENSION = "png";

Utilities::ImageFormat::PortableNetworkGraphics::PortableNetworkGraphics() {}

Utilities::ImageFormat::PortableNetworkGraphics::~PortableNetworkGraphics() {}

Utilities::ImageFormat::ImageFormat* Utilities::ImageFormat::PortableNetworkGraphics::duplicate() const {
    return new PortableNetworkGraphics();
}

#ifndef BUILD_WITH_LIBPNG

size_t Utilities::ImageFormat::PortableNetworkGraphics::getSpace( const ImageBase2D<Grid2DPlacementNormal>& image_data ) const {
    return 0; // The format is invalid for writing.
}

bool Utilities::ImageFormat::PortableNetworkGraphics::supports( const PixelFormatColor& pixel_format ) const {
    return 0;
}

bool Utilities::ImageFormat::PortableNetworkGraphics::canRead() const {
    return false;
}

bool Utilities::ImageFormat::PortableNetworkGraphics::canWrite() const {
    return false;
}

int Utilities::ImageFormat::PortableNetworkGraphics::write( const ImageBase2D<Grid2DPlacementNormal>& image_data, Buffer& buffer ) {
    return -1;
}

#else

#include <libpng16/png.h>
#include <zlib.h>
#include "../ImagePalette2D.h"

namespace {

png_image setupImage( const Utilities::ImageBase2D<Utilities::Grid2DPlacementNormal>& image_data, bool &is_valid ) {
    // Thanks NonStatic
    png_image image_write;
    memset( &image_write, 0, sizeof(image_write) );
    image_write.version = PNG_IMAGE_VERSION;
    image_write.format = PNG_FORMAT_RGB;
    image_write.height = image_data.getHeight();
    image_write.width  = image_data.getWidth();

    is_valid = true;

    if( dynamic_cast<const Utilities::PixelFormatColor_W8*>( image_data.getPixelFormat() ) != nullptr )
        image_write.format = PNG_FORMAT_GRAY;
    else
    if( dynamic_cast<const Utilities::PixelFormatColor_W8A8*>( image_data.getPixelFormat() ) != nullptr )
        image_write.format = PNG_FORMAT_GA;
    else
    if( dynamic_cast<const Utilities::PixelFormatColor_R8G8B8*>( image_data.getPixelFormat() ) != nullptr )
        image_write.format = PNG_FORMAT_RGB;
    else
    if( dynamic_cast<const Utilities::PixelFormatColor_R8G8B8A8*>( image_data.getPixelFormat() ) != nullptr )
        image_write.format = PNG_FORMAT_RGBA;
    else
        is_valid = false;
        
    // image_write.format = PNG_FORMAT_LINEAR_Y;
    // image_write.format = PNG_FORMAT_LINEAR_Y_ALPHA;
    // image_write.format = PNG_FORMAT_LINEAR_RGB;
    // image_write.format = PNG_FORMAT_LINEAR_RGB_ALPHA;
    
    return image_write;
}

bool internalMemory( png_image& info, void *buffer_r, png_alloc_size_t &length, const Utilities::ImageBase2D<Utilities::Grid2DPlacementNormal>& image_data ) {
    const Utilities::ImageBase2D<Utilities::Grid2DPlacementNormal>* image_data_r = &image_data;
    
    bool is_valid = png_image_write_to_memory( &info, buffer_r, &length, 0, (void*)image_data_r->getDirectGridData(), 0, nullptr );
    
    return is_valid;
}

}

bool Utilities::ImageFormat::PortableNetworkGraphics::canRead() const {
    return false;
}

bool Utilities::ImageFormat::PortableNetworkGraphics::canWrite() const {
    return true;
}

bool Utilities::ImageFormat::PortableNetworkGraphics::supports( const PixelFormatColor& pixel_format ) const {
    if( dynamic_cast<const Utilities::PixelFormatColor_W8*>( &pixel_format ) != nullptr )
        return true;
    else
    if( dynamic_cast<const Utilities::PixelFormatColor_W8A8*>( &pixel_format ) != nullptr )
        return true;
    else
    if( dynamic_cast<const Utilities::PixelFormatColor_R8G8B8*>( &pixel_format ) != nullptr )
        return true;
    else
    if( dynamic_cast<const Utilities::PixelFormatColor_R5G5B5A1*>( &pixel_format ) != nullptr )
        return true;
    else
    if( dynamic_cast<const Utilities::PixelFormatColor_R8G8B8A8*>( &pixel_format ) != nullptr )
        return true;
    else
        return false;
}

size_t Utilities::ImageFormat::PortableNetworkGraphics::getSpace( const ImageBase2D<Grid2DPlacementNormal>& image_data ) const {
    bool is_valid;
    auto image_write = setupImage( image_data, is_valid );
    
    if( !is_valid )
        return 0; // The format is invalid for writing.
    else {
        png_alloc_size_t length = 0;
        
        if( !internalMemory( image_write, nullptr, length, image_data ) )
            return 0; // Failed to obtain data needed to write the PNG.
        else
            return static_cast<size_t>( length );
    }
}

int Utilities::ImageFormat::PortableNetworkGraphics::write( const ImageBase2D<Grid2DPlacementNormal>& image_data, Buffer& buffer ) {
    bool is_valid;
    png_image image_write;
    Utilities::Image2D image_convert( 0, 0, *image_data.getPixelFormat() );
    Utilities::Image2D image_convert_rbga( 0, 0, Utilities::PixelFormatColor_R8G8B8A8::linear ); // TODO Check the color spaces. PNG's use sRGB space.
    auto selected_image_r = &image_data;
    
    if( dynamic_cast<const Utilities::ImagePalette2D*>( &image_data ) != nullptr ) {
        image_convert.setDimensions( image_data.getWidth(), image_data.getHeight() );
        
        image_convert.inscribeSubImage( 0, 0, image_data );
        
        selected_image_r = &image_convert;
    }
    
    if( dynamic_cast<const Utilities::PixelFormatColor_R5G5B5A1*>( selected_image_r->getPixelFormat() ) != nullptr ) {
        image_convert_rbga.setDimensions( selected_image_r->getWidth(), selected_image_r->getHeight() );
        
        
        image_convert_rbga.inscribeSubImage( 0, 0, *selected_image_r );
        
        selected_image_r = &image_convert_rbga;
    }
    
    image_write = setupImage( *selected_image_r, is_valid );
    
    if( !is_valid )
        return -1; // The format is invalid for writing.
    else {
        png_alloc_size_t length = 0;
        
        if( !internalMemory( image_write, nullptr, length, *selected_image_r ) )
            return -2; // Failed to obtain data needed to write the PNG.
        else if( length == 0 )
            return -3; // There is no length to allocate for the buffer.
        else {
            // Now, attempt to allocate the buffer.
            if( !buffer.allocate( length ) )
                return -4;
            else {
                // The buffer is finally allocated. Now, attempt to write the PNG to memory
                if( internalMemory( image_write, (void*)buffer.dangerousPointer(), length, *selected_image_r ) )
                    return 1; // The buffer is successfully read.
                else
                    return -5; // The image has failed to be written.
            }
        }
    }
}
#endif
