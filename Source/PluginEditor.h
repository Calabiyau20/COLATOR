#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "NeonVoid.h"

class PercoolatorEditor;

// ---- a rotary that accepts random nodes dropped on it ----
class ModSlider : public juce::Slider, public juce::DragAndDropTarget
{
public:
    ModSlider (PercoolatorProcessor& p, juce::String id) : pid (std::move (id)), proc (p) {}
    bool isInterestedInDragSource (const SourceDetails&) override { return true; }
    void itemDragEnter (const SourceDetails&) override { hot = true; repaint(); }
    void itemDragExit (const SourceDetails&) override { hot = false; repaint(); }
    void itemDropped (const SourceDetails&) override;
    void paintOverChildren (juce::Graphics&) override;
    juce::String pid;
private:
    PercoolatorProcessor& proc;
    bool hot = false;
};

// ---- knob + name + value + modulation badges ----
class KnobCell : public juce::Component
{
public:
    KnobCell (PercoolatorProcessor&, const juce::String& pid, const juce::String& name, juce::Colour accent);
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void refresh();
    ModSlider slider;
private:
    int badgeAt (juce::Point<int>) const;
    PercoolatorProcessor& proc;
    juce::String pid, title;
    juce::Colour accent;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    mutable juce::Array<juce::Rectangle<int>> badgeRects;
    mutable juce::Array<int> badgeRoute;
    int dragRoute = -1; float dragStart = 0.0f;
};

// ---- chamfered module frame ----
class Panel : public juce::Component
{
public:
    Panel (juce::String title, juce::Colour accent);
    void paint (juce::Graphics&) override;
    void resized() override;
    void addCell (KnobCell* c);
    void setPower (PercoolatorProcessor&, const juce::String& paramID);
    void setCombo (PercoolatorProcessor&, const juce::String& paramID);
    juce::OwnedArray<KnobCell> cells;
    std::unique_ptr<juce::ComboBox> combo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtt;
    juce::String title;
    juce::Colour accent;
private:
    std::unique_ptr<juce::TextButton> power;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAtt;
};

// ---- draggable random node ----
class ModChip : public juce::Component
{
public:
    ModChip (PercoolatorProcessor&, int index, juce::Colour);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDrag (const juce::MouseEvent&) override;
    void refresh();
private:
    PercoolatorProcessor& proc;
    int index;
    juce::Colour accent;
    juce::TextButton bip { "BI" }, hold { "H" };
    float shown = 0.0f;
};

class Scope : public juce::Component
{
public:
    explicit Scope (PercoolatorProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    PercoolatorProcessor& proc;
};

class PercoolatorEditor : public juce::AudioProcessorEditor,
                          public juce::DragAndDropContainer,
                          private juce::Timer
{
public:
    explicit PercoolatorEditor (PercoolatorProcessor&);
    ~PercoolatorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void rebuildPresetList();
    void showFxPage (int index);

    PercoolatorProcessor& proc;
    NeonVoidLNF lnf;

    juce::OwnedArray<Panel> engines;   // body, noise, snap
    juce::OwnedArray<Panel> fxPages;   // trans, reso, space, diode, vca, mixer
    juce::OwnedArray<juce::TextButton> fxTabs;
    juce::OwnedArray<ModChip> chips;
    int activeFx = 0;

    Scope scope { proc };
    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" },
                     savePreset { "SAVE" }, loadPreset { "LOAD" },
                     randomise { "RANDOMIZE" }, clearMods { "CLEAR MOD" },
                     trigger { "TRIGGER" }, softClip { "SOFT" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> softAtt;
    std::unique_ptr<KnobCell> masterCell, velCell;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String status;
    int statusTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PercoolatorEditor)
};
