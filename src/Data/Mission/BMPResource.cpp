#include "BMPResource.h"

#include <cmath>
#include "../../Utilities/DataHandler.h"
#include "../../Utilities/ImageFormat/Chooser.h"
#include <fstream>
#include <cassert>

namespace {
// Both versions have this for this is the header for the texture.
const uint32_t CCB_TAG = 0x43434220; // which is { 0x43, 0x43, 0x42, 0x20 } or { 'C', 'C', 'B', ' ' } or "CCB "
// This is probably something for converting 16 bit data to 8 bit pallete data for Windows and Mac.
const uint32_t LKUP_TAG = 0x4C6B5570; // which is { 0x4C, 0x75, 0x55, 0x70 } or { 'L', 'k', 'U', 'p' } or "LkUp"
// Windows and Mac has the full 16 bit colors.
const uint32_t PX16_TAG = 0x50583136; // which is { 0x50, 0x58, 0x31, 0x36 } or { 'P', 'X', '1', '6' } or "PX16"
// Color pallete both used in Playstation 1 and the desktop versions.
const uint32_t PLUT_TAG = 0x504C5554; // which is { 0x50, 0x4C, 0x55, 0x54 } or { 'P', 'L', 'U', 'T' } or "PLUT"
// This is the pixel data for the Playstation 1 version, and probably the computer versions.
const uint32_t PDAT_TAG = 0x50444154; // which is { 0x50, 0x44, 0x41, 0x54 } or { 'P', 'D', 'A', 'T' } or "PDAT"

const Utilities::PixelFormatColor_R5G5B5T1 COMPUTER_COLOR_FORMAT;
const Utilities::ColorPalette COMPUTER_COLOR_PALETTE( COMPUTER_COLOR_FORMAT );

const Utilities::PixelFormatColor_B5G5R5T1 PS1_COLOR_FORMAT;
const Utilities::ColorPalette PS1_COLOR_PALETTE( PS1_COLOR_FORMAT );

}

const std::string Data::Mission::BMPResource::FILE_EXTENSION = "cbmp";
const uint32_t Data::Mission::BMPResource::IDENTIFIER_TAG = 0x43626D70; // which is { 0x43, 0x62, 0x6D, 0x70 } or { 'C', 'b', 'm', 'p' } or "Cbmp"

Data::Mission::BMPResource::BMPResource() : image_p( nullptr ), image_palette_p( nullptr ), isPSX(false), format_p(nullptr) {
}

Data::Mission::BMPResource::BMPResource( const BMPResource &obj ) : Resource( obj ), image_p( nullptr ), image_palette_p( nullptr ), isPSX(false), format_p(nullptr)
{
    for( size_t i = 0; i < sizeof(lookUpData) / sizeof(lookUpData[0]); i++ )
        lookUpData[ i ] = obj.lookUpData[ i ];
    
    if( obj.image_p != nullptr )
        image_p = new Utilities::Image2D( *obj.image_p );
        
    if( obj.image_palette_p != nullptr )
        image_palette_p = new Utilities::ImagePalette2D( *obj.image_palette_p );
    
    if( obj.format_p != nullptr ) {
        format_p = obj.format_p->duplicate();
    }
}

Data::Mission::BMPResource::~BMPResource() {
    if( image_p != nullptr )
        delete image_p;
        
    if( image_palette_p != nullptr )
        delete image_palette_p;
    
    if( format_p != nullptr ) {
        delete format_p;
    }
}

std::string Data::Mission::BMPResource::getFileExtension() const {
    return FILE_EXTENSION;
}

uint32_t Data::Mission::BMPResource::getResourceTagID() const {
    return IDENTIFIER_TAG;
}

