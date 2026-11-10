#include "PluginProcessor.h"
#include "Presets.h"
#include "PluginEditor.h"

using APF = juce::AudioParameterFloat;
using APB = juce::AudioParameterBool;
using APC = juce::AudioParameterChoice;
using NRange = juce::NormalisableRange<float>;

static NRange lin (float a, float b) { return NRange (a, b); }
static NRange logr (float a, float b)
{
    NRange nr (a, b);
    nr.setSkewForCentre (std::sqrt (a * b));
    return nr;
}

juce::AudioProcessorValueTreeState::ParameterLayout PercoolatorProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto f = [&l] (const char* id, const char* name, NRange r, float def)
    {
        l.add (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, r, def));
    };
    auto b = [&l] (const char* id, const char* name, bool def)
    {
        l.add (std::make_unique<APB> (juce::ParameterID { id, 1 }, name, def));
    };
    auto c = [&l] (const char* id, const char* name, juce::StringArray opts, int def)
    {
        l.add (std::make_unique<APC> (juce::ParameterID { id, 1 }, name, opts, def));
    };

    b ("body_on", "Body On", true);
    c ("body_wave", "Body Wave", { "Sine", "Triangle", "Saw", "FM Shell" }, 0);
    f ("body_pitch", "Body Pitch", logr (40.0f, 420.0f), 178.0f);
    f ("body_bend", "Body Bend", lin (-1.0f, 1.0f), 0.42f);
    f ("body_btime", "Body Bend Time", logr (2.0f, 300.0f), 26.0f);
    f ("body_decay", "Body Decay", logr (20.0f, 900.0f), 165.0f);
    f ("body_drive", "Body Drive", lin (0.0f, 1.0f), 0.22f);
    f ("body_level", "Body Level", lin (0.0f, 1.4f), 0.82f);

    b ("noise_on", "Noise On", true);
    c ("noise_type", "Noise Source", { "White", "Pink", "Metallic", "Crackle" }, 0);
    f ("noise_color", "Noise Color", lin (0.0f, 1.0f), 0.56f);
    f ("noise_q", "Noise Reso", logr (0.4f, 9.0f), 0.8f);
    f ("noise_decay", "Noise Decay", logr (10.0f, 1400.0f), 205.0f);
    f ("noise_shape", "Noise Shape", lin (1.0f, 14.0f), 4.5f);
    f ("noise_crush", "Noise Crush", lin (0.0f, 1.0f), 0.0f);
    f ("noise_level", "Noise Level", lin (0.0f, 1.4f), 0.62f);

    b ("snap_on", "Snap On", true);
    c ("snap_type", "Snap Model", { "Digital Click", "Stick", "Rim", "Clap Stack" }, 0);
    f ("snap_attack", "Snap Attack", logr (0.05f, 30.0f), 0.6f);
    f ("snap_tight", "Snap Tight", logr (1.0f, 140.0f), 16.0f);
    f ("snap_tone", "Snap Tone", logr (200.0f, 14000.0f), 4200.0f);
    f ("snap_level", "Snap Level", lin (0.0f, 1.4f), 0.5f);

    b ("trans_on", "Transient On", true);
    f ("trans_amount", "Transient", lin (-1.0f, 1.0f), 0.3f);

    b ("reso_on", "Resonator On", false);
    c ("reso_mode", "Resonator Mode", { "Comb", "String", "Plate" }, 0);
    f ("reso_tune", "Reso Tune", logr (40.0f, 2000.0f), 220.0f);
    f ("reso_fb", "Reso Feedback", lin (0.0f, 0.97f), 0.55f);
    f ("reso_damp", "Reso Damping", lin (0.0f, 1.0f), 0.45f);
    f ("reso_mix", "Reso Mix", lin (0.0f, 1.0f), 0.3f);

    b ("space_on", "Space On", false);
    f ("space_dtime", "Delay Time", logr (10.0f, 600.0f), 95.0f);
    f ("space_dfb", "Delay Feedback", lin (0.0f, 0.85f), 0.3f);
    f ("space_dmix", "Delay Mix", lin (0.0f, 1.0f), 0.14f);
    f ("space_size", "Reverb Size", lin (0.05f, 1.0f), 0.6f);
    f ("space_decay", "Reverb Decay", lin (0.0f, 1.0f), 0.5f);
    f ("space_rmix", "Reverb Mix", lin (0.0f, 1.0f), 0.2f);

    b ("diode_on", "Diode On", false);
    f ("diode_drive", "Diode Drive", lin (0.0f, 1.0f), 0.35f);
    f ("diode_thresh", "Diode Threshold", lin (0.08f, 1.0f), 0.7f);
    f ("diode_asym", "Diode Asym", lin (0.0f, 1.0f), 0.5f);
    f ("diode_tone", "Diode Tilt", lin (0.0f, 1.0f), 0.5f);

    b ("vca_on", "VCA On", true);
    f ("vca_thresh", "VCA Threshold", lin (-60.0f, 0.0f), -26.0f);
    f ("vca_ratio", "VCA Ratio", logr (1.0f, 40.0f), 8.0f);
    f ("vca_attack", "VCA Attack", logr (0.01f, 20.0f), 0.08f);
    f ("vca_release", "VCA Release", logr (3.0f, 400.0f), 45.0f);
    f ("vca_makeup", "VCA Makeup", lin (0.0f, 24.0f), 4.0f);
    f ("vca_blend", "VCA Parallel", lin (0.0f, 1.0f), 0.35f);

    b ("mixer_on", "Mixer On", true);
    f ("mixer_wet", "Processed", lin (0.0f, 1.4f), 1.0f);
    f ("mixer_dry", "Original", lin (0.0f, 1.4f), 0.0f);
    c ("mixer_inv", "Original Phase", { "Normal", "Inverted" }, 0);

    f ("master_gain", "Master", lin (0.0f, 1.6f), 0.85f);
    b ("master_soft", "Soft Clip", true);
    f ("vel", "Velocity", lin (0.05f, 1.0f), 1.0f);
    return l;
}

