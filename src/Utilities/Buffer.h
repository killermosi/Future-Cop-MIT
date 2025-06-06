#ifndef UTILITIES_BUFFER_938223_H
#define UTILITIES_BUFFER_938223_H

#include <exception>
#include <istream>
#include <filesystem>
#include <stdint.h>
#include <string>
#include <vector>

namespace Utilities {

class Buffer {
public:
    static const bool IS_CPU_LITTLE_ENDIAN;
    static const bool IS_CPU_BIG_ENDIAN;

    class Reader;
    class Writer;
    
    enum Endian {
        NO_SWAP,
        SWAP,
        LITTLE,
        BIG
    };
    
    enum Direction {
        BEGIN,
        CURRENT,
        END
    };

protected:
    std::vector<uint8_t> data;

public:
    static bool getSwap( Endian endianess );
    
    Buffer();
    Buffer( const Buffer &buffer );
    Buffer( const uint8_t *const buffer_r, size_t byte_amount );
    virtual ~Buffer();

    void reserve(  size_t byte_amount );
    bool allocate( size_t byte_amount );
    bool add( const uint8_t *const buffer, size_t byte_amount );
    bool set( const uint8_t *const buffer, size_t byte_amount );

    Reader getReader( size_t offset = 0, size_t byte_amount = 0 ) const;
    Writer getWriter( size_t offset = 0, size_t byte_amount = 0 );
    
    bool addU8(   uint8_t value );
    bool addI8(    int8_t value );
    bool addU16( uint16_t value, Endian endianess = NO_SWAP );
    bool addI16(  int16_t value, Endian endianess = NO_SWAP );
    bool addU32( uint32_t value, Endian endianess = NO_SWAP );
    bool addI32(  int32_t value, Endian endianess = NO_SWAP );
    bool addU64( uint64_t value, Endian endianess = NO_SWAP );
    bool addI64(  int64_t value, Endian endianess = NO_SWAP );
    
    bool write( const std::filesystem::path& file_path ) const;
    bool read( const std::filesystem::path& file_path );
    
    uint8_t* dangerousPointer();
    const uint8_t *const dangerousPointer() const;
public:
    class BufferOutOfBounds: public std::exception {
    private:
        std::string what_is_wrong;
    public:
        BufferOutOfBounds( const char *const method_name_r, const uint8_t *const data_r, size_t byte_amount, size_t current_index );

        virtual const char* what() const throw();
    };

    class Reader {
    protected:
        const uint8_t *const data_r;
        size_t size;

        size_t current_index;
    public:
        Reader( const Reader& reader );
        Reader( const uint8_t *const buffer_r = nullptr, size_t byte_amount = 0 );
        virtual ~Reader();

        bool empty() const;
        bool ended() const;
        size_t totalSize() const;
        size_t getPosition( Direction way = BEGIN ) const;

        void setPosition( int offset, Direction way = BEGIN );

        std::vector<uint8_t> getBytes( size_t byte_amount = 0, Endian endianess = NO_SWAP );

        Reader getReader( size_t reader_size );

        uint64_t readU64( Endian endianess = NO_SWAP );
        int64_t  readI64( Endian endianess = NO_SWAP );

        uint32_t readU32( Endian endianess = NO_SWAP );
        int32_t  readI32( Endian endianess = NO_SWAP );

        uint16_t readU16( Endian endianess = NO_SWAP );
        int16_t  readI16( Endian endianess = NO_SWAP );

        uint8_t readU8();
        int8_t  readI8();
        
        std::vector<bool> getBitfield( size_t byte_amount = 0 );

        bool addToBuffer( Buffer &buffer, size_t size );
    };
    
    class Writer {
    protected:
        uint8_t *const data_r;
        size_t size;

        size_t current_index;
    public:
        Writer( uint8_t *const buffer_r, size_t byte_amount );
        virtual ~Writer();
        
        void setPosition( int offset, Direction way = BEGIN );
        
        bool empty() const;
        bool ended() const;
        size_t totalSize() const;
        size_t getPosition( Direction way = BEGIN ) const;
        
        Writer getWriter( size_t writer_size );

        void writeU64( uint64_t content, Endian endianess = NO_SWAP );
        void writeI64(  int64_t content, Endian endianess = NO_SWAP );

        void writeU32( uint32_t content, Endian endianess = NO_SWAP );
        void writeI32(  int32_t content, Endian endianess = NO_SWAP );

        void writeU16( uint16_t content, Endian endianess = NO_SWAP );
        void writeI16(  int16_t content, Endian endianess = NO_SWAP );

        void writeU8( uint8_t content );
        void writeI8(  int8_t content );

        size_t write( std::istream &buffer, size_t byte_amount );
        
        void addToBuffer( Buffer& buffer ) const;
    };
};

}

#endif // UTILITIES_BUFFER_938223_H
