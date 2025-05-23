#include "../../Environment.h" // Include the interface class
#include "Environment.h" // Include the internal class
#include "Window.h"
#include "Camera.h"
#include "Text2DBuffer.h"
#include "Internal/GLES2.h"
#include <algorithm>

#include <iostream>
#include "../../../Data/Mission/TOSResource.h"

// This source code is a decendent of https://gist.github.com/SuperV1234/5c5ad838fe5fe1bf54f9 or SuperV1234

namespace Graphics::SDL2::GLES2 {

Environment::Environment() : window_p( nullptr ) {
    this->world_p             = nullptr;
    this->shiney_texture_p    = nullptr;

    this->display_world = false;
    this->has_initialized_routines = false;
    this->draw_bounding_boxes = false;

    this->force_gl2 = false;
    this->semi_transparent_limit = 256;
}

Environment::~Environment() {
    // Close and destroy the window
    if( this->window_p != nullptr )
        delete this->window_p;

    for( auto texture : this->textures ) {
        delete texture.second;
        texture.second = nullptr;
    }
    
    if( this->world_p != nullptr)
        delete this->world_p;
}

int Environment::initSystem() {
    if( SDL_InitSubSystem( SDL_INIT_VIDEO ) == 0 ) {
        return 1;
    }
    else
        return 0;
}

int Environment::deinitEntireSystem() {
    SDL_GL_UnloadLibrary();    
    
    SDL_Quit();

    return 1;
}

std::string Environment::getEnvironmentIdentifier() const {
    return Graphics::Environment::SDL2_WITH_GLES_2;
}

int Environment::loadResources( const Data::Accessor &accessor ) {
    int problem_level = 1; // -1 means total failure, 0 means partial success, but some errors, 1 means loaded with no problems.

    auto error_log = Utilities::logger.getLog( Utilities::Logger::ERROR );
    error_log.info << "GLES 2 Graphics load resources\n";

    if( this->draw_2d_routine.text_draw_routine_p != nullptr )
        delete this->draw_2d_routine.text_draw_routine_p;

    {
        std::vector<const Data::Mission::FontResource*> fonts_r = accessor.getAllConstFNT();

        this->draw_2d_routine.text_draw_routine_p = new Graphics::SDL2::GLES2::Internal::FontSystem( fonts_r );
    }

    this->anm_resources.clear();

    auto tos_resource_r = accessor.getConstTOS( 1 );

    if(tos_resource_r != nullptr) {
        for(const uint32_t tos_offset: tos_resource_r->getOffsets()) {
            const Data::Accessor *swvr_accessor_r = accessor.getSWVRAccessor(tos_offset);

            if(swvr_accessor_r == nullptr)
                continue;

            auto canm_r = swvr_accessor_r->getConstANM(1);

            if(canm_r == nullptr)
                continue;

            this->anm_resources[tos_offset] = canm_r;
        }
    }

    std::vector<const Data::Mission::BMPResource*> textures = accessor.getAllConstBMP();

    int shine_index = -1;

    for( auto i : this->textures ) {
        delete i.second;
    }
    this->textures.clear();

    if( this->shiney_texture_p != nullptr )
        delete this->shiney_texture_p;
    this->shiney_texture_p = nullptr;

    // Make a no texture texture. Yes, it is easier to make a texture than to attempt to make a no texture state in OpenGLES 2.
    // The 32x32 image has 8x8 white squares at each four corners of the image. The rest of the image is filled with a checker board of purple and violet.
    // TODO Maybe make it so that this texture is not regenerated each time.
    {
        const unsigned DIMENSION = 32;
        const unsigned CORNER    = DIMENSION / 4;
        const auto WHITE = Utilities::PixelFormatColor::GenericColor( 1.0f, 1.0f, 1.0f, 1.0f );
        const Utilities::PixelFormatColor::GenericColor CHECKER[2] = {
            Utilities::PixelFormatColor::GenericColor( 1.0f, 0.0f, 1.0f, 1.0f ),
            Utilities::PixelFormatColor::GenericColor( 0.5f, 0.0f, 1.0f, 1.0f ) };


        Utilities::Image2D image_accessor( DIMENSION, DIMENSION, Utilities::PixelFormatColor_R8G8B8A8::linear );

        for( unsigned y = 0; y < DIMENSION; y++ ) {
            for( unsigned x = 0; x < DIMENSION; x++ ) {
                if( x < CORNER && y < CORNER )
                    image_accessor.writePixel( x, y, WHITE );
                else if( x >= CORNER * 3 && y < CORNER )
                    image_accessor.writePixel( x, y, WHITE );
                else if( x >= CORNER * 3 && y >= CORNER * 3 )
                    image_accessor.writePixel( x, y, WHITE );
                else if( x < CORNER      && y >= CORNER * 3 )
                    image_accessor.writePixel( x, y, WHITE );
                else
                    image_accessor.writePixel( x, y, CHECKER[(x + y) % 2] );
            }
        }

        this->textures[ 0 ] = new SDL2::GLES2::Internal::Texture2D;

        this->textures[ 0 ]->setFilters( 0, GL_NEAREST, GL_LINEAR );
        this->textures[ 0 ]->setImage( 0, 0, GL_RGBA, image_accessor.getWidth(), image_accessor.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image_accessor.getDirectGridData() );
    }

    for( unsigned int i = 0; i < textures.size(); i++ )
    {
        auto converted_texture = textures[i];
        if( converted_texture != nullptr )
        {
            Utilities::Image2D image_accessor( *converted_texture->getImage(), Utilities::PixelFormatColor_R8G8B8A8::linear );
            
            const auto CBMP_ID = converted_texture->getResourceID();

            this->textures[ CBMP_ID ] = new SDL2::GLES2::Internal::Texture2D;
            
            this->textures[ CBMP_ID ]->setFilters( 0, GL_NEAREST, GL_LINEAR );
            this->textures[ CBMP_ID ]->setImage( 0, 0, GL_RGBA, image_accessor.getWidth(), image_accessor.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image_accessor.getDirectGridData() );
            
            if( CBMP_ID == 10 )
                shine_index = i;
        }
        else {
            problem_level = 0;

            error_log.output << "NULL Texture at Slot "<< std::dec << i << "\n";
        }
    }
    
    if( !textures.empty() ) {
        Utilities::Image2D environment_image( 0, 0, Utilities::PixelFormatColor_R8G8B8A8::linear );

        if( shine_index < 0 )
            textures.back()->getImage()->subImage( 0, 124, 128, 128, environment_image );
        else
            textures.at( shine_index )->getImage()->subImage( 0, 124, 128, 128, environment_image );

        this->shiney_texture_p = new Internal::Texture2D();

        this->shiney_texture_p->setFilters( 1, GL_NEAREST, GL_LINEAR );
        this->shiney_texture_p->setImage( 1, 0, GL_RGBA, environment_image.getWidth(), environment_image.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, environment_image.getDirectGridData() );
    }

    // Erase the culling data.
    if( this->window_p != nullptr ) {
        for( unsigned int i = 0; i < window_p->getCameras()->size(); i++ ) {
            auto current_camera_r = dynamic_cast<Graphics::SDL2::GLES2::Camera*>(window_p->getCameras()->at( i ));

            if( current_camera_r != nullptr )
                current_camera_r->culling_info.setDimensions( 0, 0 );
        }
    }

    // Destory the last world.
    if( this->world_p != nullptr )
        delete this->world_p;
    this->world_p = nullptr;

    const Data::Mission::PTCResource* ptc_r = accessor.getConstPTC( 1 );
    std::vector<const Data::Mission::TilResource*> map_sections = accessor.getAllConstTIL();

    // Make sure that the pointers are not pointers.
    if( ptc_r != nullptr && map_sections.size() != 0 ) {
        // Allocate the world
        this->world_p = new Internal::World();

        // Setup the vertex and fragment shaders
        this->world_p->setVertexShader(map_sections);
        this->world_p->setFragmentShader();
        this->world_p->compileProgram();

        this->map_section_width  = ptc_r->getWidth();
        this->map_section_height = ptc_r->getHeight();

        // Turn the map into a world.
        this->world_p->setWorld( *ptc_r, map_sections, this->textures );
    }

    auto err = glGetError();

    if( err != GL_NO_ERROR )
        error_log.output << "Call Before Graphics::Environment::setModelTypes is broken! " << err << "\n";

    if( !this->has_initialized_routines ) {
        // Setup the 2D vertex and fragment shaders
        this->draw_2d_routine.setVertexShader();
        this->draw_2d_routine.setFragmentShader();
        this->draw_2d_routine.compileProgram();

        // Setup the vertex and fragment shaders
        this->static_model_draw_routine.setVertexShader();
        this->static_model_draw_routine.setFragmentShader();
        this->static_model_draw_routine.compileProgram();

        err = glGetError();

        if( err != GL_NO_ERROR )
            error_log.output << "Static Model shader is broken!: " << err << "\n";

        this->morph_model_draw_routine.setVertexShader();
        this->morph_model_draw_routine.setFragmentShader();
        this->morph_model_draw_routine.compileProgram();

        err = glGetError();

        if( err != GL_NO_ERROR )
            error_log.output << "Morph Model shader is broken!: " << err << "\n";

        this->skeletal_model_draw_routine.setVertexShader();
        this->skeletal_model_draw_routine.setFragmentShader();
        this->skeletal_model_draw_routine.compileProgram();

        err = glGetError();

        if( err != GL_NO_ERROR )
            error_log.output << "Skeletal Model shader is broken!: " << err << "\n";

        this->dynamic_triangle_draw_routine.setVertexShader();
        this->dynamic_triangle_draw_routine.setFragmentShader();
        this->dynamic_triangle_draw_routine.compileProgram();

        err = glGetError();

        if( err != GL_NO_ERROR )
            error_log.output << "Dynamic Triangle is broken!: " << err << "\n";

        this->has_initialized_routines = true;
    }
    else {
        this->skeletal_model_draw_routine.clearModels();
        this->morph_model_draw_routine.clearModels();
        this->static_model_draw_routine.clearModels();
    }

    this->static_model_draw_routine.setEnvironmentTexture( this->shiney_texture_p );
    this->morph_model_draw_routine.setEnvironmentTexture( this->shiney_texture_p );
    this->skeletal_model_draw_routine.setEnvironmentTexture( this->shiney_texture_p );
    this->dynamic_triangle_draw_routine.setEnvironmentTexture( this->shiney_texture_p );

    Utilities::ModelBuilder *model_r;
    std::vector<const Data::Mission::ObjResource*> model_types = accessor.getAllConstOBJ();

    for( unsigned int i = 0; i < model_types.size(); i++ ) {
        if( model_types[ i ] != nullptr ) {
            model_r = model_types[ i ]->createModel();

            if( model_r != nullptr ) {
                if( model_r->getNumJoints() > 0 )
                    this->skeletal_model_draw_routine.inputModel( model_r, *model_types[ i ], this->textures );
                else
                if( model_r->getNumMorphFrames() > 0)
                    this->morph_model_draw_routine.inputModel( model_r, *model_types[ i ], this->textures );
                else
                    this->static_model_draw_routine.inputModel( model_r, *model_types[ i ], this->textures );
            }
            else {
                problem_level = 0;

                error_log.output << "Model " << std::dec << model_types[ i ]->getResourceID() << " createModel has failed!" << "\n";
            }

            model_r = model_types[ i ]->createBoundingBoxes();

            if( model_r != nullptr ) {
                if( model_r->getNumMorphFrames() > 0)
                    this->morph_model_draw_routine.inputBoundingBoxes( model_r, model_types[ i ]->getResourceID(), this->textures );
                else
                    this->static_model_draw_routine.inputBoundingBoxes( model_r, model_types[ i ]->getResourceID(), this->textures );
            }
            else {
                problem_level = 0;

                error_log.output << "Model " << std::dec << model_types[ i ]->getResourceID() << " createBoundingBoxes has failed!" << "\n";
            }
        }
    }
    
    err = glGetError();

    if( err != GL_NO_ERROR )
        error_log.output << "Graphics::Environment::setModelTypes has an OpenGL Error: " << err << "\n";

    this->particle_draw_routine.load(accessor, this->textures);

    return problem_level;
}

Graphics::Window* Environment::getWindow() {
    return this->window_p;
}

bool Environment::displayMap( bool state ) {
    if( this->world_p != nullptr ) {
        display_world = state;
        return true;
    }
    else
        return false;
}

size_t Environment::getTilAmount() const {
    if( this->world_p != nullptr )
    {
        return this->world_p->getTilAmount();
    }
    else
        return 0; // There are no Ctils to read if there is no world.
}

int Environment::setTilBlink( unsigned til_index, float seconds ) {
    if( this->world_p != nullptr )
    {
        return this->world_p->setTilBlink( til_index, seconds );
    }
    else
        return -1; // The world needs allocating first!
}

int Environment::setTilPolygonBlink( unsigned polygon_type, float rate ) {
    if( this->world_p != nullptr )
    {
        return this->world_p->setPolygonTypeBlink( polygon_type, rate );
    }
    else
        return -1; // The world_p needs allocating first!
}

bool Environment::getBoundingBoxDraw() const {
    return this->draw_bounding_boxes;
}

void Environment::setBoundingBoxDraw(bool draw) {
    this->draw_bounding_boxes = draw;
}

void Environment::setupFrame() {
    for( unsigned int i = 0; i < window_p->getCameras()->size(); i++ )
    {
        // Setup the current camera.
        auto current_camera_r = dynamic_cast<Graphics::SDL2::GLES2::Camera*>(window_p->getCameras()->at( i ));

        if( current_camera_r != nullptr && this->world_p != nullptr )
        {
            if( current_camera_r->culling_info.getWidth() * current_camera_r->culling_info.getHeight() == 0 )
                current_camera_r->culling_info.setDimensions( this->map_section_width, this->map_section_height );

            this->world_p->updateCulling( *current_camera_r );
        }
    }
}

void Environment::drawFrame() {
    GLES2::Camera* current_camera_r; // Used to store the camera.

    // Clear the screen to black
    glClearColor(0.125f, 0.125f, 0.25f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    for( unsigned int i = 0; i < this->window_p->getCameras()->size(); i++ )
    {
        // Setup the current camera.
        current_camera_r = dynamic_cast<Graphics::SDL2::GLES2::Camera*>(this->window_p->getCameras()->at( i ));

        if( current_camera_r != nullptr )
        {
            // Set the viewport
            glViewport( current_camera_r->getViewportOrigin().x, current_camera_r->getViewportOrigin().y, current_camera_r->getViewportDimensions().x, current_camera_r->getViewportDimensions().y );

            // When drawing the 3D objects the depth test must be turned on.
            glEnable(GL_DEPTH_TEST);

            // Enable culling on the opaque rendering path.
            glEnable( GL_CULL_FACE );

            // Also there will be no blending as well.
            glDisable( GL_BLEND );

            // Draw the map if available.
            if( this->world_p != nullptr && this->display_world )
            {
                // Draw the map.
                this->world_p->draw( *current_camera_r );
            }

            this->static_model_draw_routine.draw(   *current_camera_r );
            this->morph_model_draw_routine.draw(    *current_camera_r );
            this->skeletal_model_draw_routine.draw( *current_camera_r );

            this->particle_draw_routine.draw( *current_camera_r );

            glEnable( GL_BLEND );
            glDepthMask(GL_FALSE);

            this->dynamic_triangle_draw_routine.draw( *current_camera_r, textures );
            current_camera_r->transparent_triangles.reset();
            glDepthMask(GL_TRUE);

            // When drawing the GUI elements depth test must be turned off.
            glDisable(GL_DEPTH_TEST);

            if(draw_bounding_boxes) {
                this->static_model_draw_routine.drawBoundingBoxes( *current_camera_r );
                this->morph_model_draw_routine.drawBoundingBoxes(  *current_camera_r );
            }

            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            this->draw_2d_routine.draw( *current_camera_r );
        }

        // Finally swap the window.
        SDL_GL_SwapWindow( this->window_p->window_p );
    }
}

bool Environment::screenshot( Utilities::Image2D &image ) const {
    auto gl_error = glGetError();

    if( gl_error != GL_NO_ERROR ) {
        std::cout << "There is an OpenGL error before Graphics::Environment::screenshot(...)\n";
        std::cout << " This error is " << gl_error << std::endl;
    }

    // if( image.isValid() && getHeight() < window TODO Work on type protection later.
    glReadPixels( 0, 0, image.getWidth(), image.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, image.getDirectGridData() );
    
    // This is a quick and easy way to fix the flipped image.
    // However, the price is that I replaced it with an O squared operation.
    image.flipVertically();

    gl_error = glGetError();

    if( gl_error != GL_NO_ERROR ) {
        std::cout << "There is an OpenGL error in glReadPixels\n";
        std::cout << " This error is " << gl_error << std::endl;
        return false;
    }
    else
        return true;
}

void Environment::advanceTime( std::chrono::microseconds delta ) {
    float seconds_passed = std::chrono::duration<float>( delta ).count();

    // For animatable meshes advance the time
    this->static_model_draw_routine.advanceTime( seconds_passed );
    this->morph_model_draw_routine.advanceTime( seconds_passed );
    this->skeletal_model_draw_routine.advanceTime( seconds_passed );

    // The world map also has the concept of time if it exists.
    if( this->world_p != nullptr )
        this->world_p->advanceTime( seconds_passed );
}

}