PercoolatorProcessor::PercoolatorProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      Thread ("PercoolatorRender"),
      apvts (*this, nullptr, "PERCOOLATOR", createLayout())
{
    for (int i = 0; i < 8; ++i)
    {
        modBipolar[(size_t) i].store (i < 4 || i == 6);
        modHold[(size_t) i].store (false);
        modVals[(size_t) i].store (0.0f);
        stale[(size_t) i].store (true);
    }
    for (auto* p : getParameters())
        if (auto* wp = dynamic_cast<juce::AudioProcessorParameterWithID*> (p))
            apvts.addParameterListener (wp->paramID, this);
}

PercoolatorProcessor::~PercoolatorProcessor()
{
    stopThread (2000);
}

void PercoolatorProcessor::parameterChanged (const juce::String&, float)
{
    dirty.store (true);
}

bool PercoolatorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void PercoolatorProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    for (auto& v : voices) v.active = false;
    renderVariant (0);
    for (int i = 1; i < 8; ++i) { juce::SpinLock::ScopedLockType l (variantLock); variants[(size_t) i] = variants[0]; }
    for (int i = 1; i < 8; ++i) stale[(size_t) i].store (true);
    if (! isThreadRunning()) startThread();
}

calabi::Params PercoolatorProcessor::buildParams (const float* mv)
{
    std::vector<ModRoute> rr;
    { juce::SpinLock::ScopedLockType l (routeLock); rr = routes; }

    auto val = [this, &rr, mv] (const char* id) -> float
    {
        auto* raw = apvts.getRawParameterValue (id);
        float v = raw != nullptr ? raw->load() : 0.0f;
        float add = 0.0f;
        for (const auto& r : rr)
            if (r.paramID == id) add += r.depth * mv[r.mod];
        if (std::abs (add) > 1.0e-6f)
        {
            auto range = apvts.getParameterRange (id);
            v = range.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, range.convertTo0to1 (v) + add));
        }
        return v;
    };
    auto bit = [this] (const char* id) -> bool
    {
        auto* raw = apvts.getRawParameterValue (id);
        return raw != nullptr && raw->load() > 0.5f;
    };
    auto idx = [this] (const char* id) -> int
    {
        auto* raw = apvts.getRawParameterValue (id);
        return raw != nullptr ? (int) raw->load() : 0;
    };

    calabi::Params p;
    p.masterGain = val ("master_gain"); p.soft = bit ("master_soft");

    p.body.on = bit ("body_on");      p.body.wave = idx ("body_wave");
    p.body.pitch = val ("body_pitch"); p.body.bend = val ("body_bend");
    p.body.btime = val ("body_btime"); p.body.decay = val ("body_decay");
    p.body.drive = val ("body_drive"); p.body.level = val ("body_level");

    p.noise.on = bit ("noise_on");     p.noise.type = idx ("noise_type");
    p.noise.color = val ("noise_color"); p.noise.q = val ("noise_q");
    p.noise.decay = val ("noise_decay"); p.noise.shape = val ("noise_shape");
    p.noise.crush = val ("noise_crush"); p.noise.level = val ("noise_level");

    p.snap.on = bit ("snap_on");       p.snap.type = idx ("snap_type");
    p.snap.attack = val ("snap_attack"); p.snap.tight = val ("snap_tight");
    p.snap.tone = val ("snap_tone");   p.snap.level = val ("snap_level");

    p.trans.on = bit ("trans_on");     p.trans.amount = val ("trans_amount");

    p.reso.on = bit ("reso_on");       p.reso.mode = idx ("reso_mode");
    p.reso.tune = val ("reso_tune");   p.reso.fb = val ("reso_fb");
    p.reso.damp = val ("reso_damp");   p.reso.mix = val ("reso_mix");

    p.space.on = bit ("space_on");     p.space.dtime = val ("space_dtime");
    p.space.dfb = val ("space_dfb");   p.space.dmix = val ("space_dmix");
    p.space.size = val ("space_size"); p.space.decay = val ("space_decay");
    p.space.rmix = val ("space_rmix");

    p.diode.on = bit ("diode_on");     p.diode.drive = val ("diode_drive");
    p.diode.thresh = val ("diode_thresh"); p.diode.asym = val ("diode_asym");
    p.diode.tone = val ("diode_tone");

    p.vca.on = bit ("vca_on");         p.vca.thresh = val ("vca_thresh");
    p.vca.ratio = val ("vca_ratio");   p.vca.attack = val ("vca_attack");
    p.vca.release = val ("vca_release"); p.vca.makeup = val ("vca_makeup");
    p.vca.blend = val ("vca_blend");

    p.mixer.on = bit ("mixer_on");     p.mixer.wet = val ("mixer_wet");
    p.mixer.dry = val ("mixer_dry");   p.mixer.inv = idx ("mixer_inv") > 0;
    return p;
}

