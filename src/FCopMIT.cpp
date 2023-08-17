#include <iostream>
#include <ratio>
#include <chrono>
#include <thread>

#include "Config.h"

#include "Data/Manager.h"

#include "Data/Mission/IFF.h"
#include "Data/Mission/ObjResource.h"
#include "Data/Mission/BMPResource.h"
#include "Data/Mission/PTCResource.h"
#include "Data/Mission/Til/Mesh.h"
#include "Data/Mission/Til/Colorizer.h"

#include "Graphics/Environment.h"

#include "Controls/System.h"
#include "Controls/StandardInputSet.h"

#include "Utilities/Logger.h"
#include "Utilities/ImageFormat/Chooser.h"
#include "Utilities/Options/Parameters.h"
#include "Utilities/Options/Paths.h"
#include "Utilities/Options/Options.h"

#include "ConfigureInput.h"
#include "SplashScreens.h"

class MainProgram {
protected:
    // Read the parameter system.
    Utilities::Options::Parameters parameters;
    Utilities::Options::Paths      paths;
    Utilities::Options::Options    options;

    // Data gathered from the options.
    std::string program_name;

    Data::Manager manager;
    Data::Manager::Platform platform;
    std::string resource_identifier;
    Data::Mission::IFF *global_r;
    Data::Mission::IFF *resource_r;

    // Graphics API variables goes here.
    std::string graphics_identifier;
    Graphics::Environment *environment_p;
    Graphics::Text2DBuffer *text_2d_buffer_r;
    Graphics::Camera *first_person_r;

    // Controls API variables goes here.
    Controls::System *control_system_p;
    Controls::CursorInputSet *control_cursor_r;
    std::vector<Controls::StandardInputSet*> controllers_r;
    glm::vec3 camera_position;
    glm::vec2 camera_rotation;
    float     camera_distance;

public:
    MainProgram( int argc, char** argv ) : parameters( argc, argv ), paths( parameters ), options( paths, parameters ) {

        // Set everything to null.
        this->global_r         = nullptr;
        this->resource_r       = nullptr;
        this->environment_p    = nullptr;
        this->text_2d_buffer_r = nullptr;
        this->first_person_r   = nullptr;
        this->control_system_p = nullptr;

        if( parameters.help.getValue() ) {
            return;
        }

        // This map has both vertex animations. The uv animations and the color animations.
        this->resource_identifier = Data::Manager::pa_venice_beach;
        this->platform = Data::Manager::Platform::WINDOWS;

        setupLogging();
        initGraphics();
        setupGraphics();
        loadResources();
        loadGraphics();
        setupCamera();
        setupControls();

        centerCamera();
        updateCamera();

        float delta_f = 1.0f;

        // Make the control system poll all the inputs.
        control_system_p->advanceTime( delta_f );

        // Grab the inputs for either menu or primary game.

        // Render GUI overlayed with menu when available.

        // Render the frame.
        environment_p->setupFrame();
        environment_p->advanceTime( delta_f );
        environment_p->drawFrame();

        std::this_thread::sleep_for( std::chrono::seconds(5) );
    }

    virtual ~MainProgram() {
        cleanup();
    }

protected:

void throwException( std::string output ) {
    {
        auto log = Utilities::logger.getLog( Utilities::Logger::CRITICAL );
        log.output << output << "\n";
    }

    cleanup();

    throw std::runtime_error( output );
}

private:

    void setupLogging() {
        // Setup the professional logger next.
        Utilities::logger.setOutputLog( &std::cout, 0, Utilities::Logger::INFO );

        {
            auto initialize_log = Utilities::logger.getLog( Utilities::Logger::ALL );
            initialize_log.output << parameters.getBinaryName() << " started at ";
        }

        Utilities::logger.setTimeStampMode( true );

        // Export some info.
        {
            auto info_log = Utilities::logger.getLog( Utilities::Logger::INFO );
            info_log.output << "Using config file: " << paths.getConfigFilePath()  << "\n";
        }
        {
            auto debug_log = Utilities::logger.getLog( Utilities::Logger::DEBUG );
            debug_log.output << "\nData directories\n"
                             << "  Windows directory:     " << options.getWindowsDataDirectory()     << "\n"
                             << "  Macintosh directory:   " << options.getMacintoshDataDirectory()   << "\n"
                             << "  Playstation directory: " << options.getPlaystationDataDirectory() << "\n"
                             << "\nUser directories\n"
                             << "  Savedgames directory:  " << options.getSaveDirectory()            << "\n"
                             << "  Screenshots directory: " << options.getScreenshotsDirectory()     << "\n";
        }
    }

