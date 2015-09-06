
#include "PluginProcessor.h"
#include "monique_core_Datastructures.h"
#include "monique_core_Synth.h"

#include "UiEditorSynthLite.h"
#include "monique_ui_SegmentedMeter.h"

// ********************************************************************************************
// ********************************************************************************************
// ********************************************************************************************
enum {
    IO_IS_MIDI = true,
    IO_IS_AUDIO = false,
};
struct IOData {
    // BY FILTER ID
    StringArray filter_curve_record_midi_devices;
    StringArray filter_curve_record_audio_devices;
    Array<int> filter_curve_record_cc_number;
    Array<int> filter_curve_record_audio_port;
    Array<bool> filter_curve_record_is_midi_else_audio;

public:
    IOData() {
        for( int flt_id = 0 ; flt_id != SUM_FILTERS ; ++flt_id )
        {
            filter_curve_record_midi_devices.add( "UNKNOWN" );
            filter_curve_record_audio_devices.add( "UNKNOWN" );
            filter_curve_record_cc_number.add( flt_id*3+82 );
            filter_curve_record_audio_port.add( 2 );
            filter_curve_record_is_midi_else_audio.add( IO_IS_MIDI );
        }
    }
};
// ********************************************************************************************
#define SINGLE_EVENT_INPUT -1
#define NO_NEW_EVENTS -1
class DATAINProcessor
{
    SynthData*const _synth_data;

    CriticalSection lock;

public:
    void processBlock ( MidiBuffer& input_messages_ )
    {
        // todo check for channels
    }
    void handle_note_input ( MidiBuffer& input_messages_ )
    {
    }
    // this can be notes to!
    void handle_cc_input ( MidiBuffer& input_messages_ )
    {
        handle_midi_controller( input_messages_ );
    }
    void handle_midi_clock() {

    }

    void handle_midi_controller( MidiBuffer& input_messages_ )
    {
        const ScopedLock locked(lock);
        Array< Parameter* >& parameters = _synth_data->get_atomateable_parameters();
        MidiBuffer::Iterator message_iter( input_messages_ );
        MidiMessage in_message;
        int sample_position;
        while( message_iter.getNextEvent( in_message, sample_position ) )
        {
            MIDIControlHandler*const midi_learn_handler = MIDIControlHandler::getInstance();
            Parameter* learing_param = midi_learn_handler->is_learning();
            if( learing_param )
            {
                if( midi_learn_handler->handle_incoming_message( in_message ) )
                {
                    // CLEAR SIBLINGS
                    for( int i = 0 ; i != parameters.size() ; ++ i )
                    {
                        Parameter* param = parameters.getUnchecked(i);
                        //bool clear_ctrl_version = !param->midi_control.is_ctrl_version;
                        if( param != learing_param && param->midi_control->get_is_ctrl_version_of_name() != learing_param->get_info().name )
                        {
                            if( param->midi_control->is_listen_to( in_message ) )
                            {
                                param->midi_control->clear();
                            }
                        }
                    }
                }
            }
            else
            {
                for( int i = 0 ; i != parameters.size() ; ++ i )
                {
                    Parameter* param = parameters.getUnchecked(i);
                    bool success = param->midi_control->read_from_if_you_listen( in_message );
                    // TODO max tow listeners per CC
                    //if( success ) // SYSEX is the default empty midi message
                    //    break;
                }
            }
        }
    }

    // TODO reset samplerate!
    MidiBuffer incomingMidi;
    MidiMessageCollector messageCollector;

    DATAINProcessor() : _synth_data(&GET_DATA(synth_data)) { }

    ~DATAINProcessor() {
    }
};

// ********************************************************************************************
// ********************************************************************************************
// ********************************************************************************************
template<typename T, int size>
class CircularBuffer {
    int pos;
    T buffer[size];
    T sum;

public:
    inline void add( T val_ ) noexcept {
        sum-=buffer[pos];
        buffer[pos] = val_;
        sum+=val_;

        ++pos;
        if( pos == size )
            pos = 0;
    }
    inline T get_average() const noexcept {
        return sum / size;
    }

