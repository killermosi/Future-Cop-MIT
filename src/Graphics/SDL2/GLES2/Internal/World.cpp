#include "World.h"
#include "../../../../Data/Mission/IFF.h"
#include "../../../../Utilities/Collision/GJK.h"
#include "GLES2.h"

#include <glm/ext/matrix_transform.hpp>
#include "SDL.h"
#include <iostream>

void Graphics::SDL2::GLES2::Internal::World::MeshDraw::Animation::addTriangles( const std::vector<DynamicTriangleDraw::Triangle> &triangles, DynamicTriangleDraw::DrawCommand &triangles_draw ) const {
    const glm::vec4 frag_inv = glm::vec4( 1, 1, 1, 1 );
    glm::vec3 inverse_color;
    double unused;

    DynamicTriangleDraw::Triangle *draw_triangles_r;

    size_t number_of_triangles = triangles_draw.getTriangles( triangles.size(), &draw_triangles_r );

    for( size_t i = 0; i < number_of_triangles; i++ ) {
        draw_triangles_r[ i ] = triangles[ i ].addTriangle( this->camera_position, transform );

        const auto info = mesh_draw_r->transparent_triangle_info[ i ];

        if( selected_tile == info.type ) {
            for( unsigned t = 0; t < 3; t++ ) {
                glm::vec4 inverse_color = frag_inv - draw_triangles_r[ i ].vertices[ t ].color;
                draw_triangles_r[ i ].vertices[ t ].color = 2.0f * ( (1.0f - glow_time) * draw_triangles_r[ i ].vertices[ t ].color + 2.0f * glow_time * inverse_color );
                draw_triangles_r[ i ].vertices[ t ].color.w = 1;
            }
        }

        if( info.animated ) {
            for( unsigned t = 0; t < 3; t++ ) {
                draw_triangles_r[ i ].vertices[ t ].coordinate += mesh_draw_r->displacement_uv_destination;

                draw_triangles_r[ i ].vertices[ t ].coordinate.x = modf( draw_triangles_r[ i ].vertices[ t ].coordinate.x, &unused );
                draw_triangles_r[ i ].vertices[ t ].coordinate.y = modf( draw_triangles_r[ i ].vertices[ t ].coordinate.y, &unused );
            }
        }
    }
}

const GLchar* Graphics::SDL2::GLES2::Internal::World::default_vertex_shader =
    // Vertex shader uniforms
    "uniform mat4  Transform;\n" // projection * view * model.
    "uniform vec2  AnimatedUVDestination;\n"
    "uniform float GlowTime;\n"
    "uniform float SelectedTile;\n"
    "uniform vec2  AnimatedUVFrames[ 16 * 4 ];\n"

    "const vec3 frag_inv = vec3(1,1,1);\n"

    "void main() {\n"
    "   float SELECT_SPECIFIER   = _TILE_TYPE.x;\n"
    "   float DISPLACEMENT       = _TILE_TYPE.y;\n"
    "   highp int FRAME_BY_FRAME = int( _TILE_TYPE.z );\n"

    "   vec3 inverse_color = frag_inv - COLOR_0;\n"
    "   float flashing = GlowTime * float(SelectedTile > SELECT_SPECIFIER - 0.5 && SelectedTile < SELECT_SPECIFIER + 0.5);\n"
    "   vertex_colors = (1.0 - flashing) * COLOR_0 + 2.0 * flashing * inverse_color;\n"
    "   vec2 tex_coord_pos = TEXCOORD_0 * float( FRAME_BY_FRAME == 0 );\n"
    "   tex_coord_pos += AnimatedUVFrames[ clamp( FRAME_BY_FRAME - 1, 0, 16 * 4 ) ] * float( FRAME_BY_FRAME != 0 );\n"
    "   texture_coord_1 = fract( tex_coord_pos + AnimatedUVDestination * DISPLACEMENT );\n"
    "   gl_Position = Transform * vec4(POSITION.xyz, 1.0);\n"
    "}\n";
const GLchar* Graphics::SDL2::GLES2::Internal::World::default_fragment_shader =
    "uniform sampler2D Texture;\n"

    "void main() {\n"
    "   vec4 frag_color = texture2D(Texture, texture_coord_1);\n"
    "   if( frag_color.a < 0.015625 )\n"
    "       discard;\n"
    "   vec3 normal_color = vertex_colors * frag_color.rgb * 2.0;\n"
    "   gl_FragColor = vec4( normal_color, 1.0 );\n"
    "}\n";

