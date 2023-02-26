#include "../../Environment.h" // Include the interface class
#include "Environment.h" // Include the internal class
#include "Window.h"
#include "Text2DBuffer.h"
#include "Internal/GLES2.h"
#include <algorithm>

// This source code is a decendent of https://gist.github.com/SuperV1234/5c5ad838fe5fe1bf54f9 or SuperV1234

#include <iostream>
Graphics::Environment::Environment() {
    auto EnvironmentInternalData = new Graphics::SDL2::GLES2::EnvironmentInternalData;

    EnvironmentInternalData->world = nullptr;
    text_draw_routine_p = nullptr;
    Environment_internals = reinterpret_cast<void*>( EnvironmentInternalData ); // This is very important! This contains all the API specific variables.
    
    window_p = nullptr;
}

Graphics::Environment::~Environment() {
    // Close and destroy the window
    auto EnvironmentInternalData = reinterpret_cast<Graphics::SDL2::GLES2::EnvironmentInternalData*>( Environment_internals );
    
    if( text_draw_routine_p != nullptr )
    {
        delete text_draw_routine_p;
        text_draw_routine_p = nullptr;
    }
    
    for( auto texture : EnvironmentInternalData->textures ) {
        delete texture.second;
        texture.second = nullptr;
    }
    
    delete EnvironmentInternalData;
    
    if( window_p != nullptr )
        delete window_p;
}

std::vector<std::string> Graphics::Environment::getAvailableIdentifiers() {
    std::vector<std::string> identifiers;
    
    identifiers.push_back( "OpenGL ES 2" );
    
    return identifiers;
}

bool Graphics::Environment::isIdentifier( const std::string &identifier ) {
    if( identifier.compare( "OpenGL ES 2" ) == 0 )
        return true;
    else
        return false;
}

Graphics::Environment* Graphics::Environment::alloc( const std::string &identifier ) {
    // TODO Choose which renderer to attach this to based on the identifier.
    // However, the only choice right now is OpenGLES 2.
    
    if( isIdentifier(identifier) )
        return new Environment();
    else
        return nullptr;
}

std::string Graphics::Environment::getEnvironmentIdentifier() const {
    return "OpenGL ES 2";
}

int Graphics::Environment::initSystem() {
    if( SDL_InitSubSystem( SDL_INIT_VIDEO ) == 0 ) {
        return 1;
    }
    else
        return 0;
}

int Graphics::Environment::deinitEntireSystem() {
    SDL_GL_UnloadLibrary();    
    
    SDL_Quit();

    return 1;
}

int Graphics::Environment::setupTextures( const std::vector<Data::Mission::BMPResource*> &textures ) {
    auto EnvironmentInternalData = reinterpret_cast<Graphics::SDL2::GLES2::EnvironmentInternalData*>( Environment_internals );
    int failed_texture_loads = 0; // A counter for how many textures failed to load at first.

    int shine_index = -1;
    
    for( unsigned int i = 0; i < textures.size(); i++ )
    {
        auto converted_texture = textures[i];
        if( converted_texture != nullptr )
        {
            Utilities::Image2D image_accessor( *converted_texture->getImage(), Utilities::PixelFormatColor_R8G8B8A8() );
            
            const auto CBMP_ID = converted_texture->getResourceID();

            EnvironmentInternalData->textures[ CBMP_ID ] = new SDL2::GLES2::Internal::Texture2D;
            
            EnvironmentInternalData->textures[ CBMP_ID ]->setCBMPResourceID( CBMP_ID );
            EnvironmentInternalData->textures[ CBMP_ID ]->setFilters( 0, GL_NEAREST, GL_LINEAR );
            EnvironmentInternalData->textures[ CBMP_ID ]->setImage( 0, 0, GL_RGBA, image_accessor.getWidth(), image_accessor.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image_accessor.getDirectGridData() );
            
            if( CBMP_ID == 10 )
                shine_index = i;
        }
        else
            failed_texture_loads--;
    }
    
    if( !textures.empty() ) {
        Utilities::Image2D environment_image( 0, 0, Utilities::PixelFormatColor_R8G8B8A8() );
        bool success;
        
        if( shine_index < 0 )
            success = textures.back()->getImage()->subImage( 0, 124, 128, 128, environment_image );
        else
            success = textures.at( shine_index )->getImage()->subImage( 0, 124, 128, 128, environment_image );

        EnvironmentInternalData->shiney_texture.setFilters( 1, GL_NEAREST, GL_LINEAR );
        EnvironmentInternalData->shiney_texture.setImage( 1, 0, GL_RGBA, environment_image.getWidth(), environment_image.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, environment_image.getDirectGridData() );
    }

    if( failed_texture_loads == 0 )
        return 1;
    else
        return -failed_texture_loads;
}

