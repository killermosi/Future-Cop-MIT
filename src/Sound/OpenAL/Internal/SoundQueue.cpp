#include "SoundQueue.h"

namespace Sound {
namespace OpenAL {
namespace Internal {

SoundQueue::SoundQueue(ALfloat v_gain, unsigned p_queue_limit) : p_gain(v_gain), queue_limit(p_queue_limit), sound_queue(), current_sound_element(), player_state(Sound::PlayerState::STOP), allocated_queue_source(false), queue_source() {
    current_sound_element.buffer_index = 0;
    current_sound_element.duration = std::chrono::high_resolution_clock::duration(0);
}

ALenum SoundQueue::initialize() {
    if(allocated_queue_source)
        return AL_NO_ERROR;

    ALenum error_state;

    alGenSources(1,&queue_source);

    error_state = alGetError();

    if(error_state != AL_NO_ERROR) {
        return error_state;
    }

    alSourcei(queue_source, AL_SOURCE_RELATIVE, AL_TRUE);

    error_state = alGetError();

    if(error_state != AL_NO_ERROR) {
        return error_state;
    }

    allocated_queue_source = true;

    alSourcef(queue_source, AL_GAIN, p_gain);

    error_state = alGetError();

    return error_state;
}

ALenum SoundQueue::reset() {
    if(!allocated_queue_source)
        return AL_NO_ERROR;

    ALenum error_state = alGetError();

    alSourceStop(queue_source);

    error_state = alGetError();

    alSourcei(queue_source, AL_BUFFER, 0);

    error_state = alGetError();

    while(!sound_queue.empty())
        sound_queue.pop();

    current_sound_element.buffer_index = 0;
    current_sound_element.duration = std::chrono::high_resolution_clock::duration(0);

    return error_state;
}

void SoundQueue::push(SoundBuffer new_sound) {
    if(sound_queue.size() > queue_limit)
        sound_queue.pop();

    sound_queue.push(new_sound);
}

void SoundQueue::setPlayerState(Sound::PlayerState player_state) {
    if(!allocated_queue_source)
        return;

    switch(player_state) {
        case Sound::PlayerState::STOP:
            {
                alSourceStop(queue_source);

                while(!sound_queue.empty())
                    sound_queue.pop();

                current_sound_element.buffer_index = 0;
                current_sound_element.duration = std::chrono::high_resolution_clock::duration(0);
            }
            break;
        case Sound::PlayerState::PAUSE:
            {
                if(this->player_state != player_state)
                    alSourcePause(queue_source);
            }
            break;
        case Sound::PlayerState::PLAY:
            {
                if(this->player_state != player_state)
                    alSourcePlay(queue_source);
            }
            break;
    }

    this->player_state = player_state;
}

void SoundQueue::update(std::chrono::high_resolution_clock::duration duration) {
    alGetError();

    // There is nothing to update.
    if(duration.count() == 0 || player_state != Sound::PlayerState::PLAY)
        return;
    else if(duration.count() >= current_sound_element.duration.count()) {
        if(!sound_queue.empty()) {
            current_sound_element = sound_queue.front();
            sound_queue.pop();

            alSourceStop(queue_source);
            alSourcei(queue_source, AL_BUFFER, current_sound_element.buffer_index);
            alSourcePlay(queue_source);
        }
        else {
            current_sound_element.buffer_index = 0;
            current_sound_element.duration = std::chrono::high_resolution_clock::duration(0);
        }
    }
    else
        current_sound_element.duration -= duration;
}

ALenum SoundQueue::setGain(ALfloat v_gain) {
    alGetError(); // Clear AL error for accurate error checking.

    p_gain = v_gain; // Set the gain value in this class.

    if(!allocated_queue_source)
        return AL_NO_ERROR;

    alSourcef(queue_source, AL_GAIN, p_gain); // Set the SoundQueue volume.

    return alGetError(); // Return with the potential error code.
}

}
}
}
