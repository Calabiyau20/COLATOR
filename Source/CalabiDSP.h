#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

// ============================================================
//  CALABI SNARE — snare design engine
//  BODY + NOISE + SNAP -> TRANSIENT -> RESONATOR -> SPACE
//  -> DIODE CLIP -> VCA (parallel) -> MIXER -> MASTER
// ============================================================
namespace calabi
{
    inline float clampf (float x, float a, float b) noexcept { return x < a ? a : (x > b ? b : x); }

    struct SVF
    {
        float ic1 = 0.0f, ic2 = 0.0f, lp = 0.0f, bp = 0.0f, hp = 0.0f;
        void reset() noexcept { ic1 = ic2 = lp = bp = hp = 0.0f; }
        void proc (float x, float fc, float Q, double sr) noexcept
        {
            fc = clampf (fc, 10.0f, (float) sr * 0.47f);
            const float g  = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
            const float k  = 1.0f / clampf (Q, 0.3f, 12.0f);
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1, a3 = g * a2;
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
            lp = v2; bp = v1; hp = x - k * v1 - v2;
        }
    };

    struct OnePole
    {
        float z = 0.0f;
        void reset() noexcept { z = 0.0f; }
        float proc (float x, float fc, double sr) noexcept
        {
            const float c = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi
                                             * clampf (fc, 5.0f, (float) sr * 0.49f) / (float) sr);
            z += (x - z) * c;
            return z;
        }
    };

    struct DelayLine
    {
        std::vector<float> buf; int i = 0;
        void init (double seconds, double sr)
        {
            buf.assign ((size_t) std::ceil (seconds * sr) + 4, 0.0f); i = 0;
        }
        float read (float samples) const noexcept
        {
            const int n = (int) buf.size();
            int j = i - juce::jmax (1, juce::roundToInt (samples));
            while (j < 0) j += n;
            return buf[(size_t) j];
        }
        void write (float x) noexcept { buf[(size_t) i] = x; i = (i + 1) % (int) buf.size(); }
    };

    // ---- the whole patch, plain values ----
    struct Params
    {
        float masterGain = 0.85f; bool soft = true;

        struct { bool on = true; int wave = 0; float pitch = 178.0f, bend = 0.42f, btime = 26.0f,
                 decay = 165.0f, drive = 0.22f, level = 0.82f; } body;       // wave: sine/tri/saw/fm
        struct { bool on = true; int type = 0; float color = 0.56f, q = 0.8f, decay = 205.0f,
                 shape = 4.5f, crush = 0.0f, level = 0.62f; } noise;         // type: white/pink/metal/crackle
        struct { bool on = true; int type = 0; float attack = 0.6f, tight = 16.0f,
                 tone = 4200.0f, level = 0.5f; } snap;                       // type: click/stick/rim/clap
        struct { bool on = true;  float amount = 0.3f; } trans;
        struct { bool on = false; int mode = 0; float tune = 220.0f, fb = 0.55f,
                 damp = 0.45f, mix = 0.3f; } reso;                           // mode: comb/string/plate
        struct { bool on = false; float dtime = 95.0f, dfb = 0.3f, dmix = 0.14f,
                 size = 0.6f, decay = 0.5f, rmix = 0.2f; } space;
        struct { bool on = false; float drive = 0.35f, thresh = 0.7f, asym = 0.5f, tone = 0.5f; } diode;
        struct { bool on = true;  float thresh = -26.0f, ratio = 8.0f, attack = 0.08f,
                 release = 45.0f, makeup = 4.0f, blend = 0.35f; } vca;
        struct { bool on = true;  float dry = 0.0f, wet = 1.0f; bool inv = false; } mixer;
    };

    inline void makeNoise (int type, int n, juce::Random& rng, std::vector<float>& out, double sr)
    {
        out.assign ((size_t) n, 0.0f);
        if (type == 0)
        {
            for (int i = 0; i < n; ++i) out[(size_t) i] = rng.nextFloat() * 2.0f - 1.0f;
        }
        else if (type == 1) // pink
        {
            float b0=0,b1=0,b2=0,b3=0,b4=0,b5=0,b6=0;
            for (int i = 0; i < n; ++i)
            {
                const float w = rng.nextFloat() * 2.0f - 1.0f;
                b0 = 0.99886f*b0 + w*0.0555179f; b1 = 0.99332f*b1 + w*0.0750759f;
                b2 = 0.96900f*b2 + w*0.1538520f; b3 = 0.86650f*b3 + w*0.3104856f;
                b4 = 0.55000f*b4 + w*0.5329522f; b5 = -0.7616f*b5 - w*0.0168980f;
                out[(size_t) i] = (b0+b1+b2+b3+b4+b5+b6 + w*0.5362f) * 0.18f;
                b6 = w * 0.115926f;
            }
        }
        else if (type == 2) // metallic
        {
            const float r[6] = { 1.0f, 1.4142f, 1.6818f, 1.9742f, 2.3146f, 2.6667f };
            const float base = 317.0f;
            for (int i = 0; i < n; ++i)
            {
                const float t = (float) i / (float) sr; float s = 0.0f;
                for (int k = 0; k < 6; ++k)
                    s += std::sin (juce::MathConstants<float>::twoPi * base * r[k] * t) >= 0.0f ? 1.0f : -1.0f;
                out[(size_t) i] = s / 6.0f;
            }
        }
        else // crackle
        {
            float env = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                if (rng.nextFloat() < 0.004f) env = 1.0f;
                env *= 0.994f;
                out[(size_t) i] = (rng.nextFloat() * 2.0f - 1.0f) * env;
            }
        }
    }

    // Renders one hit, mono, into `out`. Returns the peak.
    inline float render (const Params& p, double sr, juce::Random& rng,
                         std::vector<float>& out, std::vector<float>* bodyOut = nullptr,
                         std::vector<float>* nzOut = nullptr, float* grOut = nullptr)
    {
        const float pi2 = juce::MathConstants<float>::twoPi;

        float tail = juce::jmax (p.body.decay, juce::jmax (p.noise.decay, p.snap.tight * 3.0f)) / 1000.0f * 2.2f + 0.05f;
        if (p.reso.on)  tail += 0.25f + p.reso.fb * 0.9f;
        if (p.space.on) tail += 0.4f + p.space.size * 1.1f + p.space.decay * 0.9f
                              + p.space.dtime / 1000.0f * (1.0f + p.space.dfb * 4.0f);
        if (p.vca.on)   tail += p.vca.release / 1000.0f;
        tail = clampf (tail, 0.12f, 3.4f);

        const int n = (int) (tail * sr);
        std::vector<float> bodyL ((size_t) n, 0.0f), noiseL ((size_t) n, 0.0f), snapL ((size_t) n, 0.0f);

        // ---- 01 BODY ----
        if (p.body.on)
        {
            const float f0 = p.body.pitch, bendRatio = std::pow (4.0f, p.body.bend);
            const float bt = p.body.btime / 1000.0f, decS = p.body.decay / 1000.0f;
            const float drv = 1.0f + p.body.drive * 14.0f, nd = std::tanh (drv);
            float phase = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                const float t = (float) i / (float) sr;
                const float f = f0 * (1.0f + (bendRatio - 1.0f) * std::exp (-t / bt));
                phase += pi2 * f / (float) sr;
                float s;
                switch (p.body.wave)
                {
                    case 1:  s = 2.0f / juce::MathConstants<float>::pi * std::asin (std::sin (phase)); break;
                    case 2:  s = 2.0f * (std::fmod (phase / pi2, 1.0f)) - 1.0f; break;
                    case 3:  s = std::sin (phase + 2.2f * std::exp (-t / (decS * 0.5f)) * std::sin (phase * 1.5f)); break;
                    default: s = std::sin (phase); break;
                }
                const float env = std::exp (-3.0f * t / decS);
                bodyL[(size_t) i] = std::tanh (s * env * drv) / nd * p.body.level;
            }
        }

        // ---- 02 NOISE ----
        if (p.noise.on)
        {
            std::vector<float> raw;
            makeNoise (p.noise.type, n, rng, raw, sr);
            SVF hp, lp;
            const float fcHP = 20.0f * std::pow (200.0f, juce::jmax (0.0f, 0.5f - p.noise.color) / 0.5f);
            const float fcLP = 20000.0f * std::pow (0.015f, juce::jmax (0.0f, p.noise.color - 0.5f) / 0.5f);
            const float ndec = p.noise.decay / 1000.0f, curve = p.noise.shape;
            const float bits = 16.0f - p.noise.crush * 13.0f, step = std::pow (2.0f, bits - 1.0f);
            const int down = 1 + (int) (p.noise.crush * p.noise.crush * 31.0f);
            float hold = 0.0f; int ctr = 0;
            for (int i = 0; i < n; ++i)
            {
                const float t = (float) i / (float) sr;
                hp.proc (raw[(size_t) i], fcHP, p.noise.q, sr);
                lp.proc (hp.hp, fcLP, p.noise.q, sr);
                float x = lp.lp;
                if (p.noise.crush > 0.001f)
                {
                    if (ctr % down == 0) hold = std::round (x * step) / step;
                    ++ctr; x = hold;
                }
                noiseL[(size_t) i] = x * std::exp (-curve * t / ndec) * p.noise.level;
            }
        }

        // ---- 03 SNAP ----
        if (p.snap.on)
        {
            const float atk = juce::jmax (p.snap.attack, 0.05f) / 1000.0f;
            const float tgt = p.snap.tight / 1000.0f, tone = p.snap.tone;
            SVF f1, f2; float ph = 0.0f, ph2 = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                const float t = (float) i / (float) sr;
                float env = std::exp (-4.0f * t / tgt) * (1.0f - std::exp (-t / atk));
                float s2 = 0.0f;
                if (p.snap.type == 0)
                {
                    const float imp = (i < 2 ? 1.0f : 0.0f)
                                    + (rng.nextFloat() * 2.0f - 1.0f) * std::exp (-t / 0.0007f);
                    f1.proc (imp, tone, 0.9f, sr); s2 = f1.hp;
                }
                else if (p.snap.type == 1)
                {
                    f1.proc (rng.nextFloat() * 2.0f - 1.0f, tone, 2.6f, sr); s2 = f1.bp * 2.2f;
                }
                else if (p.snap.type == 2)
                {
                    ph  += pi2 * tone / (float) sr;
                    ph2 += pi2 * tone * 1.593f / (float) sr;
                    f2.proc (std::sin (ph) * 0.7f + std::sin (ph2) * 0.45f
                             + (rng.nextFloat() * 2.0f - 1.0f) * 0.35f, tone * 1.6f, 1.4f, sr);
                    s2 = f2.bp * 1.4f;
                }
                else
                {
                    const float sp[3] = { 0.0f, 0.0105f, 0.0205f };
                    float g = 0.0f;
                    for (int c = 0; c < 3; ++c)
                    {
                        const float dt = t - sp[c];
                        if (dt >= 0.0f) g += std::exp (-dt / 0.0035f) * (1.0f - (float) c * 0.22f);
                    }
                    g += std::exp (-t / (tgt * 2.4f)) * 0.42f;
                    f1.proc (rng.nextFloat() * 2.0f - 1.0f, tone, 1.6f, sr);
                    s2 = f1.bp * g * 1.6f;
                    env = 1.0f - std::exp (-t / atk);
                }
                snapL[(size_t) i] = s2 * env * p.snap.level;
            }
        }

        // ---- engine sum (the Mixer's dry source) ----
        std::vector<float> dry ((size_t) n), mix ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            const float s = bodyL[(size_t) i] + noiseL[(size_t) i] + snapL[(size_t) i];
            dry[(size_t) i] = s; mix[(size_t) i] = s;
            noiseL[(size_t) i] += snapL[(size_t) i];
        }

        // ---- FX 1: transient ----
        if (p.trans.on && std::abs (p.trans.amount) > 0.001f)
        {
            const float a = p.trans.amount;
            for (int i = 0; i < n; ++i)
            {
                const float t = (float) i / (float) sr;
                mix[(size_t) i] *= juce::jmax (0.02f, (1.0f + a * 2.6f * std::exp (-t / 0.006f))
                                                    * (1.0f - a * 0.45f * (1.0f - std::exp (-t / 0.035f))));
            }
        }

        // ---- FX 2: resonator ----
        if (p.reso.on)
        {
            const float tunes3[3] = { 1.0f, 1.272f, 1.618f };
            const int taps = (p.reso.mode == 2) ? 3 : 1;
            DelayLine d[3]; OnePole dl[3]; float len[3];
            for (int j = 0; j < taps; ++j)
            {
                d[j].init (0.06, sr);
                len[j] = (float) sr / (p.reso.tune * tunes3[j]);
            }
            const float fb = (p.reso.mode == 1) ? -p.reso.fb : p.reso.fb;
            const float dampF = 18000.0f * std::pow (0.02f, p.reso.damp);
            for (int i = 0; i < n; ++i)
            {
                const float d0 = mix[(size_t) i]; float wet = 0.0f;
                for (int j = 0; j < taps; ++j)
                {
                    const float y = d0 + fb * dl[j].proc (d[j].read (len[j]), dampF, sr);
                    d[j].write (y); wet += y;
                }
                mix[(size_t) i] = d0 * (1.0f - p.reso.mix) + (wet / (float) taps) * p.reso.mix * 0.8f;
            }
        }

        // ---- FX 3: space ----
        if (p.space.on)
        {
            DelayLine dl; dl.init (0.8, sr); OnePole dlp;
            const float dsmp = p.space.dtime / 1000.0f * (float) sr;
            const float cd[4] = { 29.7f, 37.1f, 41.1f, 43.7f };
            const float apl[2] = { 5.0f, 1.7f };
            DelayLine cb[4], ap[2]; OnePole clp[4]; float clen[4], alen[2];
            for (int k = 0; k < 4; ++k)
            {
                cb[k].init (0.2, sr);
                clen[k] = cd[k] * (0.4f + p.space.size * 1.4f) / 1000.0f * (float) sr;
            }
            for (int k = 0; k < 2; ++k) { ap[k].init (0.05, sr); alen[k] = apl[k] / 1000.0f * (float) sr; }
            const float rfb = 0.70f + p.space.decay * 0.27f;
            const float rdamp = 14000.0f * std::pow (0.05f, p.space.decay * 0.7f);
            for (int i = 0; i < n; ++i)
            {
                const float dd = dl.read (dsmp);
                dl.write (mix[(size_t) i] + dlp.proc (dd, 6500.0f, sr) * p.space.dfb);
                const float wd = mix[(size_t) i] + dd * p.space.dmix;
                float rv = 0.0f;
                for (int k = 0; k < 4; ++k)
                {
                    const float fv = clp[k].proc (cb[k].read (clen[k]), rdamp, sr);
                    cb[k].write (wd + fv * rfb); rv += fv;
                }
                rv *= 0.25f;
                for (int k = 0; k < 2; ++k)
                {
                    const float bd = ap[k].read (alen[k]);
                    const float inp = rv - 0.5f * bd;
                    ap[k].write (inp); rv = bd + 0.5f * inp;
                }
                mix[(size_t) i] = wd * (1.0f - p.space.rmix * 0.5f) + rv * p.space.rmix * 1.1f;
            }
        }

        // ---- FX 4: diode clipper (after Space) ----
        if (p.diode.on)
        {
            const float d2 = 1.0f + p.diode.drive * 22.0f;
            const float th = clampf (p.diode.thresh, 0.08f, 1.0f);
            const float ak = clampf (1.0f - p.diode.asym * 0.75f, 0.25f, 1.0f);
            const float mk = 1.0f / (0.35f + p.diode.drive * 1.6f);
            OnePole tilt;
            for (int i = 0; i < n; ++i)
            {
                const float x2 = mix[(size_t) i] * d2;
                float y = x2 >= 0.0f ? th * (1.0f - std::exp (-x2 / th))
                                     : -th * ak * (1.0f - std::exp (x2 / (th * ak)));
                y *= mk;
                const float low = tilt.proc (y, 760.0f, sr);
                mix[(size_t) i] = low * 2.0f * (1.0f - p.diode.tone) + (y - low) * 2.0f * p.diode.tone;
            }
        }

        // ---- FX 5: parallel VCA ----
        float maxGR = 0.0f;
        if (p.vca.on && p.vca.blend > 0.0005f)
        {
            const float aAt = std::exp (-1.0f / (juce::jmax (p.vca.attack, 0.01f) / 1000.0f * (float) sr));
            const float aRe = std::exp (-1.0f / (juce::jmax (p.vca.release, 1.0f) / 1000.0f * (float) sr));
            const float mkp = std::pow (10.0f, p.vca.makeup / 20.0f);
            const float slope = 1.0f - 1.0f / juce::jmax (p.vca.ratio, 1.0f);
            float env = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                const float rect = std::abs (mix[(size_t) i]);
                env = rect > env ? aAt * env + (1.0f - aAt) * rect
                                 : aRe * env + (1.0f - aRe) * rect;
                const float eDb = env > 1.0e-7f ? 20.0f * std::log10 (env) : -140.0f;
                const float over = eDb - p.vca.thresh;
                const float grDb = over > 0.0f ? -over * slope : 0.0f;
                if (grDb < maxGR) maxGR = grDb;
                mix[(size_t) i] += mix[(size_t) i] * std::pow (10.0f, grDb / 20.0f) * mkp * p.vca.blend;
            }
        }

        // ---- MIXER ----
        const float dw = p.mixer.on ? p.mixer.dry : 0.0f;
        const float phs = p.mixer.inv ? -1.0f : 1.0f;
        for (int i = 0; i < n; ++i)
            mix[(size_t) i] = mix[(size_t) i] * p.mixer.wet + dry[(size_t) i] * dw * phs;

        // ---- master ----
        const float g0 = p.masterGain * 1.25f; float peak = 0.0f;
        const float nrm = std::tanh (1.15f);
        for (int i = 0; i < n; ++i)
        {
            float v = mix[(size_t) i] * g0;
            v = p.soft ? std::tanh (v * 1.15f) / nrm : clampf (v, -1.0f, 1.0f);
            mix[(size_t) i] = v;
            peak = juce::jmax (peak, std::abs (v));
        }

        // ---- trim + fade ----
        int last = n - 1;
        while (last > 480 && std::abs (mix[(size_t) last]) < 0.0004f) --last;
        last = juce::jmin (n - 1, last + 240);
        out.assign (mix.begin(), mix.begin() + last + 1);
        const int fade = juce::jmin (240, (int) out.size());
        for (int i = 0; i < fade; ++i)
            out[out.size() - (size_t) fade + (size_t) i] *= 1.0f - (float) i / (float) fade;

        if (bodyOut != nullptr) bodyOut->assign (bodyL.begin(), bodyL.begin() + (long) out.size());
        if (nzOut   != nullptr) nzOut->assign (noiseL.begin(), noiseL.begin() + (long) out.size());
        if (grOut   != nullptr) *grOut = maxGR;
        return peak;
    }
}
