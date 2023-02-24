#include "Colorizer.h"

#include "Config.h"

#include <fstream>

namespace {
const double SHADING_VALUE = 0.0078125;

Utilities::PixelFormatColor::GenericColor getColor(
    Data::Mission::TilResource::TileGraphics tile,
    const std::vector<Utilities::PixelFormatColor::GenericColor> &colors )
{
    Utilities::PixelFormatColor::GenericColor color = { 0, 0, 0, 1 };
    
    switch( tile.type ) {
        case 0b00: // Solid Monochrome
            {
                color.setValue( static_cast<double>( tile.shading ) * SHADING_VALUE );
            }
        case 0b01: // Dynamic Monochrome
            {
                color.setValue( static_cast<double>( tile.shading & 0xFC ) * SHADING_VALUE );
            }
            break;
        case 0b10: // Dynamic Color
            {
                if( !colors.empty() )
                    color = colors.at( tile.shading % colors.size() );
            }
            break;
        case 0b11: // Lava Animation
            {
                color.red = 0.5;
            }
            break;
    }
    
    return color;
}

glm::vec3 colorToVec3( Utilities::PixelFormatColor::GenericColor color )
{
    glm::vec3 vertex_value;
    
    vertex_value.x = color.red;
    vertex_value.y = color.green;
    vertex_value.z = color.blue;
    
    return vertex_value;
}

glm::vec3 accessColor( uint8_t index, const std::vector<Utilities::PixelFormatColor::GenericColor> &colors ) {
    Utilities::PixelFormatColor::GenericColor color = { 0, 0, 0, 1 };
    
    if( !colors.empty() )
        color = colors.at( index % colors.size() );
    
    return colorToVec3( color );
}

glm::vec3 getColorVec3(
    Data::Mission::TilResource::TileGraphics tile,
    const std::vector<Utilities::PixelFormatColor::GenericColor> &colors )
{
    Utilities::PixelFormatColor::GenericColor color = getColor( tile, colors );
    
    return colorToVec3( color );
}

}

#ifdef FC_OPTION_EXPERIMENTAL_BLEND_COLORS

unsigned int Data::Mission::Til::Colorizer::setSquareColors( const Input &input, glm::vec3 *result_r )
{
    if( result_r != nullptr )
    {
        switch( TilResource::TileGraphics( input.til_graphics[ input.tile_index ] ).type ) {
            case 0b00: // Solid Monochrome
                {
                    result_r[0] = getColorVec3( TilResource::TileGraphics( input.til_graphics[ input.tile_index ] ), input.colors );
                    result_r[1] = result_r[0];
                    result_r[2] = result_r[0];
                    result_r[3] = result_r[0];
                }
                break;
            case 0b01: // Dynamic Monochrome
                {
                    result_r[1].x = static_cast<double>( TilResource::TileGraphics( input.til_graphics.at( (input.tile_index + 1) % input.til_graphics.size() ) ).shading ) * SHADING_VALUE;
                    result_r[1].y = result_r[1].x;
                    result_r[1].z = result_r[1].x;
                    result_r[2].x = static_cast<double>( TilResource::TileGraphics( input.til_graphics.at( (input.tile_index + 1) % input.til_graphics.size() ) ).getOtherShading() ) * SHADING_VALUE;
                    result_r[2].y = result_r[2].x;
                    result_r[2].z = result_r[2].x;
                    result_r[3] = result_r[0];
                }
                break;
            case 0b10: // Dynamic Color
                {
                    result_r[0] = accessColor( TilResource::TileGraphics( input.til_graphics.at( input.tile_index ) ).shading, input.colors );
                    result_r[1] = accessColor( TilResource::DynamicColorGraphics( input.til_graphics.at( input.tile_index + 1 ) ).second, input.colors );
                    result_r[2] = accessColor( TilResource::DynamicColorGraphics( input.til_graphics.at( input.tile_index + 1 ) ).third, input.colors );
                    result_r[3] = accessColor( TilResource::DynamicColorGraphics( input.til_graphics.at( input.tile_index + 2 ) ).second, input.colors );
                }
                break;
            case 0b11: // Lava Animation
                {
                    result_r[1] = glm::vec3(1.0, 1.0, 1.0) - result_r[0];
                    result_r[2] = result_r[1];
                    result_r[3] = result_r[1];
                }
                break;
        }

        return 1;
    }
    else
        return -1;
}

#else

unsigned int Data::Mission::Til::Colorizer::setSquareColors( const Input &input, glm::vec3 *result_r )
{
    if( result_r != nullptr )
    {
        result_r[0] = getColorVec3( input.til_graphics[ input.tile_index ], input.colors );
        result_r[1] = result_r[0];
        result_r[2] = result_r[0];
        result_r[3] = result_r[0];
        
        return 1;
    }
    else
        return -1;
}

#endif