Graphics::SDL2::GLES2::Internal::World::World() {
    this->glow_time = 0;
    this->current_selected_tile = 112;
    this->selected_tile = this->current_selected_tile + 1;

    attributes.push_back( Shader::Attribute( Shader::Type::MEDIUM, "vec4 " + Utilities::ModelBuilder::POSITION_COMPONENT_NAME ) );
    attributes.push_back( Shader::Attribute( Shader::Type::LOW,    "vec2 " + Utilities::ModelBuilder::TEX_COORD_0_COMPONENT_NAME ) );
    attributes.push_back( Shader::Attribute( Shader::Type::LOW,    "vec3 " + Utilities::ModelBuilder::COLORS_0_COMPONENT_NAME ) );
    attributes.push_back( Shader::Attribute( Shader::Type::MEDIUM, "vec3 " + Data::Mission::TilResource::TILE_TYPE_COMPONENT_NAME ) );

    varyings.push_back( Shader::Varying( Shader::Type::LOW, "vec3 vertex_colors" ) );
    varyings.push_back( Shader::Varying( Shader::Type::LOW, "vec2 texture_coord_1" ) );
}

Graphics::SDL2::GLES2::Internal::World::~World() {
    for( auto i = tiles.begin(); i != tiles.end(); i++ ) {
        delete (*i).mesh_p;
    }
}

const GLchar* Graphics::SDL2::GLES2::Internal::World::getDefaultVertexShader() {
    return default_vertex_shader;
}

const GLchar* Graphics::SDL2::GLES2::Internal::World::getDefaultFragmentShader() {
    return default_fragment_shader;
}

void Graphics::SDL2::GLES2::Internal::World::setVertexShader( const GLchar *const shader_source ) {
    vertex_shader.setShader( Shader::TYPE::VERTEX, shader_source, attributes, varyings );
}

int Graphics::SDL2::GLES2::Internal::World::loadVertexShader( const char *const file_path ) {
    return vertex_shader.loadShader( Shader::TYPE::VERTEX, file_path );
}

void Graphics::SDL2::GLES2::Internal::World::setFragmentShader( const GLchar *const shader_source ) {
    fragment_shader.setShader( Shader::TYPE::FRAGMENT, shader_source, {}, varyings );
}

int Graphics::SDL2::GLES2::Internal::World::loadFragmentShader( const char *const file_path ) {
    return fragment_shader.loadShader( Shader::TYPE::FRAGMENT, file_path );
}

int Graphics::SDL2::GLES2::Internal::World::compilieProgram() {
    bool link_success     = true;
    bool uniform_failed   = false;
    bool attribute_failed = false;
    
    // The two shaders should be allocated first.
    if( vertex_shader.getType() == Shader::TYPE::VERTEX && fragment_shader.getType() == Shader::TYPE::FRAGMENT ) {
        // Allocate the opengl program for the map.
        program.allocate();

        // Give the program these two shaders.
        program.setVertexShader( &vertex_shader );
        program.setFragmentShader( &fragment_shader );

        // Link the shader
        if( !program.link() )
            link_success = false;
        else
        {
            // Setup the uniforms for the map.
            texture_uniform_id       = program.getUniform( "Texture",          &std::cout, &uniform_failed );
            matrix_uniform_id        = program.getUniform( "Transform",        &std::cout, &uniform_failed );
            displacement_uv_destination_id = program.getUniform( "AnimatedUVDestination", &std::cout, &uniform_failed );
            frame_uv_id              = program.getUniform( "AnimatedUVFrames", &std::cout, &uniform_failed );
            glow_time_uniform_id     = program.getUniform( "GlowTime",         &std::cout, &uniform_failed );
            selected_tile_uniform_id = program.getUniform( "SelectedTile",     &std::cout, &uniform_failed );
            
            attribute_failed |= !program.isAttribute( Utilities::ModelBuilder::POSITION_COMPONENT_NAME,     &std::cout );
            attribute_failed |= !program.isAttribute( Utilities::ModelBuilder::COLORS_0_COMPONENT_NAME,     &std::cout );
            attribute_failed |= !program.isAttribute( Utilities::ModelBuilder::TEX_COORD_0_COMPONENT_NAME,  &std::cout );
            attribute_failed |= !program.isAttribute( Data::Mission::TilResource::TILE_TYPE_COMPONENT_NAME, &std::cout );
        }
        
        if( !link_success || uniform_failed || attribute_failed )
        {
            std::cout << "World Draw Error\n";
            std::cout << program.getInfoLog();
            std::cout << "\nVertex shader log\n";
            std::cout << vertex_shader.getInfoLog();
            std::cout << "\nFragment shader log\n";
            std::cout << fragment_shader.getInfoLog() << std::endl;
            
            return 1;
        }
        else
            return -1;
    }
    else
    {
        return 0; // Not every shader was loaded.
    }
}

