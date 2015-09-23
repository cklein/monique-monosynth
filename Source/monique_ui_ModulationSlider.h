/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.2.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_CB44C34F016734A4__
#define __JUCE_HEADER_CB44C34F016734A4__

//[Headers]     -- You can add your own extra header files here --
#include "App_h_includer.h"

// TODO
#define DONT_OVERRIDE_SLIDER_VALUE -99999

class Monique_Ui_DualSlider;
class SnapSlider : public Slider
{
    friend class Monique_Ui_DualSlider;
    Monique_Ui_DualSlider* owner;

    void mouseEnter (const MouseEvent& event) override;
    void mouseExit (const MouseEvent& event) override;
public:
    SnapSlider( const String& name_ ) : Slider( name_ ) { }
};

// THIS SLIDER IS LEFT TO RIGHT IF THE NAME IS "3"
class Left2MiddleSlider : public Slider
{
    Slider*_peer_behind;

    friend class Monique_Ui_DualSlider;
    Monique_Ui_DualSlider* owner;

    void mouseEnter (const MouseEvent& event) override;
    void mouseExit (const MouseEvent& event) override;
public:
    bool hitTest (int x, int) override
    {
        if( getName() == "3" ) {
            if( x >= getWidth()/2 ) {
                _peer_behind->toFront(true);
                _peer_behind->setEnabled(true);
                this->setEnabled(false);
                return false;

            }
        }

        return true;
    }

    void set_peer_behind( Slider*peer_behind_ ) {
        _peer_behind = peer_behind_;
    }

    Left2MiddleSlider(const String& componentName)
        :
        Slider(componentName),
        _peer_behind( nullptr )
    {}
};

// THIS SLIDER IS ALWAYS MIDDLE TO RIGHT
class Middle2RightSlider : public Slider
{
    Slider*_peer_behind;

public:
    bool hitTest (int x, int) override
    {
        if( x < getWidth()/2 ) {
            _peer_behind->toFront(true);
            _peer_behind->setEnabled(true);
            this->setEnabled(false);
            return false;
        }

        return true;
    }

    void set_peer_behind( Slider*peer_behind_ ) {
        _peer_behind = peer_behind_;
    }

    Middle2RightSlider(const String& componentName)
        :
        Slider(componentName),
        _peer_behind( nullptr )
    {}
};

enum 
{
    TOP_BUTTON_IS_ON = -4,
    TOP_BUTTON_IS_OFF = -5,
    NO_TOP_BUTTON_AMP = -3,
    FIXED_TOP_BUTTON_COLOUR = -2,
};

struct ModulationSliderConfigBase
{
    //==============================================================================
    // BASIC SLIDER TYPE
    virtual bool get_is_linear() const noexcept
    {
        return false;
    }

    //==============================================================================
    // FRONT SLIDER
    virtual SLIDER_STYLES get_front_slider_style() const noexcept
    {
        return UNDEFINED_SLIDER_STYLE;
    }
    virtual Parameter* get_front_parameter_base() const noexcept = 0;
    virtual int get_override_front_min_value() const noexcept
    {
        return DONT_OVERRIDE_SLIDER_VALUE;
    }
    virtual int get_override_front_max_value() const noexcept
    {
        return DONT_OVERRIDE_SLIDER_VALUE;
    }

    //==============================================================================
    // BACK SLIDER
    virtual SLIDER_STYLES get_back_slider_style() const noexcept
    {
        return UNDEFINED_SLIDER_STYLE;
    }
    // JUST RETURN THE FRONT PARAM IF YOU LIKT TO SET THE BACK AS MODULATION SLIDER
    virtual Parameter* get_back_parameter_base() const noexcept
    {
        return nullptr;
    }

    //==============================================================================
    // TOP BUTTON
    enum TOP_BUTTON_TYPE
    {
        TOP_BUTTON_IS_MODULATOR,
        TOP_BUTTON_IS_ON_OFF,

