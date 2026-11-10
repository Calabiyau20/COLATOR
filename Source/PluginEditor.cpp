#include "PluginEditor.h"

static const juce::Colour modCols[8] = { cosmos::neon, cosmos::fuchsia, cosmos::electric, cosmos::gold,
                                         cosmos::teal, cosmos::orchid, cosmos::azure, cosmos::jade };

// ============================ ModSlider ============================
void ModSlider::itemDropped (const SourceDetails& d)
{
    hot = false;
    proc.addRoute ((int) d.description, pid, 0.25f);
    if (auto* p = getParentComponent()) p->repaint();
    repaint();
}

void ModSlider::paintOverChildren (juce::Graphics& g)
{
    if (! hot) return;
    g.setColour (cosmos::neon);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 3.0f, 1.5f);
}

// ============================ KnobCell =============================
KnobCell::KnobCell (PercoolatorProcessor& p, const juce::String& id, const juce::String& name, juce::Colour a)
    : slider (p, id), proc (p), pid (id), title (name), accent (a)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 12);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::textBoxTextColourId, cosmos::starlight);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id, slider);
}

void KnobCell::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (11);
    r.removeFromBottom (12);
    slider.setBounds (r);
}

void KnobCell::refresh()
{
    const float off = proc.modOffsetFor (pid);
    const float old = (float) slider.getProperties().getWithDefault ("modOffset", 0.0);
    if (std::abs (off - old) > 0.0005f)
    {
        slider.getProperties().set ("modOffset", off);
        slider.repaint();
    }
    repaint (getLocalBounds().removeFromBottom (12));
}

int KnobCell::badgeAt (juce::Point<int> p) const
{
    for (int i = 0; i < badgeRects.size(); ++i)
        if (badgeRects[i].contains (p)) return badgeRoute[i];
    return -1;
}

void KnobCell::paint (juce::Graphics& g)
{
    g.setColour (cosmos::steel);
    g.setFont (brandfonts::sans (8.5f, true));
    g.drawFittedText (title.toUpperCase(), getLocalBounds().removeFromTop (11), juce::Justification::centred, 1);

    badgeRects.clearQuick(); badgeRoute.clearQuick();
    auto strip = getLocalBounds().removeFromBottom (12).reduced (1, 0);
    const auto routes = proc.getRoutes();
    int x = strip.getX();
    g.setFont (brandfonts::sans (8.0f, true));
    for (int i = 0; i < (int) routes.size(); ++i)
    {
        if (routes[(size_t) i].paramID != pid) continue;
        const juce::String txt = "R" + juce::String (routes[(size_t) i].mod + 1)
                               + (routes[(size_t) i].depth >= 0 ? "+" : "-")
                               + juce::String (std::abs (juce::roundToInt (routes[(size_t) i].depth * 100.0f)));
        const int w = juce::jmax (22, g.getCurrentFont().getStringWidth (txt) + 4);
        juce::Rectangle<int> br (x, strip.getY(), w, strip.getHeight());
        if (br.getRight() > strip.getRight()) break;
        g.setColour (modCols[routes[(size_t) i].mod % 8]);
        g.drawRect (br, 1);
        g.drawFittedText (txt, br, juce::Justification::centred, 1);
        badgeRects.add (br); badgeRoute.add (i);
        x += w + 2;
    }
}

void KnobCell::mouseDown (const juce::MouseEvent& e)
{
    dragRoute = badgeAt (e.getPosition());
    if (dragRoute >= 0)
    {
        const auto routes = proc.getRoutes();
        if (dragRoute < (int) routes.size()) dragStart = routes[(size_t) dragRoute].depth;
    }
}
void KnobCell::mouseDrag (const juce::MouseEvent& e)
{
    if (dragRoute < 0) return;
    proc.setRouteDepth (dragRoute, dragStart + (float) -e.getDistanceFromDragStartY() / 160.0f);
    repaint();
}
void KnobCell::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int idx = badgeAt (e.getPosition());
    if (idx >= 0) { proc.removeRoute (idx); repaint(); }
}

