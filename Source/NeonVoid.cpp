#include "NeonVoid.h"

namespace brandfonts
{
   #if JUCE_MAJOR_VERSION >= 8
    static juce::Font make (const juce::String& name, float h, int style)
    {
        juce::Font f (juce::FontOptions (name, h, style));
        if (f.getTypefaceName() != name)
            f = juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), h, style));
        return f;
    }
   #else
    static juce::Font make (const juce::String& name, float h, int style)
    {
        juce::Font f (name, h, style);
        if (f.getTypefaceName() != name)
            f = juce::Font (juce::Font::getDefaultSansSerifFontName(), h, style);
        return f;
    }
   #endif

    juce::Font display (float h)
    {
        return make ("Syncopate", h, juce::Font::bold).withExtraKerningFactor (0.08f);
    }
    juce::Font sans (float h, bool bold)
    {
        return make ("Montserrat", h, bold ? juce::Font::bold : juce::Font::plain);
    }
}

namespace brand
{
    static juce::Path chamferPath (juce::Rectangle<float> r, float cut)
    {
        juce::Path p;
        p.startNewSubPath (r.getX() + cut, r.getY());
        p.lineTo (r.getRight(), r.getY());
        p.lineTo (r.getRight(), r.getBottom() - cut);
        p.lineTo (r.getRight() - cut, r.getBottom());
        p.lineTo (r.getX(), r.getBottom());
        p.lineTo (r.getX(), r.getY() + cut);
        p.closeSubPath();
        return p;
    }

    juce::Rectangle<float> chamferFrame (juce::Graphics& g, juce::Rectangle<float> r,
                                         float cut, juce::Colour accent, bool bolts)
    {
        auto outer = chamferPath (r, cut);
        g.setColour (cosmos::panel);
        g.fillPath (outer);
        g.setColour (accent.withAlpha (0.85f));
        g.strokePath (outer, juce::PathStrokeType (1.0f));
        auto inner = r.reduced (4.0f);
        g.setColour (accent.withAlpha (0.18f));
        g.strokePath (chamferPath (inner, cut * 0.7f), juce::PathStrokeType (1.0f));
        if (bolts)
        {
            g.setColour (cosmos::steel.withAlpha (0.5f));
            const float d = 2.6f;
            g.fillEllipse (r.getX() + 9.0f, r.getY() + 6.0f, d, d);
            g.fillEllipse (r.getRight() - 11.0f, r.getY() + 9.0f, d, d);
            g.fillEllipse (r.getX() + 6.0f, r.getBottom() - 11.0f, d, d);
            g.fillEllipse (r.getRight() - 9.0f, r.getBottom() - 9.0f, d, d);
        }
        return r.reduced (10.0f);
    }
}

NeonVoidLNF::NeonVoidLNF()
{
    setColour (juce::ResizableWindow::backgroundColourId, cosmos::voidTop);
    setColour (juce::Label::textColourId, cosmos::starlight);
    setColour (juce::ComboBox::backgroundColourId, cosmos::panelHi);
    setColour (juce::ComboBox::textColourId, cosmos::starlight);
    setColour (juce::ComboBox::outlineColourId, cosmos::line);
    setColour (juce::ComboBox::arrowColourId, cosmos::steel);
    setColour (juce::PopupMenu::backgroundColourId, cosmos::panelHi);
    setColour (juce::PopupMenu::textColourId, cosmos::starlight);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, cosmos::neon.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, cosmos::starlight);
    setColour (juce::TextButton::buttonColourId, cosmos::panelHi);
    setColour (juce::TextButton::textColourOffId, cosmos::steel);
    setColour (juce::TextButton::textColourOnId, cosmos::neon);
    setColour (juce::Slider::rotarySliderFillColourId, cosmos::neon);
}

juce::Font NeonVoidLNF::getLabelFont (juce::Label&)      { return brandfonts::sans (12.0f); }
juce::Font NeonVoidLNF::getComboBoxFont (juce::ComboBox&) { return brandfonts::sans (12.0f); }
juce::Font NeonVoidLNF::getPopupMenuFont()                { return brandfonts::sans (13.0f); }