    void initGraphics() {
        auto graphics_identifiers = Graphics::Environment::getAvailableIdentifiers();

        if( graphics_identifiers.empty() )
            throwException( "Graphics has no available identifiers. Therefore there is nothing for the game to render to." );

        if( !Graphics::Environment::isIdentifier( graphics_identifiers[0] ) )
            throwException( "The graphics identifier \"" + graphics_identifiers[0] + "\" is not an identifer." );

        this->graphics_identifier = graphics_identifiers[0];

        Graphics::Environment::initSystem( this->graphics_identifier );
    }

    void setupGraphics() {
        this->environment_p = Graphics::Environment::alloc( this->graphics_identifier );

        if( this->environment_p == nullptr )
            throwException( "Sorry, but OpenGL 2/OpenGL ES 2 are the minimum requirements for this engine. Identifier: " + this->graphics_identifier );

        // Declare a pointer
        Graphics::Window *window_r = Graphics::Window::alloc( *this->environment_p );

        if( window_r == nullptr )
            throwException( "The graphics window has failed to allocate." );

        std::string title = "Future Cop M.I.T.";

        window_r->setWindowTitle( title );
        window_r->setDimensions( glm::u32vec2( options.getVideoWidth(), options.getVideoHeight() ) );
        window_r->setFullScreen( options.getVideoFullscreen() );

        if( window_r->attach() != 1 )
            throwException( "The graphics window has failed to attach." );
    }

    void loadResources() {
        manager.autoSetEntries( options.getWindowsDataDirectory(),     Data::Manager::Platform::WINDOWS );
        manager.autoSetEntries( options.getMacintoshDataDirectory(),   Data::Manager::Platform::MACINTOSH );
        manager.autoSetEntries( options.getPlaystationDataDirectory(), Data::Manager::Platform::PLAYSTATION );

        // TODO If the global path is specified then use a specified path.
        /* if( global_path.compare("") != 0 ) {
            Data::Manager::IFFEntry entry = manager.getIFFEntry( Data::Manager::global );
            // Just in case if this was not set on global id.
            entry.importance = Data::Manager::Importance::NEEDED;
            // Overide the global path.
            entry.setPath( platform, global_path );
            manager.setIFFEntry( global_id, entry );
        }*/

        // TODO If the mission path is specified then use a specified path.
        /* if( mission_path.compare("") != 0  ) {
            resource_identifier = "unk_custom_mission";

            Data::Manager::IFFEntry entry = manager.getIFFEntry( resource_identifier );
            // Overide the global path.
            entry.setPath( platform, mission_path );
            manager.setIFFEntry( resource_identifier, entry );
        }*/

        auto entry = manager.getIFFEntry( resource_identifier );
        entry.importance = Data::Manager::Importance::NEEDED;

        if( !manager.setIFFEntry( this->resource_identifier, entry ) )
            throwException( "Set IFF Entry has failed for \"" + this->resource_identifier + "\"." );

        manager.togglePlatform( this->platform, true );

        manager.setLoad( Data::Manager::Importance::NEEDED );

        this->global_r = manager.getIFFEntry( Data::Manager::global ).getIFF( this->platform );
        if( this->global_r == nullptr ) {
            auto log = Utilities::logger.getLog( Utilities::Logger::ERROR );
            log.output << "The global IFF " << Data::Manager::global << " did not load.";
        }

        this->resource_r = manager.getIFFEntry( this->resource_identifier ).getIFF( this->platform );
        if( this->resource_r == nullptr ) {
            auto log = Utilities::logger.getLog( Utilities::Logger::ERROR );
            log.output << "The mission IFF " << this->resource_identifier << " did not load.";
        }
    }

    void loadGraphics() {
        if( this->resource_r != nullptr ) {
            // First get the model textures from the resource file.
            auto cbmp_resources = Data::Mission::BMPResource::getVector( *this->resource_r );

            int status = this->environment_p->setupTextures( cbmp_resources );

            if( status < 0 ) {
                auto log = Utilities::logger.getLog( Utilities::Logger::ERROR );
                log.output << (-status) << " general textures had failed to load out of " << cbmp_resources.size();
            }

            // Load all the 3D meshes from the resource as well.
            auto cobj_resources = Data::Mission::ObjResource::getVector( *this->resource_r );

            status = this->environment_p->setModelTypes( cobj_resources );

            if( status < 0 ) {
                auto log = Utilities::logger.getLog( Utilities::Logger::ERROR );
                log.output << (-status) << " 3d meshes had failed to load out of " << cobj_resources.size();
            }

            std::vector<Data::Mission::TilResource*> til_resources = Data::Mission::TilResource::getVector( *this->resource_r );

            this->environment_p->setMap( *Data::Mission::PTCResource::getVector( *this->resource_r ).at( 0 ), til_resources );
        }

        std::vector<Data::Mission::IFF*> loaded_IFFs;

        if( this->global_r != nullptr )
            loaded_IFFs.push_back( this->global_r );
        if( this->resource_r != nullptr )
            loaded_IFFs.push_back( this->resource_r );

        // Get the font from the resource file.
        if( Graphics::Text2DBuffer::loadFonts( *this->environment_p, loaded_IFFs ) == 0 ) {
            auto log = Utilities::logger.getLog( Utilities::Logger::ERROR );
            log.output << "Fonts are missing.";
        }

        this->text_2d_buffer_r = Graphics::Text2DBuffer::alloc( *this->environment_p );

        if( this->text_2d_buffer_r == nullptr )
            throwException( "The Graphics::Text2DBuffer has failed to allocate." );

        // Initialze the camera
        this->first_person_r = Graphics::Camera::alloc( *this->environment_p );
        this->first_person_r->attachText2DBuffer( *this->text_2d_buffer_r );
        this->environment_p->window_p->attachCamera( *this->first_person_r );

        // Center the camera.
        if( this->environment_p->window_p->center() != 1 ) {
            auto log = Utilities::logger.getLog( Utilities::Logger::ERROR );
            log.output << "The window had failed to center.";
        }
    }