// ============================== Panel ==============================
Panel::Panel (juce::String t, juce::Colour a) : title (std::move (t)), accent (a) {}

void Panel::addCell (KnobCell* c) { cells.add (c); addAndMakeVisible (c); }

void Panel::setPower (PercoolatorProcessor& p, const juce::String& paramID)
{
    power = std::make_unique<juce::TextButton> ("ON");
    power->setClickingTogglesState (true);
    power->setColour (juce::TextButton::textColourOnId, accent);
    addAndMakeVisible (*power);
    powerAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, paramID, *power);
}

void Panel::setCombo (PercoolatorProcessor& p, const juce::String& paramID)
{
    combo = std::make_unique<juce::ComboBox>();
    if (auto* par = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter (paramID)))
        combo->addItemList (par->choices, 1);
    addAndMakeVisible (*combo);
    comboAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, paramID, *combo);
}

void Panel::paint (juce::Graphics& g)
{
    brand::chamferFrame (g, getLocalBounds().toFloat().reduced (1.0f), 9.0f, accent);
    auto head = getLocalBounds().reduced (9, 5).removeFromTop (13);
    g.setColour (accent);
    g.setFont (brandfonts::display (8.5f));
    g.drawFittedText (title.toUpperCase(), head, juce::Justification::centredLeft, 1);
}

void Panel::resized()
{
    auto r = getLocalBounds().reduced (8, 5);
    auto head = r.removeFromTop (13);
    if (power != nullptr) power->setBounds (head.removeFromRight (34).withSizeKeepingCentre (32, 13));
    if (combo != nullptr) combo->setBounds (head.removeFromRight (96).withSizeKeepingCentre (94, 15));
    r.removeFromTop (4);

    juce::FlexBox fb;
    fb.flexWrap = juce::FlexBox::Wrap::wrap;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    fb.alignContent = juce::FlexBox::AlignContent::flexStart;
    for (auto* c : cells)
        fb.items.add (juce::FlexItem (*c).withWidth (66.0f).withHeight (74.0f).withMargin (1.0f));
    fb.performLayout (r);
}

// ============================= ModChip =============================
ModChip::ModChip (PercoolatorProcessor& p, int i, juce::Colour a) : proc (p), index (i), accent (a)
{
    bip.setClickingTogglesState (true);
    hold.setClickingTogglesState (true);
    bip.setColour (juce::TextButton::textColourOnId, accent);
    hold.setColour (juce::TextButton::textColourOnId, accent);
    bip.setToggleState (proc.modBipolar[(size_t) index].load(), juce::dontSendNotification);
    hold.setToggleState (proc.modHold[(size_t) index].load(), juce::dontSendNotification);
    bip.onClick  = [this] { proc.modBipolar[(size_t) index].store (bip.getToggleState()); };
    hold.onClick = [this] { proc.modHold[(size_t) index].store (hold.getToggleState()); };
    addAndMakeVisible (bip);
    addAndMakeVisible (hold);
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void ModChip::resized()
{
    auto bottom = getLocalBounds().removeFromBottom (13);
    bip.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2).reduced (1, 0));
    hold.setBounds (bottom.reduced (1, 0));
}

void ModChip::refresh()
{
    const float v = proc.getModValue (index);
    if (std::abs (v - shown) > 0.001f) { shown = v; repaint(); }
    bip.setToggleState (proc.modBipolar[(size_t) index].load(), juce::dontSendNotification);
    hold.setToggleState (proc.modHold[(size_t) index].load(), juce::dontSendNotification);
}