void NeonVoidLNF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                    float pos, float start, float end, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = start + pos * (end - start);
    const float ringR = r * 0.88f, sw = juce::jmax (2.5f, r * 0.1f);
    auto col = s.findColour (juce::Slider::rotarySliderFillColourId);
    if (col.isTransparent()) col = cosmos::neon;
    const bool on = s.isEnabled();
    const bool hot = s.isMouseOverOrDragging() && on;

    juce::Path track;
    track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, start, end, true);
    g.setColour (cosmos::track);
    g.strokePath (track, juce::PathStrokeType (sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? (start + end) * 0.5f : start;
    if (std::abs (angle - from) > 0.001f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour ((on ? col : cosmos::steel).withAlpha (0.25f));
        g.strokePath (arc, juce::PathStrokeType (sw * 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (on ? col : cosmos::steel);
        g.strokePath (arc, juce::PathStrokeType (sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // modulation span, set by the host component through a property
    const float modOff = (float) s.getProperties().getWithDefault ("modOffset", 0.0);
    if (std::abs (modOff) > 0.0005f)
    {
        const float a2 = start + juce::jlimit (0.0f, 1.0f, pos + modOff) * (end - start);
        juce::Path m;
        m.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, juce::jmin (angle, a2), juce::jmax (angle, a2), true);
        g.setColour (cosmos::starlight);
        g.strokePath (m, juce::PathStrokeType (sw * 0.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (juce::Colour (0xff07070b));
    g.fillEllipse (juce::Rectangle<float> (r * 1.3f, r * 1.3f).withCentre (c));
    g.setColour (col.withAlpha (hot ? 0.9f : 0.35f));
    g.drawEllipse (juce::Rectangle<float> (r * 1.3f, r * 1.3f).withCentre (c), 1.2f);

    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (juce::AffineTransform::rotation (angle, c.x, c.y));
    const float ds = r * 0.62f, dw = ds * 0.56f, dy = c.y - r * 0.08f;
    juce::Path d;
    d.startNewSubPath (c.x, dy - ds / 2.0f);
    d.cubicTo (c.x + dw * 0.25f, dy - ds * 0.2f, c.x + dw * 0.55f, dy + ds * 0.05f, c.x + dw * 0.5f, dy + ds * 0.22f);
    d.addCentredArc (c.x, dy + ds * 0.22f, dw * 0.5f, dw * 0.5f, 0.0f,
                     juce::MathConstants<float>::pi * 0.5f, juce::MathConstants<float>::pi * 1.5f, false);
    d.cubicTo (c.x - dw * 0.55f, dy + ds * 0.05f, c.x - dw * 0.25f, dy - ds * 0.2f, c.x, dy - ds / 2.0f);
    d.closeSubPath();
    if (on)
    {
        g.setColour (col.withAlpha (0.25f + 0.25f * pos));
        g.strokePath (d, juce::PathStrokeType (5.0f));
    }
    g.setColour (on ? col : cosmos::steel);
    g.fillPath (d);
}

void NeonVoidLNF::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (cosmos::panelHi);
    g.fillRect (r);
    g.setColour (box.isMouseOver() ? cosmos::neon : cosmos::line);
    g.drawRect (r, 1.0f);
    juce::Path arrow;
    const float cx = (float) w - 12.0f, cy = (float) h * 0.5f;
    arrow.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour (cosmos::steel);
    g.fillPath (arrow);
}

void NeonVoidLNF::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 0, box.getWidth() - 24, box.getHeight());
    label.setFont (brandfonts::sans (12.0f));
}

void NeonVoidLNF::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                        bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    auto accent = b.findColour (juce::TextButton::textColourOnId);
    g.setColour (on ? accent.withAlpha (0.18f) : cosmos::panelHi);
    g.fillRect (r);
    g.setColour (on || over ? accent : cosmos::line);
    g.drawRect (r, 1.0f);
    if (down) { g.setColour (accent.withAlpha (0.12f)); g.fillRect (r); }
}

void NeonVoidLNF::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (brandfonts::sans (10.5f, true));
    g.setColour (b.getToggleState() ? b.findColour (juce::TextButton::textColourOnId)
                                    : b.findColour (juce::TextButton::textColourOffId));
    g.drawFittedText (b.getButtonText().toUpperCase(), b.getLocalBounds(), juce::Justification::centred, 1);
}

void NeonVoidLNF::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (cosmos::panelHi);
    g.setColour (cosmos::line);
    g.drawRect (0, 0, w, h, 1);
}