    void setupCamera() {
        this->first_person_r->setViewportOrigin( glm::u32vec2( 0, 0 ) );
        this->first_person_r->setViewportDimensions( glm::u32vec2( options.getVideoWidth(), options.getVideoHeight() ) );

        glm::mat4 projection_matrix = glm::ortho( 0.0f, static_cast<float>( options.getVideoWidth() ), -static_cast<float>( options.getVideoHeight() ), 0.0f, -1.0f, 1.0f );

        this->first_person_r->setProjection2D( projection_matrix );

        projection_matrix = glm::perspective( glm::pi<float>() / 4.0f, static_cast<float>( options.getVideoWidth() ) / static_cast<float>( options.getVideoHeight() ), 0.1f, 200.0f );

        this->first_person_r->setProjection3D( projection_matrix );

        this->camera_position = { 0, 0, 0 };
        this->camera_rotation = glm::vec2( glm::pi<float>() / 4.0f, glm::pi<float>() / 4.0f );
        this->camera_distance = -20;

        updateCamera();
    }

    void centerCamera() {
        Data::Mission::PTCResource *map_r = Data::Mission::PTCResource::getVector( *this->resource_r ).at( 0 );

        if( map_r != nullptr ) {
            this->camera_position.x = static_cast<float>( map_r->getWidth()  - 1 ) / 2.0f * Data::Mission::TilResource::AMOUNT_OF_TILES;
            this->camera_position.y = 0;
            this->camera_position.z = static_cast<float>( map_r->getHeight() - 1 ) / 2.0f * Data::Mission::TilResource::AMOUNT_OF_TILES;
        }
        else
            this->camera_position = { 0, 0, 0 };
    }

    void updateCamera() {
        glm::mat4 extra_matrix_0;
        glm::mat4 extra_matrix_1;
        glm::mat4 extra_matrix_2;

        extra_matrix_0 = glm::rotate( glm::mat4(1.0f), -this->camera_rotation.x, glm::vec3( 0.0, 1.0, 0.0 ) );

        extra_matrix_0 = glm::translate( glm::mat4(1.0f), glm::vec3( 0, 0, this->camera_distance ) );
        extra_matrix_1 = glm::rotate( glm::mat4(1.0f), this->camera_rotation.y, glm::vec3( 1.0, 0.0, 0.0 ) ); // rotate up and down.
        extra_matrix_2 = extra_matrix_0 * extra_matrix_1;
        extra_matrix_1 = glm::rotate( glm::mat4(1.0f), this->camera_rotation.x, glm::vec3( 0.0, 1.0, 0.0 ) ); // rotate left and right.
        extra_matrix_0 = extra_matrix_2 * extra_matrix_1;
        extra_matrix_1 = glm::translate( glm::mat4(1.0f), -this->camera_position );
        extra_matrix_2 = extra_matrix_0 * extra_matrix_1;

        this->first_person_r->setView3D( extra_matrix_2 );
    }

    void setupControls() {
        // Setup the controls
        this->control_system_p = Controls::System::getSingleton(); // create the new system for controls
        this->controllers_r.push_back( new Controls::StandardInputSet( "Player 1" ) ); // It is not player_1_controller_r job to delete itself.
        control_system_p->addInputSet( this->controllers_r.back() );

        control_system_p->allocateCursor();
        this->control_cursor_r = control_system_p->getCursor();
    }

    void cleanup() {
        options.saveOptions();

        if( this->control_system_p != nullptr )
            delete this->control_system_p;

        if( this->environment_p != nullptr )
            delete this->environment_p;

        // Set everything to null.
        this->global_r         = nullptr;
        this->resource_r       = nullptr;
        this->environment_p    = nullptr;
        this->text_2d_buffer_r = nullptr;
        this->first_person_r   = nullptr;
        this->control_system_p = nullptr;
    }
};

int main(int argc, char** argv)
{
    MainProgram main_program( argc, argv );

    return 0;
}