        TOP_BUTTON_TYPE_IS_UNKNOWN
    };
    virtual TOP_BUTTON_TYPE get_top_button_type() const noexcept
    {
        return TOP_BUTTON_TYPE_IS_UNKNOWN;
    }
    virtual BoolParameter* get_top_button_parameter_base() const noexcept
    {
        return nullptr;
    }
    virtual StringRef get_top_button_text() const noexcept
    {
        return "";
    }
    virtual float get_top_button_amp() const noexcept
    {
        return NO_TOP_BUTTON_AMP;
    }

    //==============================================================================
    // BOTTOM BUTTON
    virtual StringRef get_bottom_button_text() const noexcept
    {
        return "";
    }
    virtual StringRef get_bottom_button_switch_text() const noexcept
    {
        return "";
    }
    virtual bool get_is_bottom_button_text_dynamic() const noexcept
    {
        return false;
    }

    //==============================================================================
    // CENTER LABEL
    enum SHOW_TYPES
    {
        DEFAULT_SHOW_SLIDER_VAL_ON_CHANGE,
        SHOW_OWN_VALUE
    };
    virtual SHOW_TYPES show_slider_value_on_top_on_change() const noexcept
    {
        return DEFAULT_SHOW_SLIDER_VAL_ON_CHANGE;
    }
    virtual String get_center_value() const noexcept
    {
        return "";
    }
    virtual StringRef get_center_suffix() const noexcept
    {
        return "";
    }
    virtual float get_label_edit_value( float entered_value_ ) const noexcept
    {
        return entered_value_;
    }

    //==============================================================================
    // TOOLTIP
    virtual StringRef get_tootip_front() const noexcept
    {
        return "";
    }
    virtual StringRef get_tootip_top() const noexcept
    {
        return "";
    }
    virtual StringRef get_tootip_bottom() const noexcept
    {
        return "";
    }
    virtual StringRef get_tootip_back() const noexcept
    {
        return "";
    }

protected:
    ModulationSliderConfigBase() {}

public:
    virtual ~ModulationSliderConfigBase() {}
};

class MoniqueSynthData;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class Monique_Ui_DualSlider  : public Component,
    public Monique_Ui_Refreshable,
    public SliderListener,
    public ButtonListener,
    public LabelListener
{
public:
    //==============================================================================
    Monique_Ui_DualSlider (ModulationSliderConfigBase* config_);
    ~Monique_Ui_DualSlider();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    bool is_in_ctrl_view() const;
    void set_ctrl_view_mode( bool mode_ ) const;
    void show_view_mode();
    void update_return_values() noexcept;
    
    Parameter* get_front_parameter() noexcept { return front_parameter; }

    const float original_w;
    const float original_h;
private:
    ModulationSliderConfigBase*const _config;
    MoniqueSynthData* synth_data;

    Parameter* front_parameter;
    Parameter* modulation_parameter;
    Parameter* back_parameter;
    ModulationSliderConfigBase::TOP_BUTTON_TYPE top_button_type;
    BoolParameter* top_parameter;

    bool runtime_show_value_popup;
    bool last_runtime_show_value_popup;
    ModulationSliderConfigBase::SHOW_TYPES show_value_popup_type;
    float last_painted_value_slider_val;
    float last_painted_mod_slider_val;

    void refresh() noexcept;
    void sliderClicked (Slider*s_) override;

public:
    void sliderValueEnter (Slider*s_);
    void sliderValueExit (Slider*s_);
    void sliderModEnter (Slider*s_);
    void sliderModExit (Slider*s_);

public:
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void sliderValueChanged (Slider* sliderThatWasMoved);
    void buttonClicked (Button* buttonThatWasClicked);
    void labelTextChanged (Label* labelThatHasChanged);



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<SnapSlider> slider_value;
    ScopedPointer<TextButton> button_bottom;
    ScopedPointer<Left2MiddleSlider> slider_modulation;
    ScopedPointer<Label> label;
    ScopedPointer<TextButton> button_top;
    ScopedPointer<Label> label_top;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Monique_Ui_DualSlider)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_CB44C34F016734A4__
