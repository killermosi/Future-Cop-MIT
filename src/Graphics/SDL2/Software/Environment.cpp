#include "Environment.h"

#include "Window.h"

namespace Graphics::SDL2::Software {

Environment::Environment() {
    this->display_world = false;
}

Environment::~Environment() {
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
    return Graphics::Environment::SDL2_WITH_SOFTWARE;
}

int Environment::loadResources( const Data::Accessor &accessor ) {
    return -1;
}

bool Environment::displayMap( bool state ) {
    display_world = state;
    return false;
}

size_t Environment::getTilAmount() const {
    return 0;
}

int Environment::setTilBlink( unsigned til_index, float seconds ) {
    return -1;
}

int Environment::setTilPolygonBlink( unsigned polygon_type, float rate ) {
    return -1;
}

bool Environment::getBoundingBoxDraw() const {
    return false;
}

void Environment::setBoundingBoxDraw(bool draw) {
}

void Environment::setupFrame() {
}

void Environment::drawFrame() {
    auto window_r = this->window_p;

    auto window_SDL_r = dynamic_cast<Software::Window*>( window_r );

    SDL_UpdateTexture(window_SDL_r->texture_p, nullptr, window_SDL_r->pixel_buffer_p, window_SDL_r->pixel_buffer_pitch);
    SDL_RenderCopy(window_SDL_r->renderer_p, window_SDL_r->texture_p, nullptr, nullptr);
    SDL_RenderPresent(window_SDL_r->renderer_p);
}

bool Environment::screenshot( Utilities::Image2D &image ) const {
    return false;
}

void Environment::advanceTime( float seconds_passed ) {
}

// Declares
Graphics::ANMFrame*         Environment::allocateVideoANM(uint32_t track_offset) { return nullptr; }
Graphics::ExternalImage*    Environment::allocateExternalImage(bool has_alpha) { return nullptr; }
Graphics::Image*            Environment::allocateImage() { return nullptr; }
Graphics::ModelInstance*    Environment::allocateModel( uint32_t obj_identifier, const glm::vec3 &position, const glm::quat &rotation, const glm::vec2 &texture_offset ) { return nullptr; }
Graphics::ParticleInstance* Environment::allocateParticleInstance() { return nullptr; }

bool Environment::doesModelExist(uint32_t obj_resource_id) const {return false;}

}
