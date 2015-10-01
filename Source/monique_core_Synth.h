#ifndef MONOSYNTH___H_INCLUDED
#define MONOSYNTH___H_INCLUDED

#include "App_h_includer.h"

//==============================================================================
//==============================================================================
//==============================================================================
class MoniqueSynthesiserSound : public SynthesiserSound
{
    bool appliesToNote (int) override;
    bool appliesToChannel (int) override;

public:
    COLD MoniqueSynthesiserSound() noexcept;
    COLD ~MoniqueSynthesiserSound() noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoniqueSynthesiserSound)
};

//==============================================================================
//==============================================================================
//==============================================================================
class MoniqueAudioProcessor;

class MoniqueSynthData;
class RuntimeInfo;
class LFO;
class OSC;
class ENV;
class FilterProcessor;
class EQProcessor;
class FXProcessor;
class DataBuffer;
class ArpSequencer;

class MoniqueSynthesiserVoice : public SynthesiserVoice
{
    //==============================================================================
    MoniqueAudioProcessor*const audio_processor;
    MoniqueSynthData*const synth_data;

    //==============================================================================
    RuntimeInfo*const info;
    DataBuffer*const data_buffer;

    friend class mono_ParameterOwnerStore;
    ArpSequencer*const arp_sequencer;
    EQProcessor*const eq_processor_l;
    EQProcessor*const eq_processor_r;
    FXProcessor*const fx_processor;

    //==============================================================================
    OSC** oscs;
    LFO** lfos;
    FilterProcessor** filter_processors;

    //==============================================================================
    int current_note;
    float pitch_offset;
    bool is_sostenuto_pedal_down;
    bool stopped_and_sostenuto_pedal_was_down;
    bool is_soft_pedal_down;
    bool was_soft_pedal_down_on_note_start;
    bool is_sustain_pedal_down;
    bool stopped_and_sustain_pedal_was_down;
    float current_velocity;
    int current_step;
    bool an_arp_note_is_already_running;
    int sample_position_for_restart_arp;
    struct ArpInfo
    {
        int current_note;
        float current_velocity;
    } arp_info;

    //==============================================================================
    bool canPlaySound (SynthesiserSound*) override {
        return true;
    }
    void startNote(int midiNoteNumber, float velocity, SynthesiserSound*, int /*currentPitchWheelPosition*/) override;
    void start_internal( int midiNoteNumber, float velocity ) noexcept;
    void stopNote(float, bool allowTailOff) override;
public:
    void stop_arp() noexcept;
    void restart_arp( int sample_pos_in_buffer_ ) noexcept;
private:
    void stop_internal() noexcept;
    void release_if_inactive() noexcept;

    void renderNextBlock( AudioSampleBuffer&, int startSample, int numSamples) override;
    void render_block( AudioSampleBuffer&, int step_number_, int startSample, int numSamples) noexcept;

    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override { } // see synthesizer
public:
    //==============================================================================
    int get_current_note() const noexcept;
    float get_current_velocity() const noexcept;
    void reset() noexcept;
    void reset_internal() noexcept;
    void handle_sustain_pedal( bool down_ ) noexcept;
    void handle_sostueno_pedal( bool down_ ) noexcept;
    void handle_soft_pedal( bool down_ ) noexcept;

public:
    //==============================================================================
    // UI INFOS
    float get_filter_env_amp( int filter_id_ ) const noexcept;
    float get_lfo_amp( int lfo_id_ ) const noexcept;
    float get_arp_sequence_amp( int step_ ) const noexcept;
    float get_current_frequency() const noexcept;
    float get_flt_input_env_amp( int flt_id_, int input_id_ ) const noexcept;
    float get_band_env_amp( int band_id_ ) const noexcept;
    float get_chorus_modulation_env_amp() const noexcept;

public:
    //==============================================================================
    COLD MoniqueSynthesiserVoice( MoniqueAudioProcessor*const audio_processor_, MoniqueSynthData*const synth_data_ ) noexcept;
    COLD ~MoniqueSynthesiserVoice() noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoniqueSynthesiserVoice)
};

//==================================================================================
inline int MoniqueSynthesiserVoice::get_current_note() const noexcept
{
    return current_note;
}
inline float MoniqueSynthesiserVoice::get_current_velocity() const noexcept
{
    return current_velocity;
}


//==============================================================================
//==============================================================================
//==============================================================================
class MoniqueSynthesizer : public Synthesiser
{
    MoniqueSynthesiserVoice*const voice;

    void handleSustainPedal (int midiChannel, bool isDown) override;
    void handleSostenutoPedal (int midiChannel, bool isDown) override;
    void handleSoftPedal (int midiChannel, bool isDown) override;
    
    void handleBankSelect (int controllerValue) noexcept;
    void handleProgramChange (int midiChannel, int programNumber) override;
    void handleController (int midiChannel, int controllerNumber, int controllerValue) override;    
public:
    COLD SynthesiserVoice* addVoice( SynthesiserVoice* newVoice ) noexcept;
    COLD SynthesiserSound* addSound( const SynthesiserSound::Ptr& sound_ ) noexcept;

public:
COLD MoniqueSynthesizer( MoniqueSynthesiserVoice*voice_, const SynthesiserSound::Ptr& sound_ ) noexcept :
    voice( voice_ )
    {
        Synthesiser::addVoice(voice_);
        Synthesiser::addSound(sound_);
    }
    COLD ~MoniqueSynthesizer() noexcept {}
};

#endif

