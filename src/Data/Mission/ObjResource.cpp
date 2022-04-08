#include "ObjResource.h"

#include "IFF.h"

#include "../../Utilities/DataHandler.h"
#include "../../Utilities/Math.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cassert>

namespace {
    // The header
    const uint32_t TAG_4DGI = 0x34444749; // which is { 0x34, 0x44, 0x47, 0x49 } or { '4', 'D', 'G', 'I' } or "4DGI"
    // Texture reference list
    const uint32_t TAG_3DTL = 0x3344544C; // which is { 0x33, 0x44, 0x54, 0x4C } or { '3', 'D', 'T', 'L' } or "3DTL"
    // Face Quad list holding offsets of vertices and normals.
    const uint32_t TAG_3DQL = 0x3344514C; // which is { 0x33, 0x44, 0x51, 0x4C } or { '3', 'D', 'Q', 'L' } or "3DQL"
    // 3D reference?
    const uint32_t TAG_3DRF = 0x33445246; // which is { 0x33, 0x44, 0x52, 0x46 } or { '3', 'D', 'R', 'F' } or "3DRF"
    // 3D reference list?
    const uint32_t TAG_3DRL = 0x3344524C; // which is { 0x33, 0x44, 0x52, 0x4C } or { '3', 'D', 'R', 'L' } or "3DRL"
    // Bones.
    const uint32_t TAG_3DHY = 0x33444859; // which is { 0x33, 0x44, 0x48, 0x59 } or { '3', 'D', 'H', 'Y' } or "3DHY"
    // Positions of other objects
    const uint32_t TAG_3DHS = 0x33444853; // which is { 0x33, 0x44, 0x48, 0x53 } or { '3', 'D', 'H', 'S' } or "3DHS"
    // Bone Animation Attributes.
    const uint32_t TAG_3DMI = 0x33444D49; // which is { 0x33, 0x44, 0x4d, 0x49 } or { '3', 'D', 'M', 'I' } or "3DMI"
    // 3D triangle array?
    const uint32_t TAG_3DTA = 0x33445441; // which is { 0x33, 0x44, 0x54, 0x41 } or { '3', 'D', 'T', 'A' } or "3DTA"
    // 3D array list?
    const uint32_t TAG_3DAL = 0x3344414C; // which is { 0x33, 0x44, 0x41, 0x4C } or { '3', 'D', 'A', 'L' } or "3DAL"
    // 4D vertex list (Note: Ignore the 4D data).
    const uint32_t TAG_4DVL = 0x3444564C; // which is { 0x33, 0x44, 0x56, 0x4C } or { '4', 'D', 'V', 'L' } or "4DVL"
    // 4D normal list
    const uint32_t TAG_4DNL = 0x34444E4C; // which is { 0x34, 0x44, 0x4E, 0x4C } or { '4', 'D', 'N', 'L' } or "4DNL"
    // Animation track data
    const uint32_t TAG_AnmD = 0x416e6d44; // which is { 0x41, 0x6e, 0x6d, 0x44 } or { 'A', 'n', 'm', 'D' } or "AnmD"
    // 3D bounding box
    const uint32_t TAG_3DBB = 0x33444242; // which is { 0x33, 0x44, 0x42, 0x42 } or { '3', 'D', 'B', 'B' } or "3DBB"

    const auto INTEGER_FACTOR = 1.0 / 256.0;

    void triangleToCoords( const Data::Mission::ObjResource::FaceTriangle &triangle, Utilities::DataTypes::Vec2UByte *coords )
    {
        if( !triangle.is_other_side )
        {
            coords[0] = triangle.texture_quad_ref->coords[0];
            coords[1] = triangle.texture_quad_ref->coords[1];
            coords[2] = triangle.texture_quad_ref->coords[2];
        }
        else
        {
            coords[0] = triangle.texture_quad_ref->coords[2];
            coords[1] = triangle.texture_quad_ref->coords[3];
            coords[2] = triangle.texture_quad_ref->coords[0];
        }
    }

    void handlePositions( Utilities::DataTypes::Vec3 &position, const Utilities::DataTypes::Vec3Short *array, int index ) {
        position.x = -array[ index ].x * INTEGER_FACTOR;
        position.y =  array[ index ].y * INTEGER_FACTOR;
        position.z =  array[ index ].z * INTEGER_FACTOR;
    }
    void handleNormals( Utilities::DataTypes::Vec3 &normal, const Utilities::DataTypes::Vec3Short *array, int index ) {
        normal.x = array[ index ].x;
        normal.y = array[ index ].y;
        normal.z = array[ index ].z;

        normal.normalize();
    }
}

bool Data::Mission::ObjResource::TextureQuad::isWithinBounds( size_t texture_amount ) const {
    if( index >= 0 )
    {
        return (static_cast<size_t>(index) > texture_amount);
    }
    else
    if( index < 0 )
    {
        return (static_cast<size_t>(-index - 1) > texture_amount);
    }
}

bool Data::Mission::ObjResource::FaceTriangle::isWithinBounds( size_t vertex_limit, size_t normal_limit, size_t texture_quad_limit, const TextureQuad *origin ) const {
    bool is_valid = true;

    if( static_cast<unsigned int>(texture_quad_ref - origin) >= texture_quad_limit )
        is_valid = false;
    else
    if( this->v0 >= vertex_limit )
        is_valid = false;
    else
    if( this->v1 >= vertex_limit )
        is_valid = false;
    else
    if( this->v2 >= vertex_limit )
        is_valid = false;
    else
    if( this->n0 >= normal_limit )
        is_valid = false;
    else
    if( this->n1 >= normal_limit )
        is_valid = false;
    else
    if( this->n2 >= normal_limit )
        is_valid = false;

    return is_valid;
}

bool Data::Mission::ObjResource::FaceTriangle::operator() ( const FaceTriangle & l_operand, const FaceTriangle & r_operand ) const {
    if( l_operand.texture_quad_ref != nullptr && r_operand.texture_quad_ref != nullptr )
        return (l_operand.texture_quad_ref->index < r_operand.texture_quad_ref->index);
    else
    if( l_operand.texture_quad_ref != nullptr )
        return false;
    else
        return true;
}

Data::Mission::ObjResource::FaceTriangle Data::Mission::ObjResource::FaceQuad::firstTriangle() const {
    FaceTriangle newTri;

    newTri.is_other_side = false;
    newTri.is_reflective = is_reflective;
    newTri.texture_quad_ref = texture_quad_ref;
    newTri.v0 = v0;
    newTri.v1 = v1;
    newTri.v2 = v2;
    newTri.n0 = n0;
    newTri.n1 = n1;
    newTri.n2 = n2;

    return newTri;
}

