#include "Environment.h"
#include "../../../Data/Mission/MSICResource.h"

namespace Sounds {
namespace SDL2 {
namespace MojoAL {

ALenum Environment::getFormat(unsigned int number_of_channels, unsigned int bits_per_sample) {
    if(number_of_channels == 1) {
        if(bits_per_sample == 8)
            return AL_FORMAT_MONO8;
        else if(bits_per_sample == 16)
            return AL_FORMAT_MONO16;
        else
            return AL_INVALID_ENUM;
    }
    else if(number_of_channels == 2) {
        if(bits_per_sample == 8)
            return AL_FORMAT_STEREO8;
        else if(bits_per_sample == 16)
            return AL_FORMAT_STEREO16;
        else
            return AL_INVALID_ENUM;
    }
    else
            return AL_INVALID_ENUM;

}

Environment::Environment() : alc_device_p(nullptr), alc_context_p(nullptr), music_buffer(0), music_source(0) {}

Environment::~Environment() {
    alcMakeContextCurrent(nullptr);

    if(alc_context_p != nullptr)
        alcDestroyContext(alc_context_p);

    if(alc_device_p != nullptr)
        alcCloseDevice(alc_device_p);
}

int Environment::initSystem() {
    return 1;
}

int Environment::deinitEntireSystem() {
    return 1;
}

std::string Environment::getEnvironmentIdentifier() const {
    return SDL2_WITH_MOJO_AL;
}

int Environment::loadResources( const Data::Accessor &accessor ) {
    if(alc_device_p == nullptr) {
        alc_device_p = alcOpenDevice(nullptr);

        if(alc_device_p == nullptr)
            return -2;
    }

    if(alc_context_p == nullptr) {
        alc_context_p = alcCreateContext(alc_device_p, nullptr);

        if(alc_context_p == nullptr) {
            return -3;
        }
    }

    if(alcMakeContextCurrent(alc_context_p) == ALC_FALSE)
        return -4;

    ALenum error_state = alGetError();

    const Data::Mission::MSICResource* misc_r = accessor.getSWVRAccessor().getConstMSIC();

    if(music_source != 0)
        alDeleteSources(1, &music_source);
    if(music_buffer != 0)
        alDeleteBuffers(1, &music_buffer);

    if(misc_r != nullptr) {
        const Data::Mission::WAVResource *const sound_r = misc_r->soundAccessor();

        ALenum format = getFormat(sound_r->getChannelNumber(), sound_r->getBitsPerSample());

        if(format == AL_INVALID_ENUM)
            return -5;

        alGenBuffers(1, &music_buffer);

        error_state = alGetError();

        if(error_state != AL_NO_ERROR) {
            if(error_state != AL_INVALID_VALUE)
                return -6;
            else if(error_state != AL_OUT_OF_MEMORY)
                return -7;
            else
                return -8;
        }

        alBufferData(music_buffer, format, sound_r->getPCMData(), sound_r->getTotalPCMBytes(), sound_r->getSampleRate());
    }

    alGenSources(1,&music_source);

    error_state = alGetError();

    if(error_state != AL_NO_ERROR) {
        return -9;
    }

    alSourcei(music_source, AL_BUFFER, music_buffer);

    alSourcei(music_source, AL_LOOPING, AL_TRUE);

    return 1;
}

bool Environment::setMusicState(Sounds::PlayerState player_state) {
    if(music_source == 0)
        return false;

    switch(player_state) {
        case Sounds::PlayerState::STOP:
            alSourceStop(music_source);
            break;
        case Sounds::PlayerState::PAUSE:
            alSourcePause(music_source);
            break;
        case Sounds::PlayerState::PLAY:
            alSourcePlay(music_source);
            break;
        default:
            return false;
    }

    ALenum error_state = alGetError();

    if(error_state == AL_NO_ERROR)
        return true;
    else
        return false;
}

Sounds::PlayerState Environment::getMusicState() const {
    if(music_source == 0)
        return Sounds::PlayerState::STOP;

    ALint state = -1;

    alGetSourcei(music_source, AL_SOURCE_STATE, &state);

    switch(state) {
        case AL_STOPPED:
            return Sounds::PlayerState::STOP;
            break;
        case AL_PAUSED:
            return Sounds::PlayerState::PAUSE;
            break;
        case AL_PLAYING:
            return Sounds::PlayerState::PLAY;
            break;
        default:
            return Sounds::PlayerState::STOP;
    }
}

bool Environment::queueTrack(uint32_t track_offset) {
    return false;
}

void Environment::clearTrackQueue() {
    // There is no queue.
}

bool Environment::setTrackPlayerState(PlayerState player_state) {
    return false;
}

Sounds::PlayerState Environment::getTrackPlayerState() const {
    return Sounds::PlayerState::STOP;
}

}
}
}