void Graphics::SDL2::GLES2::Internal::World::setWorld( const Data::Mission::PTCResource &pointer_tile_cluster, const std::vector<Data::Mission::TilResource*> resources_til,  const std::map<uint32_t, Internal::Texture2D*>& textures ) {
    tiles.resize( resources_til.size() );

    // Set up the primary tiles. O(n)
    for( auto i = tiles.begin(); i != tiles.end(); i++ ) {
        const Data::Mission::TilResource *data = resources_til[ i - tiles.begin() ];
        auto model_p = data->createCulledModel();

        assert( model_p != nullptr );

        (*i).mesh_p = new Graphics::SDL2::GLES2::Internal::Mesh( &program );
        (*i).til_resource_r = data;
        (*i).change_rate = -1.0;
        (*i).current = 0.0;

        (*i).displacement_uv_factor = data->getUVAnimation();
        (*i).displacement_uv_destination = glm::vec2( 0, 0 );
        (*i).displacement_uv_time = glm::vec2( 0, 0 );

        const std::vector<Data::Mission::TilResource::InfoSCTA> &scta_infos = (*i).til_resource_r->getInfoSCTA();
        const std::vector<glm::u8vec2> &uv_frames = (*i).til_resource_r->getSCTATextureCords();

        (*i).frame_uv_times.resize( scta_infos.size(), 0 );
        (*i).current_frame_uvs.resize( scta_infos.size() * 4 );

        for( unsigned info_index = 0; info_index < scta_infos.size(); info_index++ ) {
            const Data::Mission::TilResource::InfoSCTA &info = scta_infos[ info_index ];

            if( info.isMemorySafe() ) {
                const auto factor = glm::vec2( 1. / 256., 1. / 256. );
                const unsigned frame_index = 0 * 4;

                for( unsigned a = 0; a < 4; a++ ) {
                    (*i).current_frame_uvs[ info_index * 4 + a ] = glm::vec2( uv_frames[ info.animated_uv_offset / 2 + a + frame_index ].x, uv_frames[ info.animated_uv_offset / 2 + a + frame_index].y ) * factor;
                }
            }
        }

        (*i).mesh_p->setup( *model_p, textures );

        Utilities::ModelBuilder::TextureMaterial material;
        GLsizei transparent_count = 0;

        for( unsigned int a = 0; a < model_p->getNumMaterials(); a++ ) {
            model_p->getMaterial( a, material );
            GLsizei opeque_count = std::min( material.count, material.opeque_count );
            transparent_count += material.count - material.opeque_count;
        }

        (*i).transparent_triangles.reserve(     transparent_count );
        (*i).transparent_triangle_info.reserve( transparent_count );

        GLsizei material_count = 0;

        unsigned   position_compenent_index = model_p->getNumVertexComponents();
        unsigned      color_compenent_index = position_compenent_index;
        unsigned coordinate_compenent_index = position_compenent_index;
        unsigned  tile_type_compenent_index = position_compenent_index;

        Utilities::ModelBuilder::VertexComponent element("EMPTY");
        for( unsigned i = 0; model_p->getVertexComponent( i, element ); i++ ) {
            auto name = element.getName();

            if( name == Utilities::ModelBuilder::POSITION_COMPONENT_NAME )
                position_compenent_index = i;
            if( name == Utilities::ModelBuilder::COLORS_0_COMPONENT_NAME )
                color_compenent_index = i;
            if( name == Utilities::ModelBuilder::TEX_COORD_0_COMPONENT_NAME )
                coordinate_compenent_index = i;
            if( name == Data::Mission::TilResource::TILE_TYPE_COMPONENT_NAME )
                tile_type_compenent_index = i;
        }

        for( unsigned int a = 0; a < model_p->getNumMaterials(); a++ ) {
            model_p->getMaterial( a, material );

            uint32_t cbmp_id;

            if( textures.find( material.cbmp_resource_id ) != textures.end() )
                cbmp_id = material.cbmp_resource_id;
            else if( !textures.empty() ) {
                cbmp_id = textures.begin()->first;
            }
            else
                cbmp_id = 0;

            GLsizei opeque_count = std::min( material.count, material.opeque_count );

            glm::vec4   positions = glm::vec4(0, 0, 0, 1);
            glm::vec4      colors = glm::vec4(0.5, 0.5, 0.5, 0.5); // Just in case if the mesh does not have vertex color information.
            glm::vec4 coordinates = glm::vec4(0, 0, 0, 1);
            glm::vec4   tile_type = glm::vec4(0, 0, 0, 1);

            const unsigned vertex_per_triangle = 3;

            for( GLsizei m = opeque_count; m < material.count; m += vertex_per_triangle ) {
                DynamicTriangleDraw::Triangle triangle;

                for( unsigned t = 0; t < 3; t++ ) {
                    model_p->getTransformation(   positions,   position_compenent_index, material_count + m + t );
                    model_p->getTransformation(      colors,      color_compenent_index, material_count + m + t );
                    model_p->getTransformation( coordinates, coordinate_compenent_index, material_count + m + t );
                    model_p->getTransformation(   tile_type,  tile_type_compenent_index, material_count + m + t );

                    triangle.vertices[t].position = { positions.x, positions.y, positions.z };
                    triangle.vertices[t].color = 2.0f * colors;
                    triangle.vertices[t].color.w = 1;
                    triangle.vertices[t].coordinate = coordinates;

                    if( t == 0 ) {
                        MeshDraw::Info info;

                        info.type     = tile_type.x;
                        info.animated = tile_type.y;

                        (*i).transparent_triangle_info.push_back( info );
                    }
                }

                triangle.setup( cbmp_id, glm::vec3(0, 0, 0) );

                (*i).transparent_triangles.push_back( triangle );
            }

            material_count += material.count;
        }

        delete model_p;
    }

    {
        std::vector<uint32_t> temp_amounts( tiles.size(), 0 );

        // Set the position amounts. O(x*y) or O(n^2).
        for( unsigned int x = 0; x < pointer_tile_cluster.getWidth(); x++ )
        {
            for( unsigned int y = 0; y < pointer_tile_cluster.getHeight(); y++ )
            {
                auto pointer = pointer_tile_cluster.getTile(x, y);
                if( pointer != nullptr )
                    temp_amounts[ pointer->getIndexNumber() ]++;
            }
        }

        // Allocate them. O(n)
        for( auto i = tiles.begin(); i != tiles.end(); i++ ) {
            (*i).sections.reserve( temp_amounts[ i - tiles.begin() ] );
        }
    }

    // Finally setup the map. O(x*y) or O(n^2).
    for( unsigned int x = 0; x < pointer_tile_cluster.getWidth(); x++  )
    {
        for( unsigned int y = 0; y < pointer_tile_cluster.getHeight(); y++ )
        {
            auto pointer = pointer_tile_cluster.getTile( x, y );
            if( pointer != nullptr )
            {
                unsigned int index = pointer->getIndexNumber();
                
                tiles.at(index).sections.push_back( MeshDraw::Section() );

                tiles.at(index).sections.back().position.x = x - 1;
                tiles.at(index).sections.back().position.y = y;
            }
        }
    }
    // This algorithm is 2*O(n^2) + 3*O(n) = O(n^2).
}

