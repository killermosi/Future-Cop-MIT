#include "RPNSResource.h"
#include <limits>
#include <cassert>

#include <iostream>

const std::string Data::Mission::RPNSResource::FILE_EXTENSION = "rpns";
// which is { 0x52, 0x50, 0x4E, 0x53 } or { 'R', 'P', 'N', 'S' } or "RPNS"
const uint32_t Data::Mission::RPNSResource::IDENTIFIER_TAG = 0x52504E53;

Data::Mission::RPNSResource::RPNSResource() {
}

Data::Mission::RPNSResource::RPNSResource( const RPNSResource &obj ) {
}

std::string Data::Mission::RPNSResource::getFileExtension() const {
    return Data::Mission::RPNSResource::FILE_EXTENSION;
}

uint32_t Data::Mission::RPNSResource::getResourceTagID() const {
    return Data::Mission::RPNSResource::IDENTIFIER_TAG;
}

bool Data::Mission::RPNSResource::parse( const ParseSettings &settings ) {
    const size_t TAG_HEADER_SIZE = 2 * sizeof(uint32_t);
    Bitfield bitfield;
    uint8_t byte;
    
    if( this->data_p != nullptr )
    {
        auto reader = this->data_p->getReader();
        
        bitfields.reserve( reader.totalSize() );
        
        while( !reader.ended() ) {
            byte = reader.readU8();
            bitfield.a = ((1 << 2) - 1) & (byte >> 6);
            bitfield.b = ((1 << 6) - 1) & (byte >> 0);
            bitfields.push_back( bitfield );
        }
        
        return true;
    }
    
    return false;
}

Data::Mission::Resource * Data::Mission::RPNSResource::duplicate() const {
    return new RPNSResource( *this );
}

int Data::Mission::RPNSResource::write( const std::string& file_path, const std::vector<std::string> & arguments ) const {
    return 0;
}

bool Data::Mission::IFFOptions::RPNSOption::readParams( std::map<std::string, std::vector<std::string>> &arguments, std::ostream *output_r ) {
    return IFFOptions::ResourceOption::readParams( arguments, output_r );
}