Data::Mission::ObjResource::FaceTriangle Data::Mission::ObjResource::FaceQuad::secondTriangle() const {
    FaceTriangle newTri;

    newTri.is_other_side = true;
    newTri.is_reflective = is_reflective;
    newTri.texture_quad_ref = texture_quad_ref;
    newTri.v0 = v2;
    newTri.v1 = v3;
    newTri.v2 = v0;
    newTri.n0 = n2;
    newTri.n1 = n3;
    newTri.n2 = n0;

    return newTri;

}

unsigned int Data::Mission::ObjResource::Bone::getNumAttributes() const {
    return (getOpcodeBytesPerFrame( this->opcode ) / 2);
}
        
const std::string Data::Mission::ObjResource::FILE_EXTENSION = "cobj";
const uint32_t Data::Mission::ObjResource::IDENTIFIER_TAG = 0x436F626A; // which is { 0x43, 0x6F, 0x62, 0x6A } or { 'C', 'o', 'b', 'j' } or "Cobj"

Data::Mission::ObjResource::ObjResource() {
    this->bone_frames = 0;
    this->max_bone_childern = 0;
    this->bone_animation_data = nullptr;
}

Data::Mission::ObjResource::ObjResource( const ObjResource &obj ) : ModelResource( obj ) {
    this->bounding_box_frames = 0;
    this->bone_frames = 0;
    this->max_bone_childern = 0;
    this->bone_animation_data = nullptr;
    if( obj.bone_animation_data != nullptr )
    {
        this->bone_animation_data_size = obj.bone_animation_data_size;
        this->bone_animation_data = new int16_t [ obj.bone_animation_data_size ];
    }
}

Data::Mission::ObjResource::~ObjResource() {
    delete [] this->bone_animation_data;
} 

std::string Data::Mission::ObjResource::getFileExtension() const {
    return FILE_EXTENSION;
}

uint32_t Data::Mission::ObjResource::getResourceTagID() const {
    return IDENTIFIER_TAG;
}

