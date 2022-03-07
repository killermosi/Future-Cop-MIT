#include "MSICResource.h"

#include "../../Utilities/DataHandler.h"

namespace {
    const size_t SIZE_OF_HEADERS = 0x0014;
    const size_t SIZE_OF_DATA    = 0x5FC0;
    const size_t SIZE_OF_READ    = SIZE_OF_HEADERS + SIZE_OF_DATA;
}

const std::string Data::Mission::MSICResource::FILE_EXTENSION = "msic";
const uint32_t Data::Mission::MSICResource::IDENTIFIER_TAG = 0x4D534943; // which is { 0x4D, 0x53, 0x49, 0x43 } or { 'M', 'S', 'I', 'C' } or "MSIC"

Data::Mission::MSICResource::MSICResource() {

}

Data::Mission::MSICResource::MSICResource( const MSICResource &obj ) : Resource( obj ), sound( obj.sound ) {

}

std::string Data::Mission::MSICResource::getFileExtension() const {
    return FILE_EXTENSION;
}

uint32_t Data::Mission::MSICResource::getResourceTagID() const {
    return IDENTIFIER_TAG;
}

bool Data::Mission::MSICResource::parse( const Utilities::Buffer &header, const Utilities::Buffer &reader_data, const ParseSettings &settings ) {
    auto raw_data = reader_data.getReader().getBytes();

    bool   file_is_not_valid = false;
    size_t advance = SIZE_OF_DATA;

    auto   data = raw_data.data();

    sound.setChannelNumber( 1 );
    sound.setSampleRate( 28000 );
    sound.setBitsPerSample( 8 );

    while( advance == SIZE_OF_DATA )
    {
        if( (SIZE_OF_READ + (data - raw_data.data())) > raw_data.size() )
            advance = raw_data.size() - (data - raw_data.data());
        else
            advance = SIZE_OF_DATA;

        data += SIZE_OF_HEADERS;

        sound.addAudioStream( data, advance );
        data += advance;
    }

    sound.updateAudioStreamLength();

    return !file_is_not_valid;
}

int Data::Mission::MSICResource::write( const char *const file_path, const std::vector<std::string> & arguments ) const {
    return sound.write( file_path, arguments );
}

Data::Mission::Resource * Data::Mission::MSICResource::duplicate() const {
    return new MSICResource( *this );
}

std::vector<Data::Mission::MSICResource*> Data::Mission::MSICResource::getVector( Data::Mission::IFF &mission_file ) {
    std::vector<Resource*> to_copy = mission_file.getResources( Data::Mission::MSICResource::IDENTIFIER_TAG );

    std::vector<MSICResource*> copy;

    copy.reserve( to_copy.size() );

    for( auto it = to_copy.begin(); it != to_copy.end(); it++ )
        copy.push_back( dynamic_cast<MSICResource*>( (*it) ) );

    return copy;
}

const std::vector<Data::Mission::MSICResource*> Data::Mission::MSICResource::getVector( const Data::Mission::IFF &mission_file ) {
    return Data::Mission::MSICResource::getVector( const_cast< IFF& >( mission_file ) );
}