bool Graphics::SDL2::GLES2::Internal::World::updateCulling( Graphics::SDL2::GLES2::Camera &camera ) const {
    if( camera.culling_info.getWidth() * camera.culling_info.getHeight() == 0 ) {
        return false;
    }

    auto position = camera.getPosition();
    auto projection = camera.getProjection3DShape();

    std::vector<glm::vec3> section_data( 8, glm::vec3() );
    const glm::vec3 MIN = glm::vec3(0, Data::Mission::TilResource::MAX_HEIGHT, 0);
    const glm::vec3 MAX = glm::vec3( Data::Mission::TilResource::AMOUNT_OF_TILES, Data::Mission::TilResource::MIN_HEIGHT, Data::Mission::TilResource::AMOUNT_OF_TILES );

    for( auto m : tiles ) {
        for( auto s : m.sections ) {
            glm::vec3 adjusted_position( s.position.x, 0, s.position.y );
            adjusted_position *= Data::Mission::TilResource::AMOUNT_OF_TILES;

            glm::vec3 min = adjusted_position + MIN;
            glm::vec3 max = adjusted_position + MAX;

            section_data[0] = glm::vec3( min.x, min.y, min.z );
            section_data[1] = glm::vec3( max.x, min.y, min.z );
            section_data[2] = glm::vec3( min.x, max.y, min.z );
            section_data[3] = glm::vec3( max.x, max.y, min.z );
            section_data[4] = glm::vec3( min.x, min.y, max.z );
            section_data[5] = glm::vec3( max.x, min.y, max.z );
            section_data[6] = glm::vec3( min.x, max.y, max.z );
            section_data[7] = glm::vec3( max.x, max.y, max.z );

            Utilities::Collision::GJKPolyhedron section_shape( section_data );

            if( Utilities::Collision::GJK::hasCollision( projection, section_shape ) == Utilities::Collision::GJK::NO_COLLISION ) {
                camera.culling_info.setValue( s.position.x, s.position.y, -1.0f );
            }
            else {
                const auto distance_2 = (adjusted_position.x - position.x) * (adjusted_position.x - position.x) + (adjusted_position.z - position.z) * (adjusted_position.z - position.z);

                camera.culling_info.setValue( s.position.x, s.position.y, distance_2 );
            }
        }
    }

    return true;
}