bool Data::Mission::ObjResource::parse( const Utilities::Buffer &header, const Utilities::Buffer &reader_data, const ParseSettings &settings ) {
    auto raw_data = reader_data.getReader().getBytes();

    bool file_is_not_valid = true;
    auto data = raw_data.data();
    
    // This is for testing mostly.
    uint8_t *data_3DMI       = nullptr;
    int data_3DMI_size       =  0;
    int bytes_per_frame_3DMI =  0;
    int frames_gen_3DHS      = -1;
    int frames_gen_AnmD      = -1;
    

    while( static_cast<unsigned int>(data - raw_data.data()) < raw_data.size() ) {
        auto identifier = Utilities::DataHandler::read_u32( data, settings.is_opposite_endian );
        auto tag_size   = Utilities::DataHandler::read_u32( data + sizeof( uint32_t ), settings.is_opposite_endian );
        auto start_data = data + sizeof( uint32_t ) * 2;

        *settings.output_ref << std::hex;

        if( identifier == TAG_4DGI ) {
            file_is_not_valid = false;

            // It always has a size of 0x3C for the full chunk size;
            if( tag_size != 0x3C && settings.output_level >= 1 )
            {
                *settings.output_ref << "Mission::ObjResource::load() 4DGI should have size of 0x3c not 0x"
                          << tag_size << std::endl;
            }
            else
            {
                if( settings.output_level >= 2 )
                    *settings.output_ref << "Mission::ObjResource::load() index #"
                        << std::dec << getIndexNumber() << std::hex
                        << " at location 0x" << this->getOffset() << std::endl;

                auto un1 = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint32_t );
                auto number_of_bone_pos_frames = Utilities::DataHandler::read_u16( start_data , settings.is_opposite_endian);
                start_data += sizeof( uint16_t );
                auto un3 = Utilities::DataHandler::read_u16( start_data, settings.is_opposite_endian ); // Could be a checksum?

                start_data += 0xc; // Skip the zeros.
                //At offset 0xc 

                auto un4 = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint32_t );
                auto un5 = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint32_t );
                auto un6 = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint32_t );
                auto un7 = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint32_t );
                auto un8 = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint8_t ); // 0x30
                auto un9 = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint8_t ); // 0x31
                auto un10 = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint8_t ); // 0x32
                auto un11 = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint8_t ); // 0x33
                auto un12 = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint32_t ); // 0x34
                auto un13 = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint32_t ); // 0x38
                auto un14 = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint32_t ); // 0x3C

                // The file is then proven to be valid.
                file_is_not_valid = false;
            }
        }
        else
        if( identifier == TAG_3DTL ) {

            if( Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian ) != 1 && settings.output_level >= 1 )
            {
                *settings.output_ref << "Mission::ObjResource::load() 3DTL unexpected number at beginning!" << std::endl;
            }

            start_data += sizeof( uint32_t );

            auto texture_ref_amount = ((data + tag_size) - start_data) / 0x10;
            if( settings.output_level >= 2 )
                *settings.output_ref << "triangle amount " << std::dec << texture_ref_amount << std::hex << std::endl;

            for( auto i = 0; i < texture_ref_amount; i++ )
            {
                /*if( DataHandler::read_u8( start_data ) != 2 )
                    std::cout << "Mission::ObjResource::load() expected 2 at uv not " << std::dec
                              << (static_cast<uint32_t>(DataHandler::read_u8( start_data )) & 0xFF)
                              << std::hex << std::endl;*/
                start_data += sizeof( uint8_t );

                for( auto i = 0; i < 3; i++ ) {
                    start_data += sizeof( uint8_t );
                }

                texture_quads.push_back( TextureQuad() );

                texture_quads.back().coords[0].x = Utilities::DataHandler::read_u8( start_data );
                texture_quads.back().coords[0].y = Utilities::DataHandler::read_u8( start_data + 1 );
                start_data += sizeof( uint16_t );

                texture_quads.back().coords[1].x = Utilities::DataHandler::read_u8( start_data );
                texture_quads.back().coords[1].y = Utilities::DataHandler::read_u8( start_data + 1 );
                start_data += sizeof( uint16_t );

                texture_quads.back().coords[2].x = Utilities::DataHandler::read_u8( start_data );
                texture_quads.back().coords[2].y = Utilities::DataHandler::read_u8( start_data + 1 );
                start_data += sizeof( uint16_t );

                texture_quads.back().coords[3].x = Utilities::DataHandler::read_u8( start_data );
                texture_quads.back().coords[3].y = Utilities::DataHandler::read_u8( start_data + 1 );
                start_data += sizeof( uint16_t );
                
                // For some reason the windows english version of Slim's 64th model that goes beyond 10 textures.
                
                texture_quads.back().index = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint32_t );
            }
        }
        else
        if( identifier == TAG_3DQL ) {
            
            if( Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian ) != 1 && settings.output_level >= 1 )
            {
                *settings.output_ref << "Mission::ObjResource::load() 3DQL unexpected number at beginning!" << std::endl;
            }
            start_data += sizeof( uint32_t );

            auto number_of_faces = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
            start_data += sizeof( uint32_t );

            if( settings.output_level >= 2 )
                *settings.output_ref << "Mission::ObjResource::load() 3DQL has 0x" << number_of_faces << " faces" << std::endl;

            for( unsigned int i = 0; i < number_of_faces; i++ )
            {
                auto face_type = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint8_t );

                // Skip unknown bytes!
                auto face_type_2 = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint8_t );

                auto texture_quad_ref = (Utilities::DataHandler::read_u16( start_data, settings.is_opposite_endian ) / 0x10) + texture_quads.data();
                start_data += sizeof( uint16_t );

                /*
                // Add to the indexes of this texture to the list when it is not contained in the data base.
                if( !std::binary_search( texture_dependences.begin(), texture_dependences.end(), FaceTriangle() ) && texture_quad_ref - texture_quads.data() > texture_quads.size() ) {
                    texture_dependences.push_back( texture_quad_ref->index );
                    std::sort( texture_dependences.begin(), texture_dependences.end() );
                }*/
                bool reflect = ((face_type_2 & 0xF0) == 0x80);
                bool is_quad;
                bool is_triangle;
                
                if( settings.type == Data::Mission::Resource::ParseSettings::Macintosh ) {
                    is_quad     = ((face_type_2 & 0xF0) == 0x40);
                    is_triangle = ((face_type_2 & 0xF0) == 0x30);
                }
                else
                {
                    is_quad     = ((face_type_2 & 0x0F) == 0x04);
                    is_triangle = ((face_type_2 & 0x0F) == 0x03);
                }

                if( is_quad )
                {
                    face_quads.push_back( FaceQuad() );

                    face_quads.back().texture_quad_ref = texture_quad_ref;
                    // (((face_type & 0xFF) == 0xCC) & ((face_type_2 & 0x84) == 0x84))
                    // 0x07 Appears to be its own face type
                    face_quads.back().is_reflective = reflect;

                    face_quads.back().v0 = Utilities::DataHandler::read_u8( start_data + 0 );
                    face_quads.back().v1 = Utilities::DataHandler::read_u8( start_data + 1 );
                    face_quads.back().v2 = Utilities::DataHandler::read_u8( start_data + 2 );
                    face_quads.back().v3 = Utilities::DataHandler::read_u8( start_data + 3 );
                    
                    face_quads.back().n0 = Utilities::DataHandler::read_u8( start_data + 4 );
                    face_quads.back().n1 = Utilities::DataHandler::read_u8( start_data + 5 );
                    face_quads.back().n2 = Utilities::DataHandler::read_u8( start_data + 6 );
                    face_quads.back().n3 = Utilities::DataHandler::read_u8( start_data + 7 );
                }
                else
                if( is_triangle )
                {
                    face_trinagles.push_back( FaceTriangle() );

                    face_trinagles.back().is_other_side = false;
                    face_trinagles.back().texture_quad_ref = texture_quad_ref;
                    // (face_type & 0x08) seems to be the effect bit.
                    // (face_type & 0x28) seems to be the tranlucent bit.
                    face_trinagles.back().is_reflective = reflect;

                    face_trinagles.back().v0 = Utilities::DataHandler::read_u8( start_data + 0 );
                    face_trinagles.back().v1 = Utilities::DataHandler::read_u8( start_data + 1 );
                    face_trinagles.back().v2 = Utilities::DataHandler::read_u8( start_data + 2 );

                    face_trinagles.back().n0 = Utilities::DataHandler::read_u8( start_data + 4 );
                    face_trinagles.back().n1 = Utilities::DataHandler::read_u8( start_data + 5 );
                    face_trinagles.back().n2 = Utilities::DataHandler::read_u8( start_data + 6 );
                }
                else
                {
                    //  TODO Not all the faces are discovered!
                }
                // Once the face is successfully read the vertices and normal indeces can be skipped.
                start_data += sizeof( uint32_t ) * 2;
            }
        }
        else
        if( identifier == TAG_3DRF ) {
            // std::cout << "Mission::ObjResource::load() 3DRF" << std::endl;
        }
        else
        if( identifier == TAG_3DRL ) {
            // std::cout << "Mission::ObjResource::load() 3DRL" << std::endl;
        }
        else
        if( identifier == TAG_3DHY ) {

            if( Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian ) != 1 && settings.output_level >= 1)
            {
                *settings.output_ref << "Mission::ObjResource::load() 3DHY unexpected number at beginning!" << std::endl;
            }
            start_data += sizeof( uint32_t );

            const auto amount_of_bones = (tag_size - (start_data - data)) / 0x14;

            if( settings.output_level >= 2 )
                *settings.output_ref << "Mission::ObjResource::load() 3DHY: size = " << amount_of_bones << std::endl;

            if( ((tag_size - (start_data - data)) % 0x14) != 0 && settings.output_level >= 1 )
                *settings.output_ref << "Mission::ObjResource::load() 3DHY bones are not right!" << std::endl;
            
            this->bones.reserve( amount_of_bones );
            
            this->max_bone_childern = 0;

            for( int i = 0; i < amount_of_bones; i++ ) {
                // This statement allocates a bone, but it reads the opcode of the bone first since I want the opcode to only be written once.
                bones.push_back( Bone() );
                
                bones.at(i).parent_amount = Utilities::DataHandler::read_u8( start_data + 0x00 );
                bones.at(i).normal_start  = Utilities::DataHandler::read_u8( start_data + 0x01 );
                bones.at(i).normal_stride = Utilities::DataHandler::read_u8( start_data + 0x02 );
                bones.at(i).vertex_start  = Utilities::DataHandler::read_u8( start_data + 0x03 );
                bones.at(i).vertex_stride = Utilities::DataHandler::read_u8( start_data + 0x04 );
                // assert( Utilities::DataHandler::read_u8(start_data + 0x5) ); // can be zero
                // assert( Utilities::DataHandler::read_u8(start_data + 0x6) ); // can be zero
                auto opcode = Utilities::DataHandler::read_u8( start_data + 0x07 );
                
                bones.at(i).opcode.unknown          = (opcode & 0b11000000) >> 6;
                bones.at(i).opcode.position.x_const = (opcode & 0b00100000) >> 5;
                bones.at(i).opcode.position.y_const = (opcode & 0b00010000) >> 4;
                bones.at(i).opcode.position.z_const = (opcode & 0b00001000) >> 3;
                bones.at(i).opcode.rotation.x_const = (opcode & 0b00000100) >> 2;
                bones.at(i).opcode.rotation.y_const = (opcode & 0b00000010) >> 1;
                bones.at(i).opcode.rotation.z_const = (opcode & 0b00000001) >> 0;
                
                bones.at(i).position.x = Utilities::DataHandler::read_u16( start_data + 0x08, settings.is_opposite_endian );
                bones.at(i).position.y = Utilities::DataHandler::read_u16( start_data + 0x0A, settings.is_opposite_endian );
                bones.at(i).position.z = Utilities::DataHandler::read_u16( start_data + 0x0C, settings.is_opposite_endian );
                bones.at(i).rotation.x = Utilities::DataHandler::read_u16( start_data + 0x0E, settings.is_opposite_endian );
                bones.at(i).rotation.y = Utilities::DataHandler::read_u16( start_data + 0x10, settings.is_opposite_endian );
                bones.at(i).rotation.z = Utilities::DataHandler::read_u16( start_data + 0x12, settings.is_opposite_endian );

                bytes_per_frame_3DMI += getOpcodeBytesPerFrame( bones.at(i).opcode );
                
                this->max_bone_childern = std::max( bones.at(i).parent_amount, this->max_bone_childern );
                
                if( settings.output_level >= 3 )
                {
                    *settings.output_ref << "bone: ";

                    if( Utilities::DataHandler::read_u16( start_data + 0x5, settings.is_opposite_endian ) != 0 ) {
                        *settings.output_ref << "They are not zeros! :";
                    }

                    *settings.output_ref 
                        << "parent index: 0x" << bones.at(i).parent_amount << ", "
                        << "0x" <<    bones.at(i).normal_start << " with 0x" <<  bones.at(i).normal_stride << " normals, "
                        << "0x" <<    bones.at(i).vertex_start << " with 0x" <<  bones.at(i).vertex_stride << " vertices, "
                        << "opcode: 0x" << opcode << std::dec
                        << ", position( " << bones.at(i).position.x << ", " << bones.at(i).position.y << ", " << bones.at(i).position.z << " )"
                        << ", rotation( " << bones.at(i).rotation.x << ", " << bones.at(i).rotation.y << ", " << bones.at(i).rotation.z << " )" << std::hex << std::endl;
                }

                start_data += 0x14; // Each bone has a size of 0x14 bytes or 20 bytes in other words.
            }
            
            if( settings.output_level >= 3 )
            {
                // The bytes_per_frame_3DMI might not actually hold true.
                // I found out that the position and the rotation contains the index to 3DMI.
                *settings.output_ref << "bytes_per_frame_3DMI = 0x" << bytes_per_frame_3DMI << std::endl;
            }
        }
        else
        if( identifier == TAG_3DHS ) {
            
            // This name was given as a wild guess.
            auto bone_depth_number = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
            start_data += sizeof( uint32_t );
            auto data_size = (tag_size - (start_data - data));

            auto read_3D_positions = data_size / ( sizeof( uint16_t ) * 4 ); // vec3 with an empty space.
            
            frames_gen_3DHS = data_size / ( 4 * sizeof( uint16_t ) * bone_depth_number );

            if( settings.output_level >= 2 )
            {
                *settings.output_ref
                    << std::dec
                    << "Mission::ObjResource::load() 3DHS has " << read_3D_positions << " 3D vectors, and contains about " << frames_gen_3DHS << " frames." << std::endl
                    << std::hex;
            }
            
            for( int d = 0; d < frames_gen_3DHS; d++ )
            {
                for( int i = 0; i < bone_depth_number; i++ )
                {
                    auto u_x = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                    start_data += sizeof( uint16_t );
                    auto u_y = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                    start_data += sizeof( uint16_t );
                    auto u_z = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                    start_data += sizeof( uint16_t );
                    // I determined that this value stays zero, so no reading needs to be done.
                    // auto u_w = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                    start_data += sizeof( uint16_t );
                }
            }
        }
        else
        if( identifier == TAG_3DMI ) {
            if( Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian ) != 1 && settings.output_level >= 1 )
                *settings.output_ref << "Mission::ObjResource::load() 3DMI unexpected number at beginning!" << std::endl;
            start_data += sizeof( uint32_t );
            
            this->bone_animation_data_size = (tag_size - (start_data - data)) / sizeof( uint16_t );
            
            if( this->bone_animation_data != nullptr )
                delete [] this->bone_animation_data;
            
            this->bone_animation_data = new int16_t [ this->bone_animation_data_size ];
            
            for( int d = 0; d < this->bone_animation_data_size; d++ )
            {
                this->bone_animation_data[ d ] = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                start_data += sizeof( int16_t );
            }
        }
        else
        if( identifier == TAG_3DTA ) {
            if( settings.output_level >= 2 )
                *settings.output_ref << "Mission::ObjResource::load() 3DTA" << std::endl;
        }
        else
        if( identifier == TAG_3DAL ) {
            if( settings.output_level >= 2 )
                *settings.output_ref << "Mission::ObjResource::load() 3DAL" << std::endl;
        }
        else
        if( identifier == TAG_4DVL ) {
            auto start_number = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
            start_data += sizeof( uint32_t );
            auto amount_of_vertices = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
            start_data += sizeof( uint32_t );
            
            if( settings.output_level >= 2 )
                *settings.output_ref << "Mission::ObjResource::load() 4DVL has 0x" << amount_of_vertices << " vertices" << std::endl;

            auto positions_pointer = &vertex_positions;

            // If vertex_positions is not empty then it is a morph target.
            if( positions_pointer->size() != 0 ) {
                vertex_anm_positions.push_back( std::vector< Utilities::DataTypes::Vec3Short >() );
                positions_pointer = &vertex_anm_positions.back();
            }

            positions_pointer->resize( amount_of_vertices );

            for( unsigned int i = 0; i < amount_of_vertices; i++ ) {
                positions_pointer->at(i).x = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint16_t );

                positions_pointer->at(i).y = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint16_t );

                positions_pointer->at(i).z = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint16_t );

                // positions_pointer->at(i).w = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint16_t );
            }
        }
        else
        if( identifier == TAG_4DNL ) {
            auto start_number = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
            start_data += sizeof( uint32_t );
            auto amount_of_normals = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
            start_data += sizeof( uint32_t );

            if( settings.output_level >= 2 )
                *settings.output_ref << "Mission::ObjResource::load() 4DNL has 0x" << amount_of_normals << " normals" << std::endl;

            auto normals_pointer = &vertex_normals;

            if( normals_pointer->size() != 0 ) {
                vertex_anm_normals.push_back( std::vector< Utilities::DataTypes::Vec3Short >() );
                normals_pointer = &vertex_anm_normals.back();
            }

            normals_pointer->resize( amount_of_normals );

            for( unsigned int i = 0; i < amount_of_normals; i++ ) {
                normals_pointer->at(i).x = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint16_t );

                normals_pointer->at(i).y = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint16_t );

                normals_pointer->at(i).z = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint16_t );

                // normals_pointer->at(i).w = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint16_t );
            }
        }
        else
        if( identifier == TAG_AnmD ) {

            if( Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian ) != 1 && settings.output_level >= 1 )
                *settings.output_ref << "Mission::ObjResource::load() AnmD unexpected number at beginning!" << std::endl;
            start_data += sizeof( uint32_t );

            const auto TRACK_AMOUNT = (tag_size - (start_data - data)) / 0x10;

            if( settings.output_level >= 2 )
                *settings.output_ref << std::dec << "Mission::ObjResource::load() AnmD has " << TRACK_AMOUNT << " tracks" << std::endl << std::hex;

            unsigned int start = 0xFFFF, end = 0x0;
            
            for( unsigned int i = 0; i < TRACK_AMOUNT; i++ ) {
                auto u0 = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint8_t );
                auto u1 = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint8_t );
                auto u2 = Utilities::DataHandler::read_u8( start_data ); // This seems to be only zero
                start_data += sizeof( uint8_t );
                auto u3 = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint8_t );
                auto from_frame = Utilities::DataHandler::read_u16( start_data, settings.is_opposite_endian );// This is the staring frame
                start_data += sizeof( uint16_t );
                auto to_frame = Utilities::DataHandler::read_u16( start_data, settings.is_opposite_endian ); // This is the ending frame.
                start_data += sizeof( uint16_t );
                auto u6 = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint8_t );
                auto u7 = Utilities::DataHandler::read_u8( start_data );
                start_data += sizeof( uint8_t );
                auto u8 = Utilities::DataHandler::read_u16( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint16_t );
                auto frame_duration = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
                start_data += sizeof( uint32_t );
                
                start = std::min( start, static_cast<unsigned int>( from_frame ) );
                start = std::min( start, static_cast<unsigned int>( to_frame ) );
                
                end = std::max( end, static_cast<unsigned int>( from_frame ) );
                end = std::max( end, static_cast<unsigned int>( to_frame ) );

                if( settings.output_level >= 2 )
                    *settings.output_ref << std::dec
                          << "f: " << from_frame << " t: " << to_frame
                          << " f.d: " << frame_duration << std::endl
                          << std::hex;
            }
            
            frames_gen_AnmD = end - start + 1;
        }
        else
        if( identifier == TAG_3DBB ) {
            bounding_box_per_frame = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian );
            
            if( bounding_box_per_frame >= 1 )
            {
                // This is a proof by exhastion example of the first numbers that appears in the 3DBB tag.
                // Numbers 1 through 7 appears throughout the English version of Future Cop on the ps1, Mac, and Windows.
                // I could of used the greater and less thans, but this would not show that the numbers are every number that is 1, 2, 3, 4, 5, 6, 7
                // This might be a bit field of some kind, this is most likely something like the number of bounding boxes.
                assert( (bounding_box_per_frame == 1) | (bounding_box_per_frame == 2) | (bounding_box_per_frame == 3) | (bounding_box_per_frame == 4) | (bounding_box_per_frame == 5) | (bounding_box_per_frame == 6) | (bounding_box_per_frame == 7) );
                
                start_data += sizeof( uint32_t );
                
                bounding_boxes.reserve( Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian ) );
                
                bounding_box_frames = Utilities::DataHandler::read_u32( start_data, settings.is_opposite_endian ) / bounding_box_per_frame;
                
                start_data += sizeof( uint32_t );
                
                size_t BOUNDING_BOX_SIZE = 8 * sizeof( uint16_t );
                
                size_t bounding_boxes_amount = bounding_box_frames * bounding_box_per_frame;
                
                size_t test_tag_size = bounding_boxes_amount * BOUNDING_BOX_SIZE + 4 * sizeof( uint32_t );
                
                if( test_tag_size != tag_size )
                {
                    *settings.output_ref << "Mission::ObjResource::load() " << getIndexNumber() << std::endl;
                    *settings.output_ref << "Mission::ObjResource::load() bounding box per frame is " << bounding_box_per_frame << std::endl;
                    *settings.output_ref << "Mission::ObjResource::load() 3DBB frames is " << bounding_box_frames << std::endl;
                    *settings.output_ref << "Mission::ObjResource::load() The tag size should be " << test_tag_size;
                    *settings.output_ref << " instead it is " << tag_size << std::endl;
                }
                
                // Another proof by exhaustion. The all the remaining data consists
                // of the bounding_boxes_amount of data structs which are 0x10 in size.
                assert( test_tag_size == tag_size );
                
                for( size_t frame_index = 0; frame_index < bounding_box_frames; frame_index++ )
                {
                    for( size_t box_index = 0; box_index < bounding_box_per_frame; box_index++ )
                    {
                        bounding_boxes.push_back( BoundingBox3D() );
                        
                        // Fact Positive and negative: Assumption position x
                        bounding_boxes.back().x = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                        start_data += sizeof( int16_t );
                        
                        // Fact Positive and negative: Assumption position y
                        bounding_boxes.back().y = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                        start_data += sizeof( int16_t );
                        
                        // Fact Positive and negative: Assumption position z
                        bounding_boxes.back().z = Utilities::DataHandler::read_16( start_data, settings.is_opposite_endian );
                        start_data += sizeof( int16_t );
                        
                        // Fact [0, 4224]: Assumption length x
                        bounding_boxes.back().length_x = Utilities::DataHandler::read_u16( start_data, settings.is_opposite_endian );
                        assert( bounding_boxes.back().length_x >= 0 );
                        start_data += sizeof( uint16_t );
                        
                        // Fact [0, 1438]: Assumption length y
                        bounding_boxes.back().length_y = Utilities::DataHandler::read_u16( start_data, settings.is_opposite_endian );
                        assert( bounding_boxes.back().length_y >= 0 );
                        start_data += sizeof( uint16_t );
                        
                        // Fact [0, 3584]: Assumption length z
                        bounding_boxes.back().length_z = Utilities::DataHandler::read_u16( start_data, settings.is_opposite_endian );
                        assert(bounding_boxes.back().length_z >= 0 );
                        start_data += sizeof( uint16_t );
                        
                        // Fact [0, 4293]: Assumption rotation x
                        bounding_boxes.back().rotation_x = Utilities::DataHandler::read_u16( start_data, settings.is_opposite_endian );
                        assert( bounding_boxes.back().rotation_x >= 0 );
                        start_data += sizeof( uint16_t );
                        
                        // Fact [0, 4293]: Assumption rotation y
                        bounding_boxes.back().rotation_y = Utilities::DataHandler::read_u16( start_data, settings.is_opposite_endian );
                        assert( bounding_boxes.back().rotation_y >= 0 );
                        start_data += sizeof( uint16_t );
                    }
                }
            }
            else
            {
                *settings.output_ref << "Mission::ObjResource::load() 3DBB unexpected number at beginning! " << bounding_box_per_frame << std::endl;
            }
        }
        else
        {
            char identifier_word[5] = {'\0'};
            const auto IDENTIFIER_SIZE = (sizeof( identifier_word ) - 1) / sizeof(identifier_word[0]);

            for( unsigned int i = 0; i < IDENTIFIER_SIZE; i++ ) {
                identifier_word[ i ] = reinterpret_cast<char*>( &identifier )[ i ];
            }
            Utilities::DataHandler::swapBytes( reinterpret_cast< uint8_t* >( identifier_word ), 4 );

            if( settings.output_level >= 1 )
                *settings.output_ref << "Mission::ObjResource::load() " << identifier_word << " not recognized" << std::endl;
            
            assert( false );
        }

        *settings.output_ref << std::dec;

        if( file_is_not_valid ) {
            if( settings.output_level >= 1 )
                *settings.output_ref << "Mission::ObjResource::load() This is not a valid Obj file!" << std::endl;
            data = raw_data.data() + raw_data.size();
        }
        else
            data += tag_size;
    }
    
    // This assertion statement tells that there are only two options for
    // Animation either morphing or bone animation.
    assert( !(( bytes_per_frame_3DMI > 0 ) & ( vertex_anm_positions.size() > 0 )) );
    
    if( bytes_per_frame_3DMI > 0 )
    {
        this->bone_frames = this->bone_animation_data_size / (bytes_per_frame_3DMI / sizeof(int16_t));
        
        // This proves that the obj resource bone frame are roughly equvilent to the number of
        // bounding box frames.
        if( bounding_box_frames != bone_frames && bounding_box_frames + 1 != bone_frames )
        {
            *settings.output_ref << "Mission::ObjResource::load() " << getIndexNumber() << std::endl;
            *settings.output_ref << "Mission::ObjResource::load() bounding box per frame is " << bounding_box_per_frame << std::endl;
            *settings.output_ref << "Mission::ObjResource::load() 3DBB frames is " << bounding_box_frames << std::endl;
            *settings.output_ref << "Mission::ObjResource::load() 3DBB frames not equal to " << bone_frames << std::endl;
            
            assert( false );
        }
    }
    else
    if( vertex_anm_positions.size() > 0 )
    {
        // This proves that each model with morph animation has an equal number of
        // vertex frames as the bounding box frames.
        if( bounding_box_frames != vertex_anm_positions.size() + 1 )
        {
            *settings.output_ref << "Mission::ObjResource::load() " << getIndexNumber() << std::endl;
            *settings.output_ref << "Mission::ObjResource::load() bounding box per frame is " << bounding_box_per_frame << std::endl;
            *settings.output_ref << "Mission::ObjResource::load() 3DBB frames is " << bounding_box_frames << std::endl;
            *settings.output_ref << "Mission::ObjResource::load() 3DBB frames not equal to " << vertex_anm_positions.size() << std::endl;
            
            assert( false );
        }
    }
    else
    {
        // This statement proves that each model without animation only has one bounding_box frame.
        assert( bounding_box_frames == 1 );
    }

    return !file_is_not_valid;
}