    CircularBuffer() : pos(0), sum(0) {
        for( int i = 0 ; i != size ; ++i )
            buffer[i] = 0;
    }
};
MoniqueAudioProcessor::MoniqueAudioProcessor()
    :
#ifdef IS_STANDALONE
    clock_smoth_buffer( new type_CLOCK_SMOTH_BUFFER ),
#endif
    data_in_processor(nullptr),

    peak_meter(nullptr),
    repaint_peak_meter(false)
{
    std::cout << "MONIQUE: init processor" << std::endl;

    FloatVectorOperations::enableFlushToZeroMode(true);

    {
        AppInstanceStore::getInstance()->audio_processor = this;

        synth = new Synthesiser();
        synth_data = new SynthData(MASTER);

#ifdef IS_PLUGIN
        voice = new MONOVoice(this,synth_data);
        synth->addVoice (voice);
        synth->addSound (new MonoSynthSound());
        data_in_processor = new DATAINProcessor();
#endif

        MidiKeyboardState::addListener(this);

#ifdef IS_STANDALONE
        init_audio();
#endif
    }

    std::cout << "MONIQUE: init midi" << std::endl;
    {
        mono_AudioDeviceManager::read();
        synth_data->read_midi();
    }
}

// ----------------------------------------------------
MoniqueAudioProcessor::~MoniqueAudioProcessor()
{
    trigger_send_clear_feedback();
    stop_midi_devices();

#ifdef IS_STANDALONE
    closeAudioDevice();
    removeAudioCallback (&player);
    player.setProcessor(nullptr);

    synth_data->save_session();
#endif
    mono_AudioDeviceManager::save();
    synth_data->save_midi();

    delete data_in_processor;

    AppInstanceStore::getInstance()->audio_processor = nullptr;

    delete synth;
    delete synth_data;
}

#ifdef IS_STANDALONE
void MoniqueAudioProcessor::init_audio()
{
    std::cout << "MONIQUE: init core" << std::endl;
    {
        voice = new MoniqueSynthesiserVoice(this,synth_data);
        synth->addVoice (voice);
        synth->addSound (new MoniqueSynthesiserSound());
        data_in_processor = new DATAINProcessor();
        synth_data->load_session ();
    }

    std::cout << "MONIQUE: init audio" << std::endl;
    {
        const OwnedArray<AudioIODeviceType>& types = getAvailableDeviceTypes();
#if JUCE_LINUX
        bool is_jack_available = false;
        for( int i = 0 ; i != types.size() ; ++i )
        {
            AudioIODeviceType* type = types[i];
            if( type->getTypeName() == "JACK" )
                is_jack_available = true;
        }
        if( is_jack_available )
            setCurrentAudioDeviceType("JACK",false);
#endif
        setPlayConfigDetails ( 0, 2, 0, 0);

        String error = initialise(0,2, nullptr, false );
        if( error == "" )
        {
            addAudioCallback (&player);
            player.setProcessor (this);
            audio_is_successful_initalized = true;
        }
        else
        {
            std::cout << error << std::endl;
            audio_is_successful_initalized = false;
        }
    }
}
#endif

//==============================================================================
#ifdef IS_STANDALONE
void MoniqueAudioProcessor::handle_extern_midi_start( const MidiMessage& message ) noexcept 
{
    data_in_processor->messageCollector.addMessageToQueue( message );
}
void MoniqueAudioProcessor::handle_extern_midi_stop( const MidiMessage& message) noexcept 
{

    data_in_processor->messageCollector.addMessageToQueue( message );

}
void MoniqueAudioProcessor::handle_extern_midi_continue( const MidiMessage& message ) noexcept 
{

}
void MoniqueAudioProcessor::handle_extern_midi_clock( const MidiMessage& message ) noexcept
{
    if( synth_data->sync ) {
        clock_smoth_buffer->add( message.getTimeStamp()-last_clock_sample );
        last_clock_sample = message.getTimeStamp();

        data_in_processor->messageCollector.addMessageToQueue( message );
    }
}
#endif