void Graphics::SDL2::GLES2::Internal::World::draw( Graphics::SDL2::GLES2::Camera &camera ) {
    glm::mat4 projection_view, final_position, position_mat;
    
    // Use the map shader for the 3D map or the world.
    program.use();

    camera.getProjectionView3D( projection_view );

    GLfloat filtered_glow_time;
    
    if( this->glow_time > 1.0f )
        filtered_glow_time = 2.0f - this->glow_time;
    else
    if( this->glow_time < 0.0f )
        filtered_glow_time = 0;
    else
        filtered_glow_time = this->glow_time;

    glUniform1f( glow_time_uniform_id, filtered_glow_time );
    
    if( this->selected_tile != this->current_selected_tile ) {
        this->current_selected_tile = this->selected_tile;
        glUniform1f( selected_tile_uniform_id, this->selected_tile );
    }

    const float TILE_SPAN = 0.5;

    const float squared_distance_culling = 64.0 * 64.0; // This is squared because square rooting the distance on the triangles is slower.

    const bool is_culling_there = camera.culling_info.getWidth() * camera.culling_info.getHeight() != 0;

    MeshDraw::Animation dynamic;
    dynamic.camera_position = camera.getPosition();
    dynamic.selected_tile = this->selected_tile;
    dynamic.glow_time = filtered_glow_time;

    for( auto i = tiles.begin(); i != tiles.end(); i++ ) {
        glUniform2f( displacement_uv_destination_id, (*i).displacement_uv_destination.x, (*i).displacement_uv_destination.y );

        if( (*i).current_frame_uvs.size() != 0 )
            glUniform2fv( frame_uv_id, (*i).current_frame_uvs.size(), reinterpret_cast<float*>((*i).current_frame_uvs.data()) );

        if( (*i).current >= 0.0 )
        for( unsigned int d = 0; d < (*i).sections.size(); d++ ) {

            auto section = (*i).sections[d];

            if( !is_culling_there || camera.culling_info.getValue( section.position.x, section.position.y ) >= -0.5 ) {
                const glm::vec3 position = glm::vec3(
                    ((section.position.x * Data::Mission::TilResource::AMOUNT_OF_TILES + Data::Mission::TilResource::SPAN_OF_TIL) + TILE_SPAN),
                    0,
                    ((section.position.y * Data::Mission::TilResource::AMOUNT_OF_TILES + Data::Mission::TilResource::SPAN_OF_TIL) + TILE_SPAN) );

                final_position = glm::translate( projection_view, position );

                position_mat = glm::translate( glm::mat4(1), position );

                // We can now send the matrix to the program.
                glUniformMatrix4fv( matrix_uniform_id, 1, GL_FALSE, reinterpret_cast<const GLfloat*>( &final_position[0][0] ) );

                if( is_culling_there && camera.culling_info.getValue( section.position.x, section.position.y ) < squared_distance_culling ) {
                    (*i).mesh_p->drawOpaque( 0, texture_uniform_id );

                    dynamic.transform = position_mat;
                    dynamic.mesh_draw_r = &(*i);

                    dynamic.addTriangles( (*i).transparent_triangles, camera.transparent_triangles );
                }
                else
                    (*i).mesh_p->draw( 0, texture_uniform_id );
            }
        }
    }
}


