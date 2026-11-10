#pragma once
#include <JuceHeader.h>

// Neon Void — the Synthgularity skin: black void, panel #0a0a10,
// neon filet, starlight text, chamfered chassis with bolts, drop knob.
namespace cosmos
{
    const juce::Colour voidTop   { 0xff000000 };
    const juce::Colour voidBot   { 0xff05020c };
    const juce::Colour panel     { 0xff0a0a10 };
    const juce::Colour panelHi   { 0xff0d0d15 };
    const juce::Colour track     { 0xff1a1626 };
    const juce::Colour line      { 0xff241f33 };
    const juce::Colour starlight { 0xfffaf1e0 };
    const juce::Colour steel     { 0xff808d9d };
    const juce::Colour neon      { 0xff11e98a };
    const juce::Colour jade      { 0xff61d693 };
    const juce::Colour fuchsia   { 0xffd613d0 };
    const juce::Colour electric  { 0xff902bea };
    const juce::Colour orchid    { 0xffc560ef };
    const juce::Colour teal      { 0xff149ba7 };
    const juce::Colour gold      { 0xfff5be3e };
    const juce::Colour azure     { 0xff345fc2 };
}

namespace brandfonts
{
    juce::Font display (float height);
    juce::Font sans (float height, bool bold = false);
}

namespace brand
{
    // Chamfered chassis with inner filet and bolts. Returns the inner area.
    juce::Rectangle<float> chamferFrame (juce::Graphics&, juce::Rectangle<float> r,
                                         float cut, juce::Colour accent, bool bolts = true);
}

class NeonVoidLNF : public juce::LookAndFeel_V4
{
public:
    NeonVoidLNF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawPopupMenuBackground (juce::Graphics&, int, int) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};