void ModChip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().reduced (1);
    auto circle = r.removeFromTop (26).withSizeKeepingCentre (24, 24).toFloat();
    g.setColour (juce::Colours::black);
    g.fillEllipse (circle);
    g.setColour (accent);
    g.drawEllipse (circle, 1.0f);
    g.setFont (brandfonts::display (8.0f));
    g.drawFittedText ("R" + juce::String (index + 1), circle.toNearestInt(), juce::Justification::centred, 1);

    auto bar = r.removeFromTop (5).reduced (1, 1);
    g.setColour (cosmos::track);
    g.fillRect (bar);
    const bool bi = proc.modBipolar[(size_t) index].load();
    const float t = bi ? (shown * 0.5f + 0.5f) : shown;
    g.setColour (accent);
    g.fillRect (juce::Rectangle<float> (bar.getX() + juce::jlimit (0.0f, 1.0f, t) * (bar.getWidth() - 2.0f),
                                        (float) bar.getY(), 2.0f, (float) bar.getHeight()));
}

void ModChip::mouseDrag (const juce::MouseEvent&)
{
    if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor (this))
        if (! c->isDragAndDropActive())
            c->startDragging (index, this);
}

// ============================== Scope ==============================
void Scope::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black);
    g.fillRect (r);
    g.setColour (cosmos::line);
    g.drawRect (r, 1.0f);
    g.setOpacity (0.5f);
    for (int i = 1; i < 8; ++i)
        g.drawLine (r.getX() + r.getWidth() * (float) i / 8.0f, r.getY(),
                    r.getX() + r.getWidth() * (float) i / 8.0f, r.getBottom(), 0.5f);
    const float mid = r.getCentreY();
    g.drawLine (r.getX(), mid, r.getRight(), mid, 0.5f);
    g.setOpacity (1.0f);

    auto buf = proc.getDisplayBuffer();
    if (buf == nullptr || buf->empty()) return;

    const int W = juce::jmax (2, (int) r.getWidth());
    const int n = (int) buf->size();
    juce::Path p;
    for (int x = 0; x < W; ++x)
    {
        const int i0 = (int) ((juce::int64) x * n / W);
        const int i1 = juce::jmin (n, (int) ((juce::int64) (x + 1) * n / W) + 1);
        float hi = 0.0f;
        for (int i = i0; i < i1; ++i)
            if (std::abs ((*buf)[(size_t) i]) > std::abs (hi)) hi = (*buf)[(size_t) i];
        const float y = mid - hi * (r.getHeight() * 0.5f - 3.0f);
        if (x == 0) p.startNewSubPath (r.getX() + (float) x, y);
        else        p.lineTo (r.getX() + (float) x, y);
    }
    g.setColour (cosmos::neon.withAlpha (0.3f));
    g.strokePath (p, juce::PathStrokeType (2.6f));
    g.setColour (cosmos::neon);
    g.strokePath (p, juce::PathStrokeType (1.1f));

    const float pk = proc.getDisplayPeak();
    g.setColour (cosmos::steel);
    g.setFont (brandfonts::sans (9.0f));
    g.drawFittedText (juce::String (juce::roundToInt (buf->size() * 1000.0 / 48000.0)) + " ms   "
                          + juce::String (pk > 0.0f ? juce::Decibels::gainToDecibels (pk) : -100.0f, 1)
                          + " dB   gr " + juce::String (proc.getDisplayGR(), 1),
                      getLocalBounds().reduced (6, 3).removeFromBottom (12),
                      juce::Justification::centredRight, 1);
}