void PercoolatorProcessor::renderVariant (int slot)
{
    float mv[8];
    for (int m = 0; m < 8; ++m)
    {
        if (modHold[(size_t) m].load())
        {
            mv[m] = modVals[(size_t) m].load();
        }
        else
        {
            mv[m] = modBipolar[(size_t) m].load() ? rng.nextFloat() * 2.0f - 1.0f : rng.nextFloat();
            modVals[(size_t) m].store (mv[m]);
        }
    }

    auto buf = std::make_shared<Buf>();
    float gr = 0.0f;
    const float peak = calabi::render (buildParams (mv), sr, rng, *buf, nullptr, nullptr, &gr);

    { juce::SpinLock::ScopedLockType l (variantLock); variants[(size_t) slot] = buf; }
    { juce::SpinLock::ScopedLockType l (displayLock); displayBuf = buf; }
    displayGR.store (gr);
    displayPeak.store (peak);
}

void PercoolatorProcessor::run()
{
    while (! threadShouldExit())
    {
        if (dirty.exchange (false))
            for (int i = 0; i < 8; ++i) stale[(size_t) i].store (true);

        bool did = false;
        for (int i = 0; i < 8 && ! threadShouldExit(); ++i)
        {
            if (stale[(size_t) i].exchange (false))
            {
                renderVariant (i);
                did = true;
                break;                        // one at a time, stays responsive
            }
        }
        wait (did ? 1 : 15);
    }
}

void PercoolatorProcessor::startVoice (float gain)
{
    BufPtr b;
    {
        juce::SpinLock::ScopedLockType l (variantLock);
        b = variants[(size_t) nextVariant];
    }
    if (b == nullptr || b->empty()) return;

    stale[(size_t) nextVariant].store (true);
    nextVariant = (nextVariant + 1) % 8;
    notify();

    Voice* slot = nullptr;
    for (auto& v : voices) if (! v.active) { slot = &v; break; }
    if (slot == nullptr)
    {
        slot = &voices[0];
        int best = 0;
        for (int i = 1; i < 8; ++i) if (voices[(size_t) i].pos > voices[(size_t) best].pos) best = i;
        slot = &voices[(size_t) best];
    }
    slot->buf = b; slot->pos = 0; slot->gain = gain; slot->active = true;
}

void PercoolatorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int numSamples = buffer.getNumSamples();
    auto* velRaw = apvts.getRawParameterValue ("vel");
    const float velParam = velRaw != nullptr ? velRaw->load() : 1.0f;

    struct Hit { int offset; float gain; };
    std::array<Hit, 16> hits; int numHits = 0;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn() && numHits < 16)
            hits[(size_t) numHits++] = { meta.samplePosition, velParam * msg.getFloatVelocity() };
    }
    if (uiTrigger.exchange (false) && numHits < 16)
        hits[(size_t) numHits++] = { 0, velParam };

    int hitIdx = 0;
    for (int s = 0; s < numSamples; ++s)
    {
        while (hitIdx < numHits && hits[(size_t) hitIdx].offset <= s)
        {
            startVoice (hits[(size_t) hitIdx].gain);
            ++hitIdx;
        }
        float out = 0.0f;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            if (v.pos >= (int) v.buf->size()) { v.active = false; v.buf.reset(); continue; }
            out += (*v.buf)[(size_t) v.pos++] * v.gain;
        }
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.getWritePointer (ch)[s] = out;
    }
    midi.clear();
}

