#ifndef MISSION_RESOURCE_MSIC_HEADER
#define MISSION_RESOURCE_MSIC_HEADER

#include "WAVResource.h"

namespace Data {

namespace Mission {

class MSICResource : public Resource {
public:
	static const std::string FILE_EXTENSION;
	static const uint32_t IDENTIFIER_TAG;
private:
    WAVResource sound;
public:
    MSICResource();
    MSICResource( const MSICResource &obj );

    virtual std::string getFileExtension() const;

    virtual uint32_t getResourceTagID() const;

    virtual bool parse( const ParseSettings &settings = Data::Mission::Resource::DEFAULT_PARSE_SETTINGS );

    virtual int write( const char *const file_path, const std::vector<std::string> & arguments ) const;

    virtual Resource * duplicate() const;

    static std::vector<MSICResource*> getVector( IFF &mission_file );
    static const std::vector<MSICResource*> getVector( const IFF &mission_file );
};

}

}

#endif // MISSION_RESOURCE_MSIC_HEADER