void Graphics::Environment::setMap( const Data::Mission::PTCResource &ptc, const std::vector<Data::Mission::TilResource*> &tiles ) {
    auto EnvironmentInternalData = reinterpret_cast<Graphics::SDL2::GLES2::EnvironmentInternalData*>( Environment_internals );
    
    // Allocate the world
    EnvironmentInternalData->world = new Graphics::SDL2::GLES2::Internal::World();
    
    // Setup the vertex and fragment shaders
    EnvironmentInternalData->world->setVertexShader();
    EnvironmentInternalData->world->setFragmentShader();
    EnvironmentInternalData->world->compilieProgram();
    
    // Turn the map into a world.
    EnvironmentInternalData->world->setWorld( ptc, tiles, EnvironmentInternalData->textures );
}

int Graphics::Environment::setModelTypes( const std::vector<Data::Mission::ObjResource*> &model_types ) {
    auto err = glGetError();

    if( err != GL_NO_ERROR )
        std::cout << "Call Before Graphics::Environment::setModelTypes is broken! " << err << std::endl;

    auto EnvironmentInternalData = reinterpret_cast<Graphics::SDL2::GLES2::EnvironmentInternalData*>( Environment_internals );
    int number_of_failures = 0; // TODO make sure that this gets set.
    Utilities::ModelBuilder* model;
    
    // Setup the vertex and fragment shaders
    EnvironmentInternalData->static_model_draw_routine.setVertexShader();
    EnvironmentInternalData->static_model_draw_routine.setFragmentShader();
    EnvironmentInternalData->static_model_draw_routine.compileProgram();
    
    EnvironmentInternalData->static_model_draw_routine.setTextures( &EnvironmentInternalData->shiney_texture );

    err = glGetError();

    if( err != GL_NO_ERROR )
        std::cout << "Static Model shader is broken!: " << err << std::endl;
    
    EnvironmentInternalData->morph_model_draw_routine.setVertexShader();
    EnvironmentInternalData->morph_model_draw_routine.setFragmentShader();
    EnvironmentInternalData->morph_model_draw_routine.compileProgram();

    EnvironmentInternalData->morph_model_draw_routine.setTextures( &EnvironmentInternalData->shiney_texture );
    
    err = glGetError();

    if( err != GL_NO_ERROR )
        std::cout << "Morph Model shader is broken!: " << err << std::endl;

    EnvironmentInternalData->skeletal_model_draw_routine.setVertexShader();
    EnvironmentInternalData->skeletal_model_draw_routine.setFragmentShader();
    EnvironmentInternalData->skeletal_model_draw_routine.compileProgram();
    
    EnvironmentInternalData->skeletal_model_draw_routine.setTextures( &EnvironmentInternalData->shiney_texture );
    
    err = glGetError();

    if( err != GL_NO_ERROR )
        std::cout << "Skeletal Model shader is broken!: " << err << std::endl;
    
    for( unsigned int i = 0; i < model_types.size(); i++ ) {
        if( model_types[ i ] != nullptr )
        {
            model = model_types[ i ]->createModel( nullptr ); // TODO revise createModel

            if( model != nullptr )
            {
                if( model->getNumJoints() > 0 )
                    EnvironmentInternalData->skeletal_model_draw_routine.inputModel( model, model_types[ i ]->getResourceID(), EnvironmentInternalData->textures );
                else
                if( model->getNumMorphFrames() > 0)
                    EnvironmentInternalData->morph_model_draw_routine.inputModel( model, model_types[ i ]->getResourceID(), EnvironmentInternalData->textures );
                else
                    EnvironmentInternalData->static_model_draw_routine.inputModel( model, model_types[ i ]->getResourceID(), EnvironmentInternalData->textures );
            }
        }
    }
    
    err = glGetError();

    if( err != GL_NO_ERROR )
        std::cout << "Graphics::Environment::setModelTypes has an OpenGL Error: " << err << std::endl;

    return number_of_failures;
}