// ============================== Editor =============================
PercoolatorEditor::PercoolatorEditor (PercoolatorProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    auto makePanel = [this] (juce::OwnedArray<Panel>& into, const juce::String& title,
                             juce::Colour accent, const juce::String& powerID) -> Panel*
    {
        auto* pan = new Panel (title, accent);
        into.add (pan);
        addChildComponent (pan);
        if (powerID.isNotEmpty()) pan->setPower (proc, powerID);
        return pan;
    };
    auto knob = [this] (Panel* pan, const char* id, const char* name, juce::Colour c)
    {
        pan->addCell (new KnobCell (proc, id, name, c));
    };

    // ---- engines ----
    auto* body = makePanel (engines, "01 Body", cosmos::neon, "body_on");
    body->setCombo (proc, "body_wave"); body->setVisible (true);
    knob (body, "body_pitch", "Pitch", cosmos::neon);
    knob (body, "body_bend", "Bend", cosmos::neon);
    knob (body, "body_btime", "Bend T", cosmos::neon);
    knob (body, "body_decay", "Decay", cosmos::neon);
    knob (body, "body_drive", "Drive", cosmos::neon);
    knob (body, "body_level", "Level", cosmos::neon);

    auto* noise = makePanel (engines, "02 Noise", cosmos::fuchsia, "noise_on");
    noise->setCombo (proc, "noise_type"); noise->setVisible (true);
    knob (noise, "noise_color", "Color", cosmos::fuchsia);
    knob (noise, "noise_q", "Reso", cosmos::fuchsia);
    knob (noise, "noise_decay", "Decay", cosmos::fuchsia);
    knob (noise, "noise_shape", "Shape", cosmos::fuchsia);
    knob (noise, "noise_crush", "Crush", cosmos::fuchsia);
    knob (noise, "noise_level", "Level", cosmos::fuchsia);

    auto* snap = makePanel (engines, "03 Snap", cosmos::electric, "snap_on");
    snap->setCombo (proc, "snap_type"); snap->setVisible (true);
    knob (snap, "snap_attack", "Attack", cosmos::electric);
    knob (snap, "snap_tight", "Tight", cosmos::electric);
    knob (snap, "snap_tone", "Tone", cosmos::electric);
    knob (snap, "snap_level", "Level", cosmos::electric);

    // ---- fx pages ----
    auto* trans = makePanel (fxPages, "Transient", cosmos::gold, "trans_on");
    knob (trans, "trans_amount", "Amount", cosmos::gold);

    auto* reso = makePanel (fxPages, "Resonator", cosmos::teal, "reso_on");
    reso->setCombo (proc, "reso_mode");
    knob (reso, "reso_tune", "Tune", cosmos::teal);
    knob (reso, "reso_fb", "Feedback", cosmos::teal);
    knob (reso, "reso_damp", "Damping", cosmos::teal);
    knob (reso, "reso_mix", "Mix", cosmos::teal);

    auto* space = makePanel (fxPages, "Space", cosmos::orchid, "space_on");
    knob (space, "space_dtime", "Dly Time", cosmos::orchid);
    knob (space, "space_dfb", "Dly Fdbk", cosmos::orchid);
    knob (space, "space_dmix", "Dly Mix", cosmos::orchid);
    knob (space, "space_size", "Size", cosmos::orchid);
    knob (space, "space_decay", "Rev Dcy", cosmos::orchid);
    knob (space, "space_rmix", "Rev Mix", cosmos::orchid);

    auto* diode = makePanel (fxPages, "Diode Clip", cosmos::fuchsia, "diode_on");
    knob (diode, "diode_drive", "Drive", cosmos::fuchsia);
    knob (diode, "diode_thresh", "Thresh", cosmos::fuchsia);
    knob (diode, "diode_asym", "Asym", cosmos::fuchsia);
    knob (diode, "diode_tone", "Tilt", cosmos::fuchsia);

    auto* vca = makePanel (fxPages, "VCA Parallel", cosmos::azure, "vca_on");
    knob (vca, "vca_thresh", "Thresh", cosmos::azure);
    knob (vca, "vca_ratio", "Ratio", cosmos::azure);
    knob (vca, "vca_attack", "Attack", cosmos::azure);
    knob (vca, "vca_release", "Release", cosmos::azure);
    knob (vca, "vca_makeup", "Makeup", cosmos::azure);
    knob (vca, "vca_blend", "Blend", cosmos::azure);

    auto* mixer = makePanel (fxPages, "Mixer", cosmos::jade, "mixer_on");
    mixer->setCombo (proc, "mixer_inv");
    knob (mixer, "mixer_wet", "Processed", cosmos::jade);
    knob (mixer, "mixer_dry", "Original", cosmos::jade);

    for (int i = 0; i < fxPages.size(); ++i)
    {
        auto* t = new juce::TextButton (fxPages[i]->title);
        t->setClickingTogglesState (false);
        t->setColour (juce::TextButton::textColourOnId, fxPages[i]->accent);
        t->onClick = [this, i] { showFxPage (i); };
        fxTabs.add (t);
        addAndMakeVisible (t);
    }

    // ---- random bank ----
    for (int i = 0; i < 8; ++i)
    {
        auto* chip = new ModChip (proc, i, modCols[i]);
        chips.add (chip);
        addAndMakeVisible (chip);
    }

    addAndMakeVisible (scope);

    // ---- transport & presets ----
    presetBox.setTextWhenNothingSelected ("Preset");
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0 && idx != proc.getPresetIndex()) proc.loadPreset (idx);
    };
    addAndMakeVisible (presetBox);
    rebuildPresetList();

    prevPreset.onClick = [this] { proc.stepPreset (-1); rebuildPresetList(); };
    nextPreset.onClick = [this] { proc.stepPreset (1);  rebuildPresetList(); };
    addAndMakeVisible (prevPreset);
    addAndMakeVisible (nextPreset);

    savePreset.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Save patch",
                      PercoolatorProcessor::userPresetFolder().getChildFile ("My Patch.percpreset"),
                      "*.percpreset");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode
                            | juce::FileBrowserComponent::warnAboutOverwriting,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f == juce::File()) return;
                if (! f.hasFileExtension ("percpreset")) f = f.withFileExtension ("percpreset");
                status = proc.savePresetFile (f) ? "Saved " + f.getFileNameWithoutExtension()
                                                 : "Could not write that file";
                statusTicks = 45;
                rebuildPresetList();
            });
    };
    loadPreset.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Load patch",
                      PercoolatorProcessor::userPresetFolder(), "*.percpreset");
        chooser->launchAsync (juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f == juce::File()) return;
                status = proc.loadPresetFile (f) ? "Loaded " + f.getFileNameWithoutExtension()
                                                 : "That file could not be read";
                statusTicks = 45;
                rebuildPresetList();
            });
    };
    addAndMakeVisible (savePreset);
    addAndMakeVisible (loadPreset);

    randomise.onClick = [this]
    {
        proc.randomiseAll (true);
        proc.triggerFromUI();
        status = "Randomized every engine and re-rolled the modulation";
        statusTicks = 45;
    };
    randomise.setColour (juce::TextButton::textColourOffId, cosmos::gold);
    addAndMakeVisible (randomise);

    clearMods.onClick = [this] { proc.clearRoutes(); repaint(); };
    addAndMakeVisible (clearMods);

    trigger.onClick = [this] { proc.triggerFromUI(); };
    trigger.setColour (juce::TextButton::textColourOffId, cosmos::neon);
    addAndMakeVisible (trigger);

    softClip.setClickingTogglesState (true);
    softClip.setColour (juce::TextButton::textColourOnId, cosmos::neon);
    addAndMakeVisible (softClip);
    softAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, "master_soft", softClip);

    masterCell = std::make_unique<KnobCell> (proc, "master_gain", "Master", cosmos::neon);
    velCell    = std::make_unique<KnobCell> (proc, "vel", "Vel", cosmos::gold);
    addAndMakeVisible (*masterCell);
    addAndMakeVisible (*velCell);

    showFxPage (0);
    setResizable (true, true);
    setResizeLimits (820, 470, 1600, 1000);
    setSize (900, 540);
    startTimerHz (15);
}

