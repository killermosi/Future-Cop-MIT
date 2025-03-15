#include "Text2DBuffer.h"

#include "Environment.h"
#include "SDL2/GLES2/Environment.h"

#include <mini/ini.h>

namespace Graphics {

const std::string Environment::SDL2_WITH_GLES_2 = "GLES2";

Environment::Environment() : map_section_width( 0 ), map_section_height( 0 ), window_p( nullptr ) {
}

Environment::~Environment() {
    // Close and destroy the window
    if( this->window_p != nullptr )
        delete this->window_p;
}

std::vector<std::string> Environment::getAvailableIdentifiers() {
    std::vector<std::string> identifiers;

    identifiers.push_back( SDL2_WITH_GLES_2 );

    return identifiers;
}

bool Environment::isIdentifier( const std::string &identifier ) {
    if( identifier.compare( SDL2_WITH_GLES_2 ) == 0 )
        return true;
    else
        return false;
}

#define GRAPHICS_NUMBER_SETTING(source, variable, variable_string, default_value, min_value) \
if(!source.has(variable_string)) { \
    source[variable_string] = std::to_string(default_value); \
    changed_data = true; \
} \
try { \
    variable = std::stof( source[variable_string] ); \
    if( variable < min_value ) { \
        variable = min_value; \
        source[variable_string] = std::to_string(variable); \
        changed_data = true; \
    } \
} catch( const std::logic_error & logical_error ) { \
    variable = default_value; \
    source[variable_string] = std::to_string(variable); \
    changed_data = true; \
}

Environment* Environment::alloc( const std::filesystem::path& file_path, const std::string &prefered_identifier ) {
    std::filesystem::path full_file_path = file_path;

    full_file_path += ".ini";

    mINI::INIFile ini_file( full_file_path.string() );

    mINI::INIStructure ini_data;

    bool changed_data = false;

    std::string identifier;

    if(!ini_file.read(ini_data))
        changed_data = true;

    {
        ini_data["general"];

        if(!ini_data["general"].has("version_major") && ini_data["general"]["version_major"].compare("0") != 0) {
            ini_data["general"]["version_major"] = "0";
            changed_data = true;
        }

        if(!ini_data["general"].has("version_minor")) {
            changed_data = true;
        }

        int version_minor = std::atoi(ini_data["general"]["version_minor"].c_str());

        if(version_minor > 0) {
            changed_data = true;
        }

        if(changed_data) {
            ini_data.clear();

            ini_data["general"];
            ini_data["general"]["version_major"] = "0";
            ini_data["general"]["version_minor"] = "0";
        }

        if(!ini_data["general"].has("renderer") && !isIdentifier(ini_data["general"]["renderer"])) {
            ini_data["general"]["renderer"] = prefered_identifier;
            changed_data = true;
        }
        identifier = ini_data["general"]["renderer"];
    }

    Environment *graphics_environment_p = nullptr;

    if( identifier.compare( SDL2_WITH_GLES_2 ) == 0 )
        graphics_environment_p = new SDL2::GLES2::Environment();

    if(changed_data)
        ini_file.write(ini_data, true);

    return graphics_environment_p;
}

int Environment::initSystem( const std::string &identifier ) {
    if( identifier.compare( SDL2_WITH_GLES_2 ) == 0 ) {
        return SDL2::GLES2::Environment::initSystem();
    }
    else
        return -1;
}

int Environment::deinitEntireSystem( const std::string &identifier ) {
    if( identifier.compare( SDL2_WITH_GLES_2 ) == 0 ) {
        return SDL2::GLES2::Environment::deinitEntireSystem();
    }
    else
        return -1;
}

}
