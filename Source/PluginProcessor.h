#pragma once
#include <JuceHeader.h>
#include "CalabiDSP.h"
#include <array>
#include <memory>
#include <vector>

using Buf = std::vector<float>;
using BufPtr = std::shared_ptr<Buf>;

struct ModRoute
{
    int mod = 0;
    juce::String paramID;
    float depth = 0.25f;
};

class PercoolatorProcessor : public juce::AudioProcessor,
                             private juce::Thread,
                             private juce::AudioProcessorValueTreeState::Listener
{
public:
    PercoolatorProcessor();
    ~PercoolatorProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Percoolator"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Patch"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- editor API ----
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void triggerFromUI() { uiTrigger.store (true); }
    BufPtr getDisplayBuffer();                       // last rendered hit, for the scope
    float getDisplayGR() const { return displayGR.load(); }
    float getDisplayPeak() const { return displayPeak.load(); }
    float getModValue (int m) const { return modVals[(size_t) m].load(); }

    std::vector<ModRoute> getRoutes();
    void addRoute (int mod, const juce::String& paramID, float depth);
    void removeRoute (int index);
    void setRouteDepth (int index, float depth);
    void clearRoutes();
    float modOffsetFor (const juce::String& paramID);  // summed depth*value, for knob display

    std::array<std::atomic<bool>, 8> modBipolar;
    std::array<std::atomic<bool>, 8> modHold;

    // ---- presets ----
    juce::StringArray getPresetNames() const;      // factory, then user
    int  getPresetIndex() const { return presetIndex; }
    void loadPreset (int index);
    void stepPreset (int delta);
    void savePresetAs (const juce::String& name);  // into the user preset folder
    bool loadPresetFile (const juce::File&);
    bool savePresetFile (const juce::File&);
    static juce::File userPresetFolder();
    void refreshUserPresets();
    void randomiseAll (bool includeModRouting);

private:
    void run() override;                               // background render thread
    void parameterChanged (const juce::String&, float) override;
    calabi::Params buildParams (const float* mv);
    void renderVariant (int slot);
    void rollMods();
    void startVoice (float gain);

    double sr = 48000.0;
    juce::Random rng;

    std::array<BufPtr, 8> variants;
    std::array<std::atomic<bool>, 8> stale;
    juce::SpinLock variantLock;
    int nextVariant = 0;

    struct Voice { BufPtr buf; int pos = 0; float gain = 1.0f; bool active = false; };
    std::array<Voice, 8> voices;

    std::array<std::atomic<float>, 8> modVals;
    std::vector<ModRoute> routes;
    juce::SpinLock routeLock;

    void applyPresetString (const juce::String& spec);
    juce::StringArray userPresetNames;
    juce::Array<juce::File> userPresetFiles;
    int presetIndex = 0;

    std::atomic<bool> uiTrigger { false };
    std::atomic<bool> dirty { true };
    std::atomic<float> displayGR { 0.0f }, displayPeak { 0.0f };
    BufPtr displayBuf;
    juce::SpinLock displayLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PercoolatorProcessor)
};