BufPtr PercoolatorProcessor::getDisplayBuffer()
{
    juce::SpinLock::ScopedLockType l (displayLock);
    return displayBuf;
}

std::vector<ModRoute> PercoolatorProcessor::getRoutes()
{
    juce::SpinLock::ScopedLockType l (routeLock);
    return routes;
}
void PercoolatorProcessor::addRoute (int mod, const juce::String& paramID, float depth)
{
    { juce::SpinLock::ScopedLockType l (routeLock); routes.push_back ({ mod, paramID, depth }); }
    dirty.store (true);
}
void PercoolatorProcessor::removeRoute (int index)
{
    {
        juce::SpinLock::ScopedLockType l (routeLock);
        if (index >= 0 && index < (int) routes.size()) routes.erase (routes.begin() + index);
    }
    dirty.store (true);
}
void PercoolatorProcessor::setRouteDepth (int index, float depth)
{
    {
        juce::SpinLock::ScopedLockType l (routeLock);
        if (index >= 0 && index < (int) routes.size())
            routes[(size_t) index].depth = juce::jlimit (-1.0f, 1.0f, depth);
    }
    dirty.store (true);
}
void PercoolatorProcessor::clearRoutes()
{
    { juce::SpinLock::ScopedLockType l (routeLock); routes.clear(); }
    dirty.store (true);
}
float PercoolatorProcessor::modOffsetFor (const juce::String& paramID)
{
    float a = 0.0f;
    juce::SpinLock::ScopedLockType l (routeLock);
    for (const auto& r : routes)
        if (r.paramID == paramID) a += r.depth * modVals[(size_t) r.mod].load();
    return a;
}

void PercoolatorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("PERC"), nullptr);
    juce::ValueTree c ("PERC");
    for (int m = 0; m < 8; ++m)
    {
        juce::ValueTree mm ("MOD");
        mm.setProperty ("i", m, nullptr);
        mm.setProperty ("bip", modBipolar[(size_t) m].load(), nullptr);
        mm.setProperty ("hold", modHold[(size_t) m].load(), nullptr);
        c.appendChild (mm, nullptr);
    }
    for (const auto& r : getRoutes())
    {
        juce::ValueTree rr ("ROUTE");
        rr.setProperty ("m", r.mod, nullptr);
        rr.setProperty ("p", r.paramID, nullptr);
        rr.setProperty ("d", r.depth, nullptr);
        c.appendChild (rr, nullptr);
    }
    state.appendChild (c, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void PercoolatorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;
    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid()) return;

    auto c = state.getChildWithName ("PERC");
    std::vector<ModRoute> newRoutes;
    if (c.isValid())
    {
        for (int i = 0; i < c.getNumChildren(); ++i)
        {
            auto ch = c.getChild (i);
            if (ch.hasType ("MOD"))
            {
                const int m = (int) ch.getProperty ("i", 0);
                if (m >= 0 && m < 8)
                {
                    modBipolar[(size_t) m].store ((bool) ch.getProperty ("bip", true));
                    modHold[(size_t) m].store ((bool) ch.getProperty ("hold", false));
                }
            }
            else if (ch.hasType ("ROUTE"))
            {
                newRoutes.push_back ({ (int) ch.getProperty ("m", 0),
                                       ch.getProperty ("p", "").toString(),
                                       (float) ch.getProperty ("d", 0.25) });
            }
        }
        state.removeChild (c, nullptr);
    }
    { juce::SpinLock::ScopedLockType l (routeLock); routes = newRoutes; }
    apvts.replaceState (state);
    dirty.store (true);
}

// ============================ presets ==============================
juce::File PercoolatorProcessor::userPresetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                 .getChildFile ("Calabi Yau").getChildFile ("Percoolator").getChildFile ("Presets");
    if (! f.exists()) f.createDirectory();
    return f;
}