bool Data::Mission::BMPResource::parse( const ParseSettings &settings ) {
    bool file_is_not_valid = false;
    
    if( this->data_p != nullptr )
    {
        auto reader = this->data_p->getReader();
        size_t pdat_position = 0;
        size_t pdat_size = 0;
        size_t plut_position = 0;
        size_t plut_size = 0;
        Utilities::ColorPalette color_palette( COMPUTER_COLOR_FORMAT );

        while( reader.getPosition() < reader.totalSize() ) {
            auto identifier = reader.readU32( settings.endian );
            auto tag_size   = reader.readU32( settings.endian );

            if( identifier == CCB_TAG ) {
                auto cbb_reader = reader.getReader( tag_size - sizeof( uint32_t ) * 2 );
                // TODO This probably handles the transparency, and maybe other effects.
                
                // Zero data 0x0-0xC.
                // 0x4
                assert( cbb_reader.readU32( settings.endian ) == 0 );
                // 0x8
                assert( cbb_reader.readU32( settings.endian ) == 0 );
                // 0xC = 12
                assert( cbb_reader.readU32( settings.endian ) == 0 );
                
                // Address 0x10 is always 0x01000000
                assert( cbb_reader.readU32( settings.endian ) == 0x01000000 );
                
                assert( cbb_reader.readU32( settings.endian ) == 0 );
                
                assert( cbb_reader.readU32( settings.endian ) == 0x01000000 );
                
                assert( cbb_reader.readU32( settings.endian ) == 0x01000000 );
                
                assert( cbb_reader.readU32( settings.endian ) == 0 );
                
                assert( cbb_reader.readU32( settings.endian ) == 0x01000000 );
                
                assert( cbb_reader.readU32( settings.endian ) == 8 );
                
                assert( cbb_reader.readU16( settings.endian ) == 0 );
                
                cbb_reader.setPosition( 0x2A, Utilities::Buffer::BEGIN );
                auto bitfield_byte = cbb_reader.readU8();
                
                assert( cbb_reader.readU8() == 0 );
                
                assert( cbb_reader.readU32( settings.endian ) == 0 );
                
                assert( cbb_reader.readU32( settings.endian ) == 0 );
                
                cbb_reader.setPosition( 0x34, Utilities::Buffer::BEGIN );
                auto first_u32 = cbb_reader.readU32( settings.endian );
                
                uint8_t array[4];
                
                array[ 0 ] = cbb_reader.readU8();
                array[ 1 ] = cbb_reader.readU8();
                array[ 2 ] = cbb_reader.readU8();
                array[ 3 ] = cbb_reader.readU8();
                
                auto second_u32 = cbb_reader.readU32( settings.endian );
                auto third_u32 = cbb_reader.readU32( settings.endian );
            }
            else
            if( identifier == LKUP_TAG ) {
                auto lkup_reader = reader.getReader( tag_size - sizeof( uint32_t ) * 2 );

                const size_t LOOKUP_DATA_AMOUNT = sizeof( lookUpData ) / sizeof( lookUpData[0] );

                // This seems to be all 8 bit data in 4 lists each containing 0x100 bytes.
                // In second thought
                // I could be that this is simply a big list of 0x400 unsigned bytes and the 16-bit data is simply going to scale down.
                for( unsigned int i = 0; i < LOOKUP_DATA_AMOUNT; i++ )
                    lookUpData[ i ] = lkup_reader.readU8();
                
                // Playstation is disabled.
                isPSX = false;
            }
            else
            if( identifier == PDAT_TAG ) { // For PlayStation
                
                // Save the position and size, because we want to process this later.
                pdat_position = reader.getPosition();
                pdat_size = tag_size - sizeof( uint32_t ) * 2;
                
                // Now, skip the data.
                reader.getReader( pdat_size );
                
                // Playstation is enabled
                isPSX = true;
            }
            else
            if( identifier == PX16_TAG ) { // For Windows and Macintosh
                auto px16_reader = reader.getReader( tag_size - sizeof( uint32_t ) * 2 );

                // setup the image
                if( image_p != nullptr )
                    delete image_p;
                
                this->image_p = new Utilities::Image2D( 0x100, 0x100, COMPUTER_COLOR_FORMAT );
                 
                if( !this->image_p->fromReader( px16_reader, settings.endian ) )
                    file_is_not_valid = true;
            }
            else
            if( identifier == PLUT_TAG) {
                // Save the position and size, because we want to process this later.
                plut_position = reader.getPosition();
                plut_size = tag_size - sizeof( uint32_t ) * 2;
                
                // Now, skip the data.
                reader.getReader( plut_size );
            }
            else
            {
                reader.setPosition( tag_size - sizeof( uint32_t ) * 2, Utilities::Buffer::CURRENT );
            }
        }
        
        if( plut_size != 0 ) {
            reader.setPosition( plut_position );
            auto plut_reader = reader.getReader( plut_size );

            // The color pallette is located 12 bytes away from the start of the tag.
            plut_reader.setPosition( 0xC, Utilities::Buffer::CURRENT );
            
            // Now store the color palette.
            color_palette.setAmount( 0x100 );

            if( isPSX ) {
                for( unsigned int d = 0; d <= color_palette.getLastIndex(); d++ ) {
                    color_palette.setIndex( d, PS1_COLOR_FORMAT.readPixel( plut_reader, Utilities::Buffer::Endian::LITTLE ) );
                }
            }
            else {
                for( unsigned int d = 0; d <= color_palette.getLastIndex(); d++ ) {
                    color_palette.setIndex( d, COMPUTER_COLOR_FORMAT.readPixel( plut_reader, settings.endian ) );
                }
            }
            
            if( image_palette_p != nullptr )
                delete image_palette_p;
            
            this->image_palette_p = new Utilities::ImagePalette2D( 0x100, 0x100, color_palette );
        }
        
        if( pdat_size != 0 && this->image_palette_p != nullptr ) {
            reader.setPosition( pdat_position );

            auto px_reader = reader.getReader( pdat_size );

            if( !this->image_palette_p->fromReader( px_reader ) )
                file_is_not_valid = true;
            
            if( this->image_p == nullptr ) {
                auto image = this->image_palette_p->toColorImage();
                
                this->image_p = new Utilities::Image2D( image );
                
                assert( this->image_p != nullptr );
            }
        }
        
        Utilities::ImageFormat::Chooser chooser;
        
        assert( this->image_p != nullptr );
        
        this->format_p = chooser.getWriterCopy( *getImage() );
        assert( this->format_p != nullptr );

        return !file_is_not_valid;
    }
    else
        return false;
}