Data::Mission::Resource * Data::Mission::ObjResource::duplicate() const {
    return new ObjResource( *this );
}

int Data::Mission::ObjResource::write( const char *const file_path, const std::vector<std::string> & arguments ) const {
    bool enable_export = true;
    int glTF_return = 0;

    for( auto arg = arguments.begin(); arg != arguments.end(); arg++ ) {
        if( (*arg).compare("--dry") == 0 )
            enable_export = false;
    }

    Utilities::ModelBuilder *model_output = createModel( &arguments );

    if( enable_export )
        glTF_return = model_output->write( std::string( file_path ) );

    delete model_output;

    return glTF_return;
}

Utilities::ModelBuilder * Data::Mission::ObjResource::createModel( const std::vector<std::string> * arguments ) const {
    Utilities::ModelBuilder *model_output = new Utilities::ModelBuilder();

    // This buffer will be used to store every triangle that the write function has.
    std::vector< FaceTriangle > triangle_buffer;
    std::vector<unsigned int> used_textures;
    std::vector<unsigned int> triangle_counts;
    bool is_specular = false;

    {
        triangle_buffer.reserve( face_trinagles.size() + face_quads.size() * 2 );

        for( auto i = face_trinagles.begin(); i != face_trinagles.end(); i++ ) {
            is_specular |= (*i).is_reflective;
            if( (*i).isWithinBounds( vertex_positions.size(), vertex_normals.size(), texture_quads.size(), texture_quads.data() ) )
            {
                triangle_buffer.push_back( (*i) );
            }
        }

        for( auto i = face_quads.begin(); i != face_quads.end(); i++ ) {
            is_specular |= (*i).is_reflective;
            if( (*i).firstTriangle().isWithinBounds( vertex_positions.size(), vertex_normals.size(), texture_quads.size(), texture_quads.data() ) )
            {
                triangle_buffer.push_back( (*i).firstTriangle() );
            }

            if( (*i).secondTriangle().isWithinBounds( vertex_positions.size(), vertex_normals.size(), texture_quads.size(), texture_quads.data() ) )
            {
                triangle_buffer.push_back( (*i).secondTriangle() );
            }
        }

        // Sort the triangle list.
        std::sort(triangle_buffer.begin(), triangle_buffer.end(), FaceTriangle() );

        int last_texture_quad_index = 0;

        // Get the list of the used textures
        for( auto i = triangle_buffer.begin(); i != triangle_buffer.end(); i++ ) {
            int index = (*i).texture_quad_ref->index;

            if( triangle_buffer.begin() == i || last_texture_quad_index != index) {
                triangle_counts.push_back( 0 );
                used_textures.push_back( index );
                last_texture_quad_index = index;
            }
            triangle_counts.back()++;
        }
    }
    

    unsigned int position_component_index = model_output->addVertexComponent( Utilities::ModelBuilder::POSITION_COMPONENT_NAME, Utilities::DataTypes::ComponentType::FLOAT, Utilities::DataTypes::Type::VEC3 );
    unsigned int normal_component_index = model_output->addVertexComponent( Utilities::ModelBuilder::NORMAL_COMPONENT_NAME, Utilities::DataTypes::ComponentType::FLOAT, Utilities::DataTypes::Type::VEC3 );
    unsigned int tex_coord_component_index = model_output->addVertexComponent( Utilities::ModelBuilder::TEX_COORD_0_COMPONENT_NAME, Utilities::DataTypes::ComponentType::UNSIGNED_BYTE, Utilities::DataTypes::Type::VEC2, true );
    unsigned int joints_0_component_index = -1;
    unsigned int weights_0_component_index = -1;
    unsigned int specular_component_index = -1; 
    
    // Specular is to exist if there is a single triangle or quad with a specular map.
    // if( is_specular )
        specular_component_index = model_output->addVertexComponent( "_Specular", Utilities::DataTypes::ComponentType::FLOAT, Utilities::DataTypes::Type::SCALAR );

    if( !bones.empty() ) {
        joints_0_component_index  = model_output->addVertexComponent( Utilities::ModelBuilder::JOINTS_INDEX_0_COMPONENT_NAME, Utilities::DataTypes::ComponentType::UNSIGNED_BYTE, Utilities::DataTypes::Type::VEC4, false );
        weights_0_component_index = model_output->addVertexComponent( Utilities::ModelBuilder::WEIGHTS_INDEX_0_COMPONENT_NAME, Utilities::DataTypes::ComponentType::UNSIGNED_BYTE, Utilities::DataTypes::Type::VEC4, true );

        model_output->allocateJoints( bones.size(), bone_frames );
        
        unsigned int *childern = new unsigned int [ max_bone_childern ]; // This can be a stack
        Utilities::DataTypes::Mat4 matrix_translate;
        Utilities::DataTypes::Mat4 matrix_rotation[3];
        Utilities::DataTypes::Mat4 matrix_combine;
        Utilities::DataTypes::Mat4 bone_transformation_matrix; // The combination of all the matrices.
        Utilities::DataTypes::Mat4 frame_matrix;
        unsigned int opcode_decode = 0;
        
        const float ANGLE_UNITS_TO_RADIANS = M_PI / 2048.0;

        for( unsigned int bone_index = 0; bone_index < bones.size(); bone_index++ ) {
            auto current_bone = bones.begin() + bone_index;
            
            childern[ (*current_bone).parent_amount - 1 ] = bone_index;
            
            for( unsigned int frame = 0; frame < bone_frames; frame++ )
            {
                auto frame_position = (*current_bone).position;
                auto frame_rotation = (*current_bone).rotation;
                
                if( !(*current_bone).opcode.position.x_const )
                    frame_position.x = bone_animation_data[ (*current_bone).position.x + frame ];
                if( !(*current_bone).opcode.position.y_const )
                    frame_position.y = bone_animation_data[ (*current_bone).position.y + frame ];
                if( !(*current_bone).opcode.position.z_const )
                    frame_position.z = bone_animation_data[ (*current_bone).position.z + frame ];
                if( !(*current_bone).opcode.rotation.x_const )
                    frame_rotation.x = bone_animation_data[ (*current_bone).rotation.x + frame ];
                if( !(*current_bone).opcode.rotation.y_const )
                    frame_rotation.y = bone_animation_data[ (*current_bone).rotation.y + frame ];
                if( !(*current_bone).opcode.rotation.z_const )
                    frame_rotation.z = bone_animation_data[ (*current_bone).rotation.z + frame ];
                
                Utilities::Math::setTranslation( matrix_translate, 
                Utilities::DataTypes::Vec3(
                   -static_cast<float>( frame_position.x ) * INTEGER_FACTOR,
                    static_cast<float>( frame_position.y ) * INTEGER_FACTOR,
                    static_cast<float>( frame_position.z ) * INTEGER_FACTOR ) );
                
                Utilities::Math::setRotation( matrix_rotation[0], Utilities::DataTypes::Vec3( 0, 1, 0 ), -static_cast<float>( frame_rotation.x ) * ANGLE_UNITS_TO_RADIANS );
                Utilities::Math::setRotation( matrix_rotation[1], Utilities::DataTypes::Vec3( 1, 0, 0 ),  static_cast<float>( frame_rotation.y ) * ANGLE_UNITS_TO_RADIANS );
                Utilities::Math::multiply( matrix_rotation[2], matrix_rotation[0], matrix_rotation[1] );
                Utilities::Math::setRotation( matrix_rotation[1], Utilities::DataTypes::Vec3( 0, 0, 1 ), -static_cast<float>( frame_rotation.z ) * ANGLE_UNITS_TO_RADIANS );
                Utilities::Math::multiply( matrix_rotation[0], matrix_rotation[1], matrix_rotation[2] );
                
                Utilities::Math::multiply( frame_matrix, matrix_translate, matrix_rotation[0] );
                
                bone_transformation_matrix.setIdentity();
                if( (*current_bone).parent_amount > 1 ) {
                    bone_transformation_matrix = model_output->getJointFrame( frame )[ childern[ (*current_bone).parent_amount - 2 ] ];
                }
                
                Utilities::Math::multiply( model_output->getJointFrame( frame )[ bone_index ], bone_transformation_matrix, frame_matrix );
            }
        }
        
        delete [] childern;
    }
    unsigned int position_morph_component_index = -1;
    unsigned int normal_morph_component_index = -1;

    if( vertex_anm_positions.size() > 0 ) {
        position_morph_component_index = model_output->setVertexComponentMorph( position_component_index );
        normal_morph_component_index = model_output->setVertexComponentMorph( normal_component_index );
    }

    // TODO add texture settings.
    // Set the sampler for all the textures ever referenced.
    // model_output->setSampler( GLTFWriter::FilterType::NEAREST, GLTFWriter::FilterType::NEAREST_MIPMAP_LINEAR,
        // GLTFWriter::WrapType::CLAMP_TO_EDGE, GLTFWriter::WrapType::CLAMP_TO_EDGE );

    // Setup the vertex components now that every field had been entered.
    model_output->setupVertexComponents( vertex_anm_positions.size() );

    model_output->allocateVertices( triangle_buffer.size() * 3 );

    if( used_textures.size() == 0 )
        model_output->setMaterial( -1 );
    /*
    for( unsigned int i = 0; i < used_textures.size(); i++ )
    {
        image_path = std::to_string(used_textures.at(i)) + ".png";

        glTF_output->addTexturePath( image_path );
    }
    */

    Utilities::DataTypes::Vec3 position;
    Utilities::DataTypes::Vec3 new_position;
    Utilities::DataTypes::Vec3 normal;
    Utilities::DataTypes::Vec3 new_normal;
    Utilities::DataTypes::Scalar specular;
    Utilities::DataTypes::Vec2UByte coords[3];
    Utilities::DataTypes::Vec4UByte weights;
    Utilities::DataTypes::Vec4UByte joints;

    // Future Cop only uses one joint, so it only needs one weight.
    weights.x = 0xFF;
    weights.y = weights.z = weights.w = 0;

    // The joint needs to be set to zero.
    joints.x = joints.y = joints.z = joints.w = 0;

    auto triangle = triangle_buffer.begin();

    for( unsigned int mat = 0; mat < triangle_counts.size(); mat++ )
    {
        model_output->setMaterial( used_textures.at(mat) );

        for( unsigned int i = 0; i < triangle_counts.at(mat); i++ )
        {
            triangleToCoords( (*triangle), coords );
            
            if( (*triangle).is_reflective )
                specular.x = 1.0f;
            else
                specular.x = 0.0f;

            model_output->startVertex();

            handlePositions( position, vertex_positions.data(), (*triangle).v0 );
            handleNormals( normal, vertex_normals.data(), (*triangle).n0 );
            model_output->setVertexData( position_component_index, Utilities::DataTypes::Vec3Type( position ) );
            model_output->setVertexData( normal_component_index, Utilities::DataTypes::Vec3Type( normal ) );
            model_output->setVertexData( tex_coord_component_index, Utilities::DataTypes::Vec2UByteType( coords[0] ) );
            if( is_specular )
            {
                model_output->setVertexData( specular_component_index, Utilities::DataTypes::ScalarType( specular ) );
            }
            for( unsigned int morph_frames = 0; morph_frames < vertex_anm_positions.size(); morph_frames++ )
            {
                handlePositions( new_position, vertex_anm_positions.at(morph_frames).data(), (*triangle).v0 );
                model_output->addMorphVertexData( position_morph_component_index, morph_frames, Utilities::DataTypes::Vec3Type( position ), Utilities::DataTypes::Vec3Type( new_position ) );

                handleNormals( new_normal, vertex_anm_normals.at(morph_frames).data(), (*triangle).n0 );
                model_output->addMorphVertexData( normal_morph_component_index, morph_frames, Utilities::DataTypes::Vec3Type( normal ), Utilities::DataTypes::Vec3Type( new_normal ) );
            }
            if( !bones.empty() ) {
                for( auto bone = bones.begin(); bone != bones.end(); bone++) {
                    if( (*bone).vertex_start > (*triangle).v0 ) {
                        break;
                    }
                    else
                    if( (*bone).vertex_start + (*bone).vertex_stride > (*triangle).v0 )
                    {
                        joints.x = bone - bones.begin();
                        model_output->setVertexData( joints_0_component_index, Utilities::DataTypes::Vec4UByteType( joints ) );
                    }
                }

                model_output->setVertexData( weights_0_component_index, Utilities::DataTypes::Vec4UByteType( weights ) );
            }

            model_output->startVertex();

            handlePositions( position, vertex_positions.data(), (*triangle).v1 );
            handleNormals( normal, vertex_normals.data(), (*triangle).n1 );
            model_output->setVertexData( position_component_index, Utilities::DataTypes::Vec3Type( position ) );
            model_output->setVertexData( normal_component_index, Utilities::DataTypes::Vec3Type( normal ) );
            model_output->setVertexData( tex_coord_component_index, Utilities::DataTypes::Vec2UByteType( coords[1] ) );
            if( is_specular )
            {
                model_output->setVertexData( specular_component_index, Utilities::DataTypes::ScalarType( specular ) );
            }
            for( unsigned int morph_frames = 0; morph_frames < vertex_anm_positions.size(); morph_frames++ )
            {
                handlePositions( new_position, vertex_anm_positions.at(morph_frames).data(), (*triangle).v1 );
                model_output->addMorphVertexData( position_morph_component_index, morph_frames, Utilities::DataTypes::Vec3Type( position ), Utilities::DataTypes::Vec3Type( new_position ) );

                handleNormals( new_normal, vertex_anm_normals.at(morph_frames).data(), (*triangle).n1 );
                model_output->addMorphVertexData( normal_morph_component_index, morph_frames, Utilities::DataTypes::Vec3Type( normal ), Utilities::DataTypes::Vec3Type( new_normal ) );
            }
            if( !bones.empty() ) {
                for( auto bone = bones.begin(); bone != bones.end(); bone++) {
                    if( (*bone).vertex_start > (*triangle).v1 ) {
                        break;
                    }
                    else
                    if( (*bone).vertex_start + (*bone).vertex_stride > (*triangle).v1 )
                    {
                        joints.x = bone - bones.begin();
                        model_output->setVertexData( joints_0_component_index, Utilities::DataTypes::Vec4UByteType( joints ) );
                    }
                }

                model_output->setVertexData( weights_0_component_index, Utilities::DataTypes::Vec4UByteType( weights ) );
            }

            model_output->startVertex();

            handlePositions( position, vertex_positions.data(), (*triangle).v2 );
            handleNormals( normal, vertex_normals.data(), (*triangle).n2 );
            model_output->setVertexData( position_component_index, Utilities::DataTypes::Vec3Type( position ) );
            model_output->setVertexData( normal_component_index, Utilities::DataTypes::Vec3Type( normal ) );
            model_output->setVertexData( tex_coord_component_index, Utilities::DataTypes::Vec2UByteType( coords[2] ) );
            if( is_specular )
            {
                model_output->setVertexData( specular_component_index, Utilities::DataTypes::ScalarType( specular ) );
            }
            for( unsigned int morph_frames = 0; morph_frames < vertex_anm_positions.size(); morph_frames++ )
            {
                handlePositions( new_position, vertex_anm_positions.at(morph_frames).data(), (*triangle).v2 );
                model_output->addMorphVertexData( position_morph_component_index, morph_frames, Utilities::DataTypes::Vec3Type( position ), Utilities::DataTypes::Vec3Type( new_position ) );

                handleNormals( new_normal, vertex_anm_normals.at(morph_frames).data(), (*triangle).n2 );
                model_output->addMorphVertexData( normal_morph_component_index, morph_frames, Utilities::DataTypes::Vec3Type( normal ), Utilities::DataTypes::Vec3Type( new_normal ) );
            }
            if( !bones.empty() ) {
                for( auto bone = bones.begin(); bone != bones.end(); bone++) {
                    if( (*bone).vertex_start > (*triangle).v2 ) {
                        break;
                    }
                    else
                    if( (*bone).vertex_start + (*bone).vertex_stride > (*triangle).v2 )
                    {
                        joints.x = bone - bones.begin();
                        model_output->setVertexData( joints_0_component_index, Utilities::DataTypes::Vec4UByteType( joints ) );
                    }
                }

                model_output->setVertexData( weights_0_component_index, Utilities::DataTypes::Vec4UByteType( weights ) );
            }

            triangle++;
        }
    }
    return model_output;
}

