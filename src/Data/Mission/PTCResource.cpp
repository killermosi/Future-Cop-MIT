#include "PTCResource.h"

#include "../../Utilities/DataHandler.h"
#include <string>
#include <algorithm>
#include <iostream>

namespace {
    const uint32_t GRDB_TAG = 0x47524442; // which is { 0x47, 0x52, 0x44, 0x42 } or { 'G', 'R', 'D', 'B' } or "GRDB"
}

const std::string Data::Mission::PTCResource::FILE_EXTENSION = "ptc";
const uint32_t Data::Mission::PTCResource::IDENTIFIER_TAG = 0x43707463; // which is { 0x43, 0x70, 0x74, 0x63 } or { 'C', 'p', 't', 'c' } or "Cptc"

Data::Mission::PTCResource::PTCResource() {

}

Data::Mission::PTCResource::PTCResource( const PTCResource &obj ) : Resource( obj ), grid( obj.grid ) {
}

std::string Data::Mission::PTCResource::getFileExtension() const {
    return FILE_EXTENSION;
}

uint32_t Data::Mission::PTCResource::getResourceTagID() const {
    return IDENTIFIER_TAG;
}

bool Data::Mission::PTCResource::makeTiles( const std::vector<Data::Mission::TilResource*> &tile_array_r ) {
    if( tile_array_r.size() > 0 )
    {
        debug_map_display.setWidth(  grid.getWidth()  * 0x11 );
        debug_map_display.setHeight( grid.getHeight() * 0x11 );
        debug_map_display.setFormat( Utilities::ImageData::RED_GREEN_BLUE, 1 );

        this->tile_array_r = std::vector<Data::Mission::TilResource*>( tile_array_r );

        for( unsigned int x = 0; x < grid.getWidth(); x++ ) {
            for( unsigned int y = 0; y < grid.getHeight(); y++ ) {
                auto pixel = grid.getPixel( x, y );

                if( pixel != nullptr && *reinterpret_cast<const uint8_t*>(pixel) != 0 )
                    debug_map_display.inscribeSubImage( x * 0x11, y * 0x11, *this->tile_array_r.at( (*reinterpret_cast<const uint8_t*>( pixel ) / sizeof( uint32_t ) - 1) % this->tile_array_r.size() )->getImage() );
            }
        }
        return true;
    }
    else
        return false;
}

Data::Mission::TilResource* Data::Mission::PTCResource::getTile( unsigned int x, unsigned int y ) {
    if( x < grid.getWidth() && y < grid.getHeight() ) {
        auto pixel = grid.getPixel( x, y );
        if( *reinterpret_cast<const uint32_t*>(pixel) != 0 )
            return tile_array_r.at( (*reinterpret_cast<const uint32_t*>( pixel ) / sizeof( uint32_t ) - 1) % this->tile_array_r.size() );
    }

    return nullptr;
}

const Data::Mission::TilResource* Data::Mission::PTCResource::getTile( unsigned int x, unsigned int y ) const {
    return const_cast< Data::Mission::PTCResource* >(this)->getTile( x, y );
}

bool Data::Mission::PTCResource::parse( const Utilities::Buffer &header, const Utilities::Buffer &buffer, const ParseSettings &settings ) {
    auto reader = buffer.getReader();
    bool file_is_not_valid = false;

    while( reader.getPosition( Utilities::Buffer::Reader::BEGINING ) < reader.totalSize() ) {
        auto identifier = reader.readU32( settings.endian );
        auto tag_size   = reader.readU32( settings.endian );

        if( identifier == GRDB_TAG ) {
            reader.setPosition( 0x0C, Utilities::Buffer::Reader::BEGINING );
            
            auto tile_amount = reader.readU32( settings.endian );
            auto width       = reader.readU32( settings.endian );
            auto height      = reader.readU32( settings.endian );
            
            reader.setPosition( 0x2C, Utilities::Buffer::Reader::BEGINING );

            // setup the grid
            grid.setWidth(  width );
            grid.setHeight( height );
            grid.setFormat( Utilities::ImageData::BLACK_WHITE, 4 );

            auto image_data = grid.getRawImageData();
            for( unsigned int a = 0; a < grid.getWidth() * grid.getHeight(); a++ ) {

                *reinterpret_cast<uint32_t*>(image_data) = reader.readU32( settings.endian );

                image_data += grid.getPixelSize();
            }
        }
        else
        {
            reader.setPosition( tag_size - sizeof( uint32_t ) * 2, Utilities::Buffer::Reader::CURRENT );
        }
    }

    return !file_is_not_valid;
}

Data::Mission::Resource * Data::Mission::PTCResource::duplicate() const {
    return new PTCResource( *this );
}

int Data::Mission::PTCResource::write( const char *const file_path, const std::vector<std::string> & arguments ) const {
    std::string file_path_texture = std::string( file_path ) + ".png";
    bool enable_export = true;

    for( auto arg = arguments.begin(); arg != arguments.end(); arg++ ) {
        if( (*arg).compare("--dry") == 0 )
            enable_export = false;
    }

    if( enable_export )
        return debug_map_display.write( file_path_texture.c_str() );
    else
        return 0;
}

std::vector<Data::Mission::PTCResource*> Data::Mission::PTCResource::getVector( Data::Mission::IFF &mission_file ) {
    std::vector<Resource*> to_copy = mission_file.getResources( Data::Mission::PTCResource::IDENTIFIER_TAG );

    std::vector<PTCResource*> copy;

    copy.reserve( to_copy.size() );

    for( auto it = to_copy.begin(); it != to_copy.end(); it++ )
        copy.push_back( dynamic_cast<PTCResource*>( (*it) ) );

    return copy;
}

const std::vector<Data::Mission::PTCResource*> Data::Mission::PTCResource::getVector( const Data::Mission::IFF &mission_file ) {
    return Data::Mission::PTCResource::getVector( const_cast< Data::Mission::IFF& >( mission_file ) );
}