PercoolatorEditor::~PercoolatorEditor() { setLookAndFeel (nullptr); }

void PercoolatorEditor::rebuildPresetList()
{
    presetBox.clear (juce::dontSendNotification);
    const auto names = proc.getPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetBox.addItem (names[i], i + 1);
    presetBox.setSelectedItemIndex (juce::jlimit (0, juce::jmax (0, names.size() - 1),
                                                 proc.getPresetIndex()),
                                    juce::dontSendNotification);
}

void PercoolatorEditor::showFxPage (int index)
{
    activeFx = juce::jlimit (0, fxPages.size() - 1, index);
    for (int i = 0; i < fxPages.size(); ++i)
    {
        fxPages[i]->setVisible (i == activeFx);
        fxTabs[i]->setToggleState (i == activeFx, juce::dontSendNotification);
    }
    resized();
}

void PercoolatorEditor::timerCallback()
{
    for (auto* p : engines)       for (auto* c : p->cells) c->refresh();
    for (auto* c : fxPages[activeFx]->cells) c->refresh();
    masterCell->refresh(); velCell->refresh();
    for (auto* c : chips) c->refresh();
    scope.repaint();
    if (statusTicks > 0 && --statusTicks == 0) { status.clear(); repaint(); }
}

void PercoolatorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (cosmos::voidTop, 0.0f, 0.0f,
                                             cosmos::voidBot, 0.0f, (float) getHeight(), false));
    g.fillAll();

    auto head = getLocalBounds().reduced (10, 7).removeFromTop (26);
    g.setFont (brandfonts::display (15.0f));
    g.setColour (cosmos::neon);
    g.drawText ("PERCOOLATOR", head.removeFromLeft (152), juce::Justification::centredLeft);

    auto foot = getLocalBounds().reduced (10, 4).removeFromBottom (13);
    g.setFont (brandfonts::sans (9.0f));
    g.setColour (status.isNotEmpty() ? cosmos::neon : cosmos::steel);
    g.drawText (status.isNotEmpty() ? status
                                    : juce::String ("Drag R1-R8 onto any knob  ·  double-click a badge to remove it"),
                foot, juce::Justification::centredLeft);
}