unsigned int Data::Mission::ObjResource::getOpcodeBytesPerFrame( Data::Mission::ObjResource::Bone::Opcode opcode ) {
    unsigned int number_of_16bit_numbers = 0;

    // I hope the compiler optimizes this.
    if( !opcode.position.x_const )
        number_of_16bit_numbers++;
    if( !opcode.position.y_const )
        number_of_16bit_numbers++;
    if( !opcode.position.z_const )
        number_of_16bit_numbers++;
    
    if( !opcode.rotation.x_const )
        number_of_16bit_numbers++;
    if( !opcode.rotation.y_const )
        number_of_16bit_numbers++;
    if( !opcode.rotation.z_const )
        number_of_16bit_numbers++;
    
    return 2 * number_of_16bit_numbers;
}

std::vector<Data::Mission::ObjResource*> Data::Mission::ObjResource::getVector( Data::Mission::IFF &mission_file ) {
    std::vector<Resource*> to_copy = mission_file.getResources( Data::Mission::ObjResource::IDENTIFIER_TAG );

    std::vector<ObjResource*> copy;

    copy.reserve( to_copy.size() );

    for( auto it = to_copy.begin(); it != to_copy.end(); it++ )
        copy.push_back( dynamic_cast<ObjResource*>( (*it) ) );

    return copy;
}

const std::vector<Data::Mission::ObjResource*> Data::Mission::ObjResource::getVector( const Data::Mission::IFF &mission_file ) {
    return Data::Mission::ObjResource::getVector( const_cast< IFF& >( mission_file ) );
}