int Graphics::Environment::setTilBlink( int til_index, float seconds ) {
    auto EnvironmentInternalData = reinterpret_cast<Graphics::SDL2::GLES2::EnvironmentInternalData*>( Environment_internals );

    if( EnvironmentInternalData->world != nullptr )
    {
        return EnvironmentInternalData->world->setTilBlink( til_index, seconds );
    }
    else
        return -1; // The world needs allocating first!
}

int Graphics::Environment::setTilPolygonBlink( unsigned polygon_type, float rate ) {
    auto EnvironmentInternalData = reinterpret_cast<Graphics::SDL2::GLES2::EnvironmentInternalData*>( Environment_internals );

    if( EnvironmentInternalData->world != nullptr )
    {
        return EnvironmentInternalData->world->setPolygonTypeBlink( polygon_type, rate );
    }
    else
        return -1; // The world needs allocating first!
}

void Graphics::Environment::drawFrame() const {
    auto EnvironmentInternalData = reinterpret_cast<Graphics::SDL2::GLES2::EnvironmentInternalData*>( Environment_internals );
    
    auto window_r =  window_p;
    
    auto window_SDL_r = dynamic_cast<Graphics::SDL2::GLES2::Window*>( window_r );
    Graphics::Camera* current_camera; // Used to store the camera.
    glm::mat4 camera_3D_projection_view_model; // This holds the two transforms from above.

    // Clear the screen to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    for( unsigned int i = 0; i < window_r->getCameras()->size(); i++ )
    {
        // Setup the current camera.
        current_camera = window_r->getCameras()->at( i );

        if( current_camera != nullptr )
        {
            // Set the viewport
            glViewport( current_camera->getViewportOrigin().x, current_camera->getViewportOrigin().y, current_camera->setViewportDimensions().x, current_camera->setViewportDimensions().y );

            // When drawing the 3D objects the depth test must be turned on.
            glEnable(GL_DEPTH_TEST);
            
            // This is very crude blending.
            glEnable( GL_BLEND );
            glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

            // Draw the map if available.
            if( EnvironmentInternalData->world != nullptr )
            {
                // Enable culling on the world map.
                // glEnable( GL_CULL_FACE );
                // glCullFace( GL_FRONT ); // The floor seems to be in reverse order! I have got to fix the floor and the slopes.
                
                // Draw the map.
                EnvironmentInternalData->world->draw( *current_camera );
                
                // Disable culling on the world map.
                // glDisable( GL_CULL_FACE );
            }

            EnvironmentInternalData->static_model_draw_routine.draw(   *current_camera );
            EnvironmentInternalData->morph_model_draw_routine.draw(    *current_camera );
            EnvironmentInternalData->skeletal_model_draw_routine.draw( *current_camera );

            // When drawing the GUI elements depth test must be turned off.
            glDisable(GL_DEPTH_TEST);
            glEnable( GL_BLEND ); // Easier to implement blending here.
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            current_camera->getProjectionView2D( camera_3D_projection_view_model );

            for( auto i = current_camera->getText2DBuffer()->begin(); i != current_camera->getText2DBuffer()->end(); i++ )
            {
                // TODO Eventually remove this kind of upcasts. They are dangerious.
                auto text_2d_draw_routine = dynamic_cast<Graphics::SDL2::GLES2::Text2DBuffer*>( *i );
                
                assert( text_2d_draw_routine != nullptr );
                
                text_2d_draw_routine->draw( camera_3D_projection_view_model );
            }
        }

        // Finally swap the window.
        SDL_GL_SwapWindow( window_SDL_r->window_p );
    }
}

bool Graphics::Environment::screenshot( Utilities::Image2D &image ) const {
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

void Graphics::Environment::advanceTime( float seconds_passed ) {
    auto EnvironmentInternalData = reinterpret_cast<Graphics::SDL2::GLES2::EnvironmentInternalData*>( Environment_internals );

    // For animatable meshes advance the time
    EnvironmentInternalData->morph_model_draw_routine.advanceTime( seconds_passed );
    EnvironmentInternalData->skeletal_model_draw_routine.advanceTime( seconds_passed );

    // The world map also has the concept of time if it exists.
    if( EnvironmentInternalData->world != nullptr )
        EnvironmentInternalData->world->advanceTime( seconds_passed );
}