void PercoolatorEditor::resized()
{
    auto r = getLocalBounds().reduced (10, 7);

    // ---- header ----
    auto head = r.removeFromTop (26);
    head.removeFromLeft (156);
    prevPreset.setBounds (head.removeFromLeft (22).reduced (0, 3));
    presetBox .setBounds (head.removeFromLeft (156).reduced (2, 3));
    nextPreset.setBounds (head.removeFromLeft (22).reduced (0, 3));
    head.removeFromLeft (6);
    savePreset.setBounds (head.removeFromLeft (46).reduced (0, 3));
    loadPreset.setBounds (head.removeFromLeft (46).reduced (1, 3));
    head.removeFromLeft (8);
    randomise .setBounds (head.removeFromLeft (84).reduced (0, 3));

    masterCell->setBounds (head.removeFromRight (62).withHeight (26));
    velCell   ->setBounds (head.removeFromRight (62).withHeight (26));
    softClip  .setBounds (head.removeFromRight (44).reduced (2, 4));
    trigger   .setBounds (head.removeFromRight (74).reduced (2, 3));
    r.removeFromTop (5);

    // ---- scope ----
    scope.setBounds (r.removeFromTop (74));
    r.removeFromTop (5);

    // ---- engines ----
    auto eng = r.removeFromTop (146);
    const int ew = eng.getWidth() / 3;
    engines[0]->setBounds (eng.removeFromLeft (ew).reduced (2));
    engines[1]->setBounds (eng.removeFromLeft (ew).reduced (2));
    engines[2]->setBounds (eng.reduced (2));
    r.removeFromTop (3);

    // ---- random bank ----
    auto rand = r.removeFromTop (46);
    clearMods.setBounds (rand.removeFromRight (76).withSizeKeepingCentre (74, 18));
    const int cw = rand.getWidth() / 8;
    for (int i = 0; i < 8; ++i)
        chips[i]->setBounds (rand.removeFromLeft (cw).reduced (3, 1));
    r.removeFromTop (3);

    // ---- fx tabs + page ----
    auto tabs = r.removeFromTop (20);
    const int tw = tabs.getWidth() / juce::jmax (1, fxTabs.size());
    for (auto* t : fxTabs) t->setBounds (tabs.removeFromLeft (tw).reduced (1, 0));
    r.removeFromTop (3);

    auto page = r.removeFromTop (juce::jmax (90, r.getHeight() - 16));
    for (auto* p : fxPages) p->setBounds (page.reduced (2, 0));
}
