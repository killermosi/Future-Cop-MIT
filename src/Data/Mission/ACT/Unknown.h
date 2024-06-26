#ifndef MISSION_ACT_TYPE_UNKNOWN_HEADER
#define MISSION_ACT_TYPE_UNKNOWN_HEADER

#include "../ACTResource.h"

namespace Data {

namespace Mission {

namespace ACT {

class Unknown : public ACTResource {
public:
    static uint_fast16_t TYPE_ID;
private:
    // In the tACT chunk after the matching number this byte describes what type of structure that this actor is.
    // I made this conclusion because I see a pattern of this byte directly affects the size of the tACT chunk.
    uint_fast8_t act_type;

    std::vector<uint8_t> act_buffer;
protected:
    virtual Json::Value makeJson() const;

    virtual bool readACTType( uint_fast8_t act_type, Utilities::Buffer::Reader &data_reader, Utilities::Buffer::Endian endian );
public:
    Unknown();
    Unknown( const ACTResource& obj );
    Unknown( const Unknown& obj );

    virtual uint_fast8_t getTypeID() const;
    virtual std::string getTypeIDName() const;

    virtual size_t getSize() const;

    virtual bool checkRSL() const;

    virtual Resource* duplicate() const;
    
    virtual ACTResource* duplicate( const ACTResource &original ) const;

    static std::vector<std::string> getStructure( uint_fast8_t type_id, const std::vector<const Data::Mission::IFF*> &little_endian, const std::vector<const Data::Mission::IFF*> &big_endian );
};

}

}

}

#endif
