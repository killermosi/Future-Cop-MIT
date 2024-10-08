#include "Text2DBuffer.h"

#include "Environment.h"
#include "SDL2/GLES2/Text2DBuffer.h"

Graphics::Text2DBuffer::Text2DBuffer() : start(std::numeric_limits<float>::max()), end(-std::numeric_limits<float>::max()) {
}

Graphics::Text2DBuffer* Graphics::Text2DBuffer::alloc( Graphics::Environment &env_r ) {
    if( env_r.getEnvironmentIdentifier().compare( Environment::SDL2_WITH_GLES_2 ) == 0 ) {
        return new Graphics::SDL2::GLES2::Text2DBuffer( env_r );
    }
    else
        return nullptr;
}

Graphics::Text2DBuffer::~Text2DBuffer() {
}

int Graphics::Text2DBuffer::loadFonts( Graphics::Environment &env_r, const Data::Accessor &accessor ) {
    if( env_r.getEnvironmentIdentifier().compare( Environment::SDL2_WITH_GLES_2 ) == 0 ) {
        return Graphics::SDL2::GLES2::Text2DBuffer::loadFonts( env_r, accessor );
    }
    else
        return -1;
}