//==============================================================================
//==============================================================================
//==============================================================================
void MoniqueAudioProcessor::handle_extern_note_input( const MidiMessage& message ) noexcept 
{
    //MidiKeyboardState::processNextMidiEvent( message );
    //MidiBuffer buffer( message );
    // data_in_processor->handle_note_input( buffer );

    // TODO, TR SENDS NOTES
    data_in_processor->messageCollector.addMessageToQueue( message );
}
void MoniqueAudioProcessor::handle_extern_cc_input( const MidiMessage& message_ ) noexcept 
{
    MidiBuffer buffer( message_ );
    data_in_processor->handle_cc_input( buffer );
}
void MoniqueAudioProcessor::trigger_send_feedback() noexcept
{
    Array< Parameter* >& parameters = synth_data->get_atomateable_parameters();
    for( int i = 0 ; i != parameters.size() ; ++ i )
        parameters.getUnchecked(i)->midi_control->send_feedback_only();
}
void MoniqueAudioProcessor::trigger_send_clear_feedback() noexcept 
{
    Array< Parameter* >& parameters = synth_data->get_atomateable_parameters();
    for( int i = 0 ; i != parameters.size() ; ++ i )
        parameters.getUnchecked(i)->midi_control->send_clear_feedback_only();
}

//==============================================================================
//==============================================================================
//==============================================================================
void MoniqueAudioProcessor::processBlock ( AudioSampleBuffer& buffer_, MidiBuffer& midi_messages_ )
{
    if( sample_rate != getSampleRate() || getBlockSize() != block_size )
        prepareToPlay(getSampleRate(),getBlockSize());

    const int num_samples = buffer_.getNumSamples();
    buffer_.clear();

#ifdef IS_STANDALONE
    static bool is_first_time = true;
    if(is_first_time)
    {
        current_pos_info.resetToDefault();
        is_first_time=false;
    }
    current_pos_info.bpm = synth_data->speed;
    current_pos_info.timeSigNumerator = 4;
    current_pos_info.timeSigDenominator = 4;
    current_pos_info.isPlaying = true;
    current_pos_info.isRecording = false;
    current_pos_info.timeInSamples += buffer_.getNumSamples();
    {
        {
#else // PLUGIN
    if ( getPlayHead() != 0 )
    {
        if( getPlayHead()->getCurrentPosition ( current_pos_info ) )
        {
#endif
            if( current_pos_info.timeInSamples + num_samples >= 0 && current_pos_info.isPlaying )
            {
                data_in_processor->incomingMidi.clear();
                data_in_processor->messageCollector.removeNextBlockOfMessages (data_in_processor->incomingMidi, num_samples);
#ifdef IS_STANDALONE
                MidiBuffer::Iterator message_iter( data_in_processor->incomingMidi );
                MidiMessage extern_midi_message;
                int sample_position;
                RuntimeInfo& runtime_info = GET_DATA( runtime_info );
                runtime_info.next_step_at_sample.clearQuick();
                runtime_info.next_step.clearQuick();
                while( message_iter.getNextEvent( extern_midi_message, sample_position ) )
                {
                    int64 abs_event_time_in_samples = current_pos_info.timeInSamples+sample_position;
                    if( extern_midi_message.isNoteOnOrOff() )
                    {
                        midi_messages_.addEvent( extern_midi_message, sample_position );
                    }
                    else if( extern_midi_message.isMidiStart() )
                    {
                        last_start_sample = abs_event_time_in_samples;
                        runtime_info.clock_counter = 0;
                        runtime_info.is_extern_synced = true;
                        runtime_info.is_running = true;
                    }
                    else if( extern_midi_message.isMidiStop() )
                    {
                        MidiMessage notes_off = MidiMessage::allNotesOff(1);
                        midi_messages_.addEvent( notes_off, sample_position );
                        runtime_info.is_extern_synced = false;
                        runtime_info.is_running = false;
                    }
                    else if( extern_midi_message.isMidiClock() )
                    {
                        // TODO  der sync muss immer gesteted werden!
                        if( synth_data->sync )
                        {
                            synth_data->speed = 60.0/(clock_smoth_buffer->get_average() * 24);
                        }

                        //todo clean up
                        //todo not needed in plugin
                        int factor = 96/16;
                        static int samples_to_count = 96;
                        /*
                                    switch( synth_data->arp_sequencer_data->speed_multi ) {
                                    case _XNORM :
                                        factor = 96/16;
                                        break;
                                    case _X2 :
                                        factor = 96/32;
                                        break;
                                    case _X05 :
                                        factor = 96/8;
                                        break;
                                    case _X4 :
                                        factor = 96/64; // BUG
                                        break;
                                    case _X025 :
                                        factor = 96/4;
                                        break;
                                    default  : //TODO
                                        factor = 96/48;
                                        break;
                                    }
                                    */
                        if( runtime_info.clock_counter%factor == 0 )
                        {
                            if( runtime_info.is_running )
                            {
                                runtime_info.next_step_at_sample.add( sample_position );
                                runtime_info.next_step.add( runtime_info.clock_counter/factor );
                            }
                        }
                        runtime_info.clock_counter++;
                        if( runtime_info.clock_counter == samples_to_count )
                        {
                            runtime_info.clock_counter = 0;

                            samples_to_count = 96;
                            /*
                                        switch( synth_data->arp_sequencer_data->speed_multi )
                                        {
                                        case _XNORM :
                                            samples_to_count = 96;
                                            break;
                                        case _X2 :
                                            samples_to_count = 96/2;
                                            break;
                                        case _X05 :
                                            samples_to_count = 96*2;
                                            break;
                                        case _X4 :
                                            samples_to_count = 96/4;
                                            break;
                                        case _X025 :
                                            samples_to_count = 96*4;
                                            break;
                                        default   : //TODO
                                            samples_to_count = 96/3;
                                            break;
                                        }
                                        */
                        }
                    }
                }
#endif
#ifdef PROFILE
                {
                    static MidiMessage note_on( MidiMessage::noteOn( 1, 64, 1.0f ) );
                    static bool is_first_block = true;
                    if( is_first_block )
                    {
                        is_first_block = false;
                        midi_messages_.addEvent( note_on, 0 );
                    }

                    if( current_pos_info.timeInSamples > 44100 * 100 )
                        exit(0);
                }
#endif
                // MIDI IN
                data_in_processor->processBlock( midi_messages_ );
                data_in_processor->handle_cc_input( midi_messages_ );

                AppInstanceStore::getInstance()->lock_amp_painter();
                {
                    // RENDER SYNTH
                    synth->renderNextBlock ( buffer_, midi_messages_, 0, num_samples );
                    midi_messages_.clear(); // WILL BE FILLED AT THE END

                    // VISUALIZE
                    if( peak_meter )
                        peak_meter->process( buffer_.getReadPointer(0), num_samples );
                }
                AppInstanceStore::getInstance()->unlock_amp_painter();
            }
        }
    }
}

//==============================================================================
//==============================================================================
//==============================================================================
void MoniqueAudioProcessor::prepareToPlay ( double sampleRate, int block_size_ ) {
    // TODO optimize functions without sample rate and block size
    // TODO replace audio sample buffer??
    GET_DATA(data_buffer).resize_buffer_if_required(block_size_);
    data_in_processor->messageCollector.reset(sampleRate);
    synth->setCurrentPlaybackSampleRate (sampleRate);
    RuntimeNotifyer::getInstance()->set_sample_rate( sampleRate );
    RuntimeNotifyer::getInstance()->set_block_size( block_size_ );
}
void MoniqueAudioProcessor::releaseResources() 
{
    // TODO reset all
}
void MoniqueAudioProcessor::reset() 
{
}

//==============================================================================
//==============================================================================
//==============================================================================
int MoniqueAudioProcessor::getNumParameters()
{
    return synth_data->get_atomateable_parameters().size();
}
bool MoniqueAudioProcessor::isParameterAutomatable ( int ) const
{
    return true;
}
float MoniqueAudioProcessor::getParameter( int i_ )
{
    return get_percent_value( synth_data->get_atomateable_parameters().getUnchecked(i_) );
}
const String MoniqueAudioProcessor::getParameterText( int i_ )
{
    return String( get_percent_value( synth_data->get_atomateable_parameters().getUnchecked(i_) ) );
}
String MoniqueAudioProcessor::getParameterLabel (int i_) const {
    return  "N/A";
}
int MoniqueAudioProcessor::getParameterNumSteps( int i_ )
{
    return synth_data->get_atomateable_parameters().getUnchecked(i_)->get_info().num_steps;
}
float MoniqueAudioProcessor::getParameterDefaultValue( int i_ )
{
    return get_percent_default_value( synth_data->get_atomateable_parameters().getUnchecked(i_) );
}
const String MoniqueAudioProcessor::getParameterName( int i_ )
{
    return synth_data->get_atomateable_parameters().getUnchecked(i_)->get_info().short_name;
}
void MoniqueAudioProcessor::setParameter( int i_, float percent_ )
{
    set_percent_value( synth_data->get_atomateable_parameters().getUnchecked(i_), percent_ );
}

//==============================================================================
//==============================================================================
//==============================================================================
void MoniqueAudioProcessor::getStateInformation ( MemoryBlock& destData )
{
    XmlElement xml("PROJECT-1.0");
    synth_data->save_to( &xml );
    copyXmlToBinary ( xml, destData );
}
void MoniqueAudioProcessor::setStateInformation ( const void* data, int sizeInBytes )
{
    ScopedPointer<XmlElement> xml ( getXmlFromBinary ( data, sizeInBytes ) );
    if ( xml )
    {
        if ( xml->hasTagName ( "PROJECT-1.0" ) || xml->hasTagName("MONOLisa")  )
        {
            synth_data->read_from( xml );
        }
    }
}

//==============================================================================
//==============================================================================
//==============================================================================
const String MoniqueAudioProcessor::getName() const
{
#ifdef IS_STANDALONE
    return "";
#else
    return JucePlugin_Name;
#endif
}

//==============================================================================
//==============================================================================
//==============================================================================
const String MoniqueAudioProcessor::getInputChannelName ( int channel_ ) const
{
    return "";
}
const String MoniqueAudioProcessor::getOutputChannelName ( int channel_ ) const
{
    String name;
    switch( channel_ )
    {
    case 0 :
        name = "MONIQUE OUT L";
        break;
    case 1 :
        name = "MONIQUE OUT R";
        break;
    }

    return name;
}

//==============================================================================
//==============================================================================
//==============================================================================
bool MoniqueAudioProcessor::isInputChannelStereoPair ( int ) const
{
    return false;
}
bool MoniqueAudioProcessor::isOutputChannelStereoPair ( int id_ ) const
{
    return true;
}

//==============================================================================
//==============================================================================
//==============================================================================
bool MoniqueAudioProcessor::acceptsMidi() const
{
    return true;
}
bool MoniqueAudioProcessor::producesMidi() const
{
    return true;
}

//==============================================================================
//==============================================================================
//==============================================================================
bool MoniqueAudioProcessor::silenceInProducesSilenceOut() const
{
    return false;
}
double MoniqueAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
//==============================================================================
//==============================================================================
int MoniqueAudioProcessor::getNumPrograms()
{
    int size = 0;
    for( int bank_id = 0 ; bank_id != 4 ; ++bank_id )
        size += synth_data->get_programms( bank_id ).size();

    return size;
}
int MoniqueAudioProcessor::getCurrentProgram()
{
    return synth_data->get_current_programm_id_abs();
}
void MoniqueAudioProcessor::setCurrentProgram ( int id_ )
{
    synth_data->set_current_program_abs(id_);
    synth_data->load();
}
const String MoniqueAudioProcessor::getProgramName ( int id_ )
{
    return synth_data->get_program_name_abs(id_);
}
void MoniqueAudioProcessor::changeProgramName ( int id_, const String& name_ )
{
    synth_data->set_current_program_abs(id_);
    synth_data->rename(name_);
}

//==============================================================================
//==============================================================================
//==============================================================================
bool MoniqueAudioProcessor::hasEditor() const
{
    return true;
}
AudioProcessorEditor* MoniqueAudioProcessor::createEditor()
{
    return new UiEditorSynthLite();
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MoniqueAudioProcessor();
}

