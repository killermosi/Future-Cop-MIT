#ifndef MISSION_RESOURCE_SOUND_HEADER
#define MISSION_RESOURCE_SOUND_HEADER

#include "Resource.h"

namespace Data {

namespace Mission {

class WAVResource : public Resource {
public:
    static const std::string FILE_EXTENSION;
    static const uint32_t IDENTIFIER_TAG;

protected:
    unsigned int audio_format; // Should be 1
    unsigned int channel_number; // 1 for mono.
    unsigned int sample_rate; // For example 44100
    unsigned int byte_rate; // sample_rate * channel_number * bits_per_sample / 8
    unsigned int block_align; // channel_number * bits_per_sample / 8
    unsigned int bits_per_sample;

    std::vector<uint8_t> audio_stream;
    // This is the amount of bytes that the wav says. So it can be smaller than the audio_stream.
    unsigned int audio_stream_length;

public:

    WAVResource();
    WAVResource( const WAVResource &obj );

    virtual std::string getFileExtension() const;

    virtual uint32_t getResourceTagID() const;

    void setChannelNumber( int channel_number );
    void setSampleRate( int sample_rate );
    void setBitsPerSample( int bits_per_sample );
    void updateDependices();

    int getChannelNumber() const { return channel_number; }
    int getSampleRate() const { return sample_rate; }
    int getBitsPerSample() const { return bits_per_sample; }
    int getTotalPCMBytes() const { return audio_stream_length; }
    const uint8_t *const getPCMData() const { return audio_stream.data(); }

    /**
     * This adds new data to the back of the member variable audio_stream.
     * @note This function will advance the reader!
     * @param reader This holds the reader to the data.
     * @return If the reader is valid or non-empty this should return true.
     */
    bool addAudioStream( Utilities::Buffer::Reader &reader );

    /**
     * This copies the buffer into the audio_stream. Warning this will clear audio_stream's
     * original content.
     * @note This function will advance the reader! 
     */
    bool setAudioStream( Utilities::Buffer::Reader &reader );

    void updateAudioStreamLength();

    /**
     * This is to be used when the file is finished loading everything into raw_data.
     * Be very sure that everything has been loaded before calling this, otherwise there could be errors.
     * @return True if the WAV header in the data is valid, or else it will return false.
     */
    virtual bool parse( const ParseSettings &settings = Data::Mission::Resource::DEFAULT_PARSE_SETTINGS );

    virtual Resource * duplicate() const;

    virtual int write( const std::string& file_path, const Data::Mission::IFFOptions &iff_options = IFFOptions() ) const;

    /**
     * This would write the audio stream to the filesystem if successful.
     * @note This command is meant to be used by children classes of WAV.
     * @param file_path The path of the file to write.
     * @param is_dry The boolean to prevent the writing.
     * @return 1 if the file is written to the filesystem. 0 if nothing has been written.
     */
    int writeAudio( const std::string& file_path, bool is_dry = false ) const;
};

}

}

#endif // MISSION_RESOURCE_SOUND_HEADER
