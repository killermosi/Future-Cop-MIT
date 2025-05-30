#include "Environment.h"

#include <mini/ini.h>

#include "../../Data/Mission/MSICResource.h"
#include "../../Data/Mission/TOSResource.h"
#include "../../Data/Mission/SNDSResource.h"

#include <cassert>

namespace Sound {
namespace OpenAL {

Environment::Environment() : alc_device_p(nullptr), alc_context_p(nullptr), listener_both(Listener::WhichEar::BOTH), sound_queue(1.0f), music_buffer(0), music_source(0), music_gain(1.0f), master_gain(1.0f) {}

Environment::~Environment() {
    alcMakeContextCurrent(nullptr);

    if(alc_context_p != nullptr)
        alcDestroyContext(alc_context_p);

    if(alc_device_p != nullptr)
        alcCloseDevice(alc_device_p);
}

int Environment::initSystem() {
    // TODO Add optional AL_SOFT_loop_points support.

    return 1;
}

int Environment::deinitEntireSystem() {
    return 1;
}

uint32_t Environment::getEnvironmentIdentifier() const {
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

    this->AL_FORMAT_MONO_FLOAT32   = AL_NONE;
    this->AL_FORMAT_STEREO_FLOAT32 = AL_NONE;

    if(alIsExtensionPresent("AL_EXT_float32")) {
        this->AL_FORMAT_MONO_FLOAT32   = alGetEnumValue("AL_FORMAT_MONO_FLOAT32");
        this->AL_FORMAT_STEREO_FLOAT32 = alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");

        assert(this->AL_FORMAT_MONO_FLOAT32   == 0x10010);
        assert(this->AL_FORMAT_STEREO_FLOAT32 == 0x10011);
    }

    // NOTE: I do not think they are that important.
    error_state = alGetError();

    const Data::Mission::MSICResource* misc_r = accessor.getConstMSIC(1);

    if(music_source != 0)
        alDeleteSources(1, &music_source);
    error_state = music_buffer.deallocate();

    if(error_state != AL_NO_ERROR)
        return -5;

    if(misc_r != nullptr) {
        const Data::Mission::WAVResource *const sound_r = misc_r->soundAccessor();

        error_state = music_buffer.allocate(*sound_r);

        switch(error_state) {
            case AL_INVALID_ENUM:
                return -6;
            case AL_INVALID_VALUE:
                return -7;
            case AL_OUT_OF_MEMORY:
                return -8;
            default:
                return -9;
            case AL_NO_ERROR:
                ; // Do nothing.
        }
    }

    alGenSources(1,&music_source);

    error_state = alGetError();

    if(error_state != AL_NO_ERROR) {
        return -10;
    }

    alSourcei(music_source, AL_BUFFER, music_buffer.buffer_index);

    alSourcei(music_source, AL_LOOPING, AL_TRUE);

    alSourcei(music_source, AL_SOURCE_RELATIVE, AL_TRUE);

    error_state = alGetError();

    if(error_state != AL_NO_ERROR) {
        return -11;
    }

    sound_queue.reset();

    for(auto key: tos_to_swvr) {
        ALenum current_error_state = key.second.deallocate();

        if(current_error_state != AL_NO_ERROR) {
            error_state = current_error_state;
        }
    }

    if(error_state != AL_NO_ERROR) {
        return -12;
    }

    tos_to_swvr.clear();

    auto tos_resource_r = accessor.getConstTOS( 1 );

    if(tos_resource_r != nullptr) {
        for( const uint32_t tos_offset: tos_resource_r->getOffsets()) {
            const Data::Accessor *swvr_accessor_r = accessor.getSWVRAccessor(tos_offset);

            // assert(swvr_accessor_r != nullptr);

            if(swvr_accessor_r != nullptr) {
                auto snds_r = swvr_accessor_r->getConstSNDS(1);

                if(snds_r == nullptr)
                    continue;

                const Data::Mission::WAVResource *const sound_r = snds_r->soundAccessor();

                tos_to_swvr[tos_offset] = Internal::SoundBuffer();

                ALenum current_error_state = tos_to_swvr[tos_offset].allocate(*sound_r);

                if(current_error_state != AL_NO_ERROR) {
                    error_state = current_error_state;
                }
            }
        }
    }

    if(error_state != AL_NO_ERROR) {
        return -13;
    }

    error_state = alGetError();

    if(error_state != AL_NO_ERROR) {
        return -14;
    }

    sound_queue.setPlayerState(Sound::PlayerState::STOP);

    error_state = alGetError();

    if(error_state != AL_NO_ERROR) {
        return -15;
    }

    error_state = sound_queue.initialize();

    if(error_state != AL_NO_ERROR) {
        return -16;
    }

    for(auto key: id_to_sound) {
        ALenum current_error_state = key.second.deallocate();

        if(current_error_state != AL_NO_ERROR) {
            error_state = current_error_state;
        }
    }

    if(error_state != AL_NO_ERROR) {
        return -17;
    }

    id_to_sound.clear();

    auto sound_resources_r = accessor.getAllConstWAV();

    for(const Data::Mission::WAVResource* sound_r: sound_resources_r) {
        id_to_sound[sound_r->getResourceID()] = Internal::SoundSource();

        ALenum current_error_state = id_to_sound[sound_r->getResourceID()].allocate(*sound_r);

        if(current_error_state != AL_NO_ERROR) {
            error_state = current_error_state;
        }
    }

    if(error_state != AL_NO_ERROR) {
        return -18;
    }

    return 1;
}

#define AUDIO_VOLUME_SETTING(variable, variable_string) \
if(!general.has(variable_string)) { \
    general[variable_string] = std::to_string(1.0); \
    changed_data = true; \
} \
try { \
    variable = std::stof( general[variable_string] ); \
    if( variable > 2.0f ) { \
        variable = 2.0f; \
        general[variable_string] = std::to_string(variable); \
        changed_data = true; \
    } \
    else \
    if( variable < 0.0f ) { \
        variable = 0.0f; \
        general[variable_string] = std::to_string(variable); \
        changed_data = true; \
    } \
} catch( const std::logic_error & logical_error ) { \
    variable = 1.0f; \
    general[variable_string] = std::to_string(variable); \
    changed_data = true; \
}

int Environment::readConfig( std::filesystem::path file ) {
    std::filesystem::path full_file_path = file;

    full_file_path += ".ini";

    mINI::INIFile ini_file( full_file_path );

    mINI::INIStructure ini_data;

    if(!ini_file.read(ini_data))
        return -1;

    bool changed_data = false;

    ALfloat master_volume = 1.0f, announcement_volume = master_volume, music_volume = master_volume, sfx_volume = master_volume, video_volume = master_volume;
    unsigned listener_sound_limit = 32, announcement_queue_limit = 32;

    if(!ini_data.has("general"))
        ini_data["general"];

    {
        auto& general = ini_data["general"];

        AUDIO_VOLUME_SETTING(master_volume,             "master_volume")
        AUDIO_VOLUME_SETTING(announcement_volume, "announcement_volume")
        AUDIO_VOLUME_SETTING(music_volume,               "music_volume")
        AUDIO_VOLUME_SETTING(sfx_volume,                   "sfx_volume")
        AUDIO_VOLUME_SETTING(video_volume,               "video_volume")
    }

    if(!ini_data.has("listener") || !ini_data["listener"].has("sound_limit")) {
        ini_data["listener"]["sound_limit"] = std::to_string(listener_sound_limit);
        changed_data = true;
    }
    try {
        listener_sound_limit = std::stoul( ini_data["listener"]["sound_limit"] );
    } catch( const std::logic_error & logical_error ) {
        ini_data["listener"]["sound_limit"] = std::to_string(listener_sound_limit);
        changed_data = true;
    }


    if(!ini_data.has("announcement") || !ini_data["announcement"].has("queue_limit")) {
        ini_data["announcement"]["queue_limit"] = std::to_string(announcement_queue_limit);
        changed_data = true;
    }
    try {
        announcement_queue_limit = std::stoul( ini_data["announcement"]["queue_limit"] );
    } catch( const std::logic_error & logical_error ) {
        ini_data["announcement"]["queue_limit"] = std::to_string(announcement_queue_limit);
        changed_data = true;
    }

    if(changed_data || !std::filesystem::exists(full_file_path)) {
        ini_file.write(ini_data, true); // Pretty print
    }

    master_gain = master_volume;

    alListenerf(AL_GAIN, master_gain);

    sound_queue.queue_limit = announcement_queue_limit;
    sound_queue.setGain(announcement_volume);

    music_gain = music_volume;

    this->stream_gain = video_volume;

    alSourcef(music_source, AL_GAIN, music_gain);

    listener_both.setGain(sfx_volume);
    listener_both.source_max_length = listener_sound_limit;

    return 1;
}

bool Environment::setMusicState(Sound::PlayerState player_state) {
    if(music_source == 0)
        return false;

    switch(player_state) {
        case Sound::PlayerState::STOP:
            alSourceStop(music_source);
            break;
        case Sound::PlayerState::PAUSE:
            alSourcePause(music_source);
            break;
        case Sound::PlayerState::PLAY:
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

Sound::PlayerState Environment::getMusicState() const {
    if(music_source == 0)
        return Sound::PlayerState::STOP;

    ALint state = -1;

    alGetSourcei(music_source, AL_SOURCE_STATE, &state);

    switch(state) {
        case AL_STOPPED:
            return Sound::PlayerState::STOP;
            break;
        case AL_PAUSED:
            return Sound::PlayerState::PAUSE;
            break;
        case AL_PLAYING:
            return Sound::PlayerState::PLAY;
            break;
        default:
            return Sound::PlayerState::STOP;
    }
}

bool Environment::queueTrack(uint32_t track_offset) {
    auto find = tos_to_swvr.find(track_offset);

    if(find == tos_to_swvr.end()) {
        return false;
    }
    else {
        Internal::SoundBuffer sound_buffer = (*find).second;

        sound_queue.push( sound_buffer );
        return true;
    }
}

bool Environment::setTrackPlayerState(PlayerState player_state) {
    sound_queue.setPlayerState(player_state);

    return true;
}

Sound::PlayerState Environment::getTrackPlayerState() const {
    return sound_queue.getPlayerState();
}

Sound::Listener* Environment::getListenerReference(Listener::WhichEar listener_type) {
    switch(listener_type) {
        case Listener::WhichEar::BOTH:
            return &listener_both;
        default:
            return nullptr;
    }
}

void Environment::advanceTime(std::chrono::high_resolution_clock::duration duration) {
    if(listener_both.getEnabled())
        listener_both.process(duration);

    sound_queue.update(duration);
}

}
}