void PercoolatorProcessor::refreshUserPresets()
{
    userPresetNames.clear();
    userPresetFiles.clear();
    for (const auto& f : userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.percpreset"))
    {
        userPresetFiles.add (f);
        userPresetNames.add (f.getFileNameWithoutExtension());
    }
}

juce::StringArray PercoolatorProcessor::getPresetNames() const
{
    juce::StringArray names;
    for (int i = 0; i < numFactoryPresets; ++i)
        names.add (juce::String (factoryPresets[i]).upToFirstOccurrenceOf ("|", false, false));
    names.addArray (userPresetNames);
    return names;
}

void PercoolatorProcessor::applyPresetString (const juce::String& spec)
{
    // every parameter first goes back to its default, then the spec is laid on top
    for (auto* p : getParameters())
        if (auto* wp = dynamic_cast<juce::RangedAudioParameter*> (p))
            wp->setValueNotifyingHost (wp->getDefaultValue());

    auto body = spec.fromFirstOccurrenceOf ("|", false, false);
    for (const auto& pair : juce::StringArray::fromTokens (body, ",", ""))
    {
        const auto id = pair.upToFirstOccurrenceOf ("=", false, false).trim();
        const auto vs = pair.fromFirstOccurrenceOf ("=", false, false).trim();
        if (id.isEmpty() || vs.isEmpty()) continue;
        if (auto* par = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
            par->setValueNotifyingHost (par->convertTo0to1 (vs.getFloatValue()));
    }
    dirty.store (true);
}

void PercoolatorProcessor::loadPreset (int index)
{
    refreshUserPresets();
    const int total = numFactoryPresets + userPresetNames.size();
    if (total == 0) return;
    index = (index % total + total) % total;
    presetIndex = index;

    if (index < numFactoryPresets)
    {
        clearRoutes();
        applyPresetString (factoryPresets[index]);
    }
    else
    {
        loadPresetFile (userPresetFiles[index - numFactoryPresets]);
    }
}

void PercoolatorProcessor::stepPreset (int delta) { loadPreset (presetIndex + delta); }

bool PercoolatorProcessor::savePresetFile (const juce::File& file)
{
    juce::MemoryBlock mb;
    getStateInformation (mb);
    auto xml = getXmlFromBinary (mb.getData(), (int) mb.getSize());
    if (xml == nullptr) return false;
    const bool ok = xml->writeTo (file);
    refreshUserPresets();
    return ok;
}

bool PercoolatorProcessor::loadPresetFile (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr) return false;
    juce::MemoryBlock mb;
    copyXmlToBinary (*xml, mb);
    setStateInformation (mb.getData(), (int) mb.getSize());
    return true;
}

void PercoolatorProcessor::savePresetAs (const juce::String& name)
{
    auto file = userPresetFolder().getChildFile (juce::File::createLegalFileName (name) + ".percpreset");
    savePresetFile (file);
    refreshUserPresets();
    const int idx = userPresetNames.indexOf (file.getFileNameWithoutExtension());
    if (idx >= 0) presetIndex = numFactoryPresets + idx;
}

void PercoolatorProcessor::randomiseAll (bool includeModRouting)
{
    // Master, velocity and the mixer blend stay put: randomising those only
    // changes how loud the result is, not what it sounds like.
    static const juce::StringArray skip { "master_gain", "master_soft", "vel",
                                          "mixer_on", "mixer_wet", "mixer_dry", "mixer_inv" };
    for (auto* p : getParameters())
    {
        auto* wp = dynamic_cast<juce::RangedAudioParameter*> (p);
        if (wp == nullptr || skip.contains (wp->paramID)) continue;

        if (auto* b = dynamic_cast<juce::AudioParameterBool*> (p))
        {
            // modules stay on more often than not, so a roll is still usable
            b->setValueNotifyingHost (rng.nextFloat() < 0.72f ? 1.0f : 0.0f);
        }
        else
        {
            wp->setValueNotifyingHost (rng.nextFloat());
        }
    }

    if (includeModRouting)
    {
        clearRoutes();
        static const char* targets[] = {
            "body_pitch", "body_bend", "body_decay", "body_drive",
            "noise_color", "noise_decay", "noise_shape", "noise_crush",
            "snap_tight", "snap_tone", "snap_level", "trans_amount",
            "reso_tune", "reso_fb", "space_rmix", "diode_drive" };
        const int numTargets = (int) (sizeof (targets) / sizeof (targets[0]));
        const int howMany = 2 + rng.nextInt (4);
        for (int i = 0; i < howMany; ++i)
            addRoute (rng.nextInt (8), targets[rng.nextInt (numTargets)],
                      (rng.nextFloat() * 2.0f - 1.0f) * 0.45f);
    }
    dirty.store (true);
}

juce::AudioProcessorEditor* PercoolatorProcessor::createEditor()
{
    return new PercoolatorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PercoolatorProcessor();
}
