#include "Mesh.h"
#include <fstream>

namespace {
    const float TILE_CORNER_POSITION_X[4] = { 0.5, -0.5,  0.5, -0.5 };
    const float TILE_CORNER_POSITION_Z[4] = { 0.5,  0.5, -0.5, -0.5 };

    #include "TileSet.h"
}


unsigned int Data::Mission::Til::Mesh::getNeighboor( unsigned int index, int next_amount ) {
    int increment;
    bool not_found;

    if( next_amount > 0)
        increment = -1;
    else
        increment =  1;

    for( int timeout = 0; 0 != next_amount && timeout < 0x80; next_amount += increment, timeout++ ) {
        not_found = true;

        while( not_found ) {
            index -= increment;
            index = index & 0x7F;
            not_found = default_mesh[index].points[0].heightmap_channel == NO_ELEMENT;
        }
    }

    return index;
}

unsigned int Data::Mission::Til::Mesh::BuildTriangle( const Input &input, const Polygon &triangle, VertexData &result, bool flipped ) {
    const unsigned int ELEMENT_AMOUNT = 3;

    static const unsigned INDEX_TABLE[2][3] = { { 0, 1, 2 }, { 2, 3, 0 } };
    
    if( result.element_amount >= ELEMENT_AMOUNT + result.element_start )
    {
        for( unsigned int i = 0; i < ELEMENT_AMOUNT; i++ ) {
            
            result.position[ result.element_start ].x = TILE_CORNER_POSITION_X[ triangle.points[ i ].facing_direction ];
            result.position[ result.element_start ].y = static_cast<float>(input.pixels[ triangle.points[ i ].facing_direction ]->channel[ triangle.points[ i ].heightmap_channel ]) * TilResource::SAMPLE_HEIGHT;
            result.position[ result.element_start ].z = TILE_CORNER_POSITION_Z[ triangle.points[ i ].facing_direction ];
            
            const unsigned int INDEX = (input.coord_index + INDEX_TABLE[ flipped ][ i ] ) % input.coord_index_limit;
            result.coords[ result.element_start ].x = input.coord_data[ INDEX ].x;
            result.coords[ result.element_start ].y = input.coord_data[ INDEX ].y;

            result.element_start++;
        }

        return ELEMENT_AMOUNT;
    }
    else
        return 0;
}

unsigned int Data::Mission::Til::Mesh::BuildQuad( const Input &input, const Polygon &quad, VertexData &result ) {
    unsigned int number_of_written_vertices = 0;

    number_of_written_vertices += Data::Mission::Til::Mesh::BuildTriangle( input, quad, result );

    Polygon other_triangle;
    other_triangle.points[0] = quad.points[2];
    other_triangle.points[1] = quad.points[3];
    other_triangle.points[2] = quad.points[0];

    number_of_written_vertices += Data::Mission::Til::Mesh::BuildTriangle( input, other_triangle, result, true );

    return number_of_written_vertices;
}

unsigned int Data::Mission::Til::Mesh::createTile( const Input &input, VertexData &vertex_data, unsigned int tileType ) {
    unsigned number_of_written_vertices = 0;
    bool found = false;
    Polygon tile_polygon;

    tile_polygon = default_mesh[ tileType ];

    if( tile_polygon.points[0].heightmap_channel != NO_ELEMENT )
    {
        vertex_data.colors[ 0 ] = input.colors[ 0 ];
        vertex_data.colors[ 1 ] = input.colors[ 1 ];
        vertex_data.colors[ 2 ] = input.colors[ 2 ];

        if( tile_polygon.points[3].heightmap_channel == NO_ELEMENT )
            number_of_written_vertices += BuildTriangle( input, tile_polygon, vertex_data );
        else
        {
            vertex_data.colors[ 3 ] = input.colors[ 2 ];
            vertex_data.colors[ 4 ] = input.colors[ 3 ];
            vertex_data.colors[ 5 ] = input.colors[ 0 ];
            number_of_written_vertices += BuildQuad( input, tile_polygon, vertex_data );
        }
    }


    return number_of_written_vertices;
}

bool Data::Mission::Til::Mesh::isWall( unsigned int tile_type ) {
    const unsigned ARRAY_LENGTH = 4;
    unsigned number_array[ARRAY_LENGTH] = {0, 0, 0, 0};
    unsigned number_of_twos = 0;
    unsigned number_of_ones = 0;

    // Buffer overflow check.
    if( tile_type >= sizeof( default_mesh ) / sizeof( default_mesh[0] ) ) {
        return false;
    }

    unsigned number_of_corners = 4;

    if( default_mesh[tile_type].points[3].heightmap_channel == NO_ELEMENT )
        number_of_corners = 3;

    for( unsigned i = 0; i < number_of_corners; i++ ) {
        number_array[ default_mesh[tile_type].points[i].facing_direction ]++;
    }

    if( number_of_corners == 3 ) { // 3 side case.
        for( unsigned i = 0; i < ARRAY_LENGTH; i++ ) {
            if( number_array[ i ] == 0 ) { // Do nothing.
            } else
            if( number_array[ i ] == 1 ) { // Increment the number of ones.
                number_of_ones++;
            } else
            if( number_array[ i ] == 2 ) { // Increment the number of twos.
                number_of_twos++;
            } else
                return false; // Abort this search.
        }

        if( number_of_ones == 1 && number_of_twos == 1 )
            return true; // Succeeded.
        else
            return false;
    }
    else { // 4 side case.
        for( unsigned i = 0; i < ARRAY_LENGTH; i++ ) {
            if( number_array[ i ] == 0 ) { // Do nothing.
            } else
            if( number_array[ i ] == 1 ) { // This is not a wall then.
                return false;
            } else
            if( number_array[ i ] == 2 ) { // Increment the number of twos.
                number_of_twos++;
            } else
                return false; // Abort this search.
        }

        if( number_of_twos == 2 )
            return true; // Succeeded.
        else
            return false;
    }
}

bool Data::Mission::Til::Mesh::isSlope( unsigned int tile_type ) {
    const unsigned ARRAY_LENGTH = 4;
    unsigned number_array[ARRAY_LENGTH] = {0, 0, 0, 0};
    unsigned number_of_corners = 4;

    // Buffer overflow check.
    if( tile_type >= sizeof( default_mesh ) / sizeof( default_mesh[0] ) ) {
        return false;
    }

    if( default_mesh[tile_type].points[0].heightmap_channel == NO_ELEMENT )
        return false;

    if( default_mesh[tile_type].points[3].heightmap_channel == NO_ELEMENT )
        number_of_corners = 3;

    for( unsigned i = 1; i < number_of_corners; i++ ) {
        if( default_mesh[tile_type].points[ 0 ].heightmap_channel != default_mesh[tile_type].points[ i ].heightmap_channel )
            return true;
    }
    return false;
}


bool Data::Mission::Til::Mesh::isFliped( unsigned int tile_type ) {
    // Buffer overflow check.
    if( tile_type >= sizeof( default_mesh ) / sizeof( default_mesh[0] ) ) {
        return false;
    }

    return default_mesh[tile_type].is_opposite;
}