void Graphics::SDL2::GLES2::Internal::World::advanceTime( float seconds_passed ) {
    for( auto i = tiles.begin(); i != tiles.end(); i++ ) {
        if( (*i).change_rate > 0.0 )
        {
            (*i).current += seconds_passed;
            if( (*i).current > (*i).change_rate )
                (*i).current -= (*i).change_rate * 2.0;
        }

        const auto frame_time_inverse = (1./5.95); // 5.95 is the amount of secounds that one unit of time.
        const auto pixel_amount = 27.0;
        const auto image_ratio  = 1.0 / 256.0; // A single Cbmp texture resource has the resolution of 256 pixels.
        const auto uv_destination = image_ratio * pixel_amount;

        (*i).displacement_uv_time.x += seconds_passed * frame_time_inverse * (*i).displacement_uv_factor.x;

        if( (*i).displacement_uv_time.x > 1.0f )
            (*i).displacement_uv_time.x -= 1.0f;

        (*i).displacement_uv_time.y += seconds_passed * frame_time_inverse * (*i).displacement_uv_factor.y;

        if( (*i).displacement_uv_time.y  > 1.0f )
            (*i).displacement_uv_time.y -= 1.0f;

        (*i).displacement_uv_destination = glm::vec2( uv_destination, uv_destination ) * (*i).displacement_uv_time;

        const std::vector<Data::Mission::TilResource::InfoSCTA> &scta_infos = (*i).til_resource_r->getInfoSCTA();
        const std::vector<glm::u8vec2> &uv_frames = (*i).til_resource_r->getSCTATextureCords();

        float last_time;
        const auto factor = glm::vec2( 1. / 256., 1. / 256. );

        for( unsigned info_index = 0; info_index < scta_infos.size(); info_index++ ) {
            const Data::Mission::TilResource::InfoSCTA &info = scta_infos[ info_index ];

            if( info.isMemorySafe() ) {

                last_time = (*i).frame_uv_times[ info_index ];

                (*i).frame_uv_times[ info_index ] += seconds_passed * info.getDurationToSeconds();

                if( (*i).frame_uv_times[ info_index ] >= info.getFrameCount() )
                    (*i).frame_uv_times[ info_index ] -= info.getFrameCount();

                if( int(last_time) != int( (*i).frame_uv_times[ info_index ] ) ) {
                    const unsigned frame_index = unsigned( (*i).frame_uv_times[ info_index ] ) * 4;

                    for( unsigned a = 0; a < 4; a++ ) {
                        (*i).current_frame_uvs[ info_index * 4 + a ] = glm::vec2( uv_frames[ info.animated_uv_offset / 2 + a + frame_index ].x, uv_frames[ info.animated_uv_offset / 2 + a + frame_index].y ) * factor;
                    }
                }
            }
        }
    }
    
    // Update glow time.
    this->glow_time += seconds_passed * this->scale;
    
    if( this->glow_time > 2.0f )
        this->glow_time = 0.0f;
}

size_t Graphics::SDL2::GLES2::Internal::World::getTilAmount() const {
    return tiles.size();
}

int Graphics::SDL2::GLES2::Internal::World::setTilBlink( unsigned til_index, float frequency ) {
    if( til_index < tiles.size() )
    {
        tiles[ til_index ].current = 0.0;
        tiles[ til_index ].change_rate = frequency;

        return 1;
    }
    else
        return 0;
}

int Graphics::SDL2::GLES2::Internal::World::setPolygonTypeBlink( unsigned polygon_type, GLfloat scale ) {
    if( polygon_type < 112 ) {
        this->scale = scale;
        this->selected_tile = polygon_type;
        return 1;
    }
    else
        return 0;
}