Data::Mission::Resource * Data::Mission::BMPResource::duplicate() const {
    return new Mission::BMPResource( *this );
}
int Data::Mission::BMPResource::write( const std::string& file_path, const Data::Mission::IFFOptions &iff_options ) const {
    auto rgba_color = Utilities::PixelFormatColor_R8G8B8A8();

    if( iff_options.bmp.shouldWrite( iff_options.enable_global_dry_default ) && getImage() != nullptr ) {
        if( this->format_p != nullptr ) {
            Utilities::Buffer buffer;
            int state;
            
            {
                auto image_convert = Utilities::Image2D( *this->image_p, rgba_color );
                
                for( unsigned int x = 0; x <= image_convert.getWidth(); x++ ) {
                    for( unsigned int y = 0; y <= image_convert.getHeight(); y++ ) {
                        auto color = image_convert.readPixel( x, y );
                        
                        image_convert.writePixel( x, y, color );
                    }
                }
                
                state = this->format_p->write( image_convert, buffer );
                buffer.write( this->format_p->appendExtension( file_path ) );
            }

            buffer.set( nullptr, 0 ); // This effectively clears the buffer.

            {
                // Make a color palette that holds RGBA values
                Utilities::ColorPalette rgba_palette( rgba_color );
                
                rgba_palette.setAmount( 0x100 );
                
                for( unsigned int i = 0; i <= this->image_palette_p->getColorPalette()->getLastIndex(); i++ ) {
                    auto color =  this->image_palette_p->getColorPalette()->getIndex( i );
                    
                    rgba_palette.setIndex( i, color );
                }
                
                auto palette = Utilities::ImagePalette2D( rgba_palette );
                
                state = this->format_p->write( palette, buffer );
                buffer.write( this->format_p->appendExtension( file_path + "_paletted" ) );
            }

            return state;
        }
        else
            return 0;
    }
    else
        return 0;
}

const Utilities::ImageFormat::ImageFormat *const Data::Mission::BMPResource::getImageFormat() const {
    return format_p;
}

Utilities::Image2D *const Data::Mission::BMPResource::getImage() const {
    return image_p;
}

Utilities::ImagePalette2D *const Data::Mission::BMPResource::getPaletteImage() const {
    return image_palette_p;
}

std::vector<Data::Mission::BMPResource*> Data::Mission::BMPResource::getVector( Data::Mission::IFF &mission_file ) {
    std::vector<Resource*> to_copy = mission_file.getResources( Data::Mission::BMPResource::IDENTIFIER_TAG );

    std::vector<BMPResource*> copy;

    copy.reserve( to_copy.size() );

    for( auto it = to_copy.begin(); it != to_copy.end(); it++ ) {
        assert( dynamic_cast<BMPResource*>( (*it) ) );
        copy.push_back( dynamic_cast<BMPResource*>( (*it) ) );
    }

    return copy;
}

const std::vector<Data::Mission::BMPResource*> Data::Mission::BMPResource::getVector( const Data::Mission::IFF &mission_file ) {
    return Data::Mission::BMPResource::getVector( const_cast< IFF& >( mission_file ) );
}

bool Data::Mission::IFFOptions::BMPOption::readParams( std::map<std::string, std::vector<std::string>> &arguments, std::ostream *output_r ) {
    if( !singleArgument( arguments, "--" + getNameSpace() + "_PALETTE", output_r, export_palette ) )
        return false; // The single argument is not valid.

    return IFFOptions::ResourceOption::readParams( arguments, output_r );
}
