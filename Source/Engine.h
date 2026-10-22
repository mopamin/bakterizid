#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>

// Klangerzeugung von Bakterizid. Pro Stimme:
//   Träger (Sinus) + Säge, beide frequenzmoduliert von einem Sinus-Modulator (Kultur = Verhältnis,
//   Toxizität = Index), Ringmodulation des Trägers mit einem Dreieck (Plasmid), Rauschen (Sporen),
//   Zustandsvariablen-Filter (Membran), ADSR (Wachstumskurve), Tonhöhen-Hüllkurve (Schock)
//   und zufällige Tonhöhendrift (Mutation).
// Danach für alle Stimmen gemeinsam: Sättigung (Lyse), Echo mit Bandpass im Rückweg (Generationen /
// Vererbung), Hall, Kompressor und Ausgangspegel.

namespace bkz
{
constexpr int kMaxVoices = 8;
constexpr double kPi = juce::MathConstants<double>::pi;
constexpr double kTwoPi = juce::MathConstants<double>::twoPi;

struct Params
{
    float ratio = 3.17f, index = 6.0f, ring = 0.45f, mutation = 40.0f;
    float cutoff = 3200.0f, q = 12.0f, lyse = 0.3f;
    float attack = 0.02f, decay = 1.2f, sustain = 0.65f, release = 1.4f;
    float penv = 0.0f, noise = 0.0f, echo = 0.4f, feedback = 0.6f, rate = 180.0f, vol = 0.8f;
    int ftype = 0, octave = 0, tuning = 0;
    bool arp = false;
};

// Frequenz einer Stufe s (0 = C3) im gewählten Tonsystem
inline double stepFreq (double s, int tuning, int octave)
{
    const double base = 130.81 * std::pow (2.0, (double) octave);
    switch (tuning)
    {
        case 1:  return base * std::pow (2.0, s / 19.0);              // Xeno 19
        case 2:  return base * std::pow (3.0, s / 13.0);              // Bohlen-Pierce
        case 3:                                                         // Obertonreihe
        {
            const double n = 8.0 + s;
            return n >= 1.0 ? base * n / 8.0 : base / 8.0 * std::pow (2.0, (n - 1.0) / 12.0);
        }
        default: return base * std::pow (2.0, s / 12.0);              // Terra 12
    }
}

// Zustandsvariablen-Filter nach Zavalishin (TPT), stabil auch bei schneller Modulation
struct Svf
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1;

    void set (float fc, float q, float sr)
    {
        fc = juce::jlimit (20.0f, sr * 0.45f, fc);
        const float g = (float) std::tan (kPi * fc / sr);
        k = 1.0f / juce::jmax (0.05f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    float process (float v0, int type)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        if (type == 1) return k * v1;                  // Bandpass mit 0 dB Spitze
        if (type == 2) return v0 - k * v1 - v2;        // Hochpass
        return v2;                                     // Tiefpass
    }

    void reset() { ic1 = ic2 = 0; }
};

// Filtergüte so umrechnen, dass sie sich wie in der Web-Version anfühlt:
// Tief- und Hochpass erhalten die Resonanz in dB, Bandpass die echte Güte.
inline float filterQ (float q, int type)
{
    return type == 1 ? q : std::pow (10.0f, q / 20.0f);
}

inline float polyBlep (double t, double dt)
{
    if (dt <= 0.0) return 0.0f;
    if (t < dt)       { t /= dt;               return (float) (t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt;    return (float) (t * t + t + t + 1.0); }
    return 0.0f;
}

inline double wrap (double p) { return p - std::floor (p); }

//==============================================================================
struct Voice
{
    bool active = false, releasing = false, killed = false, glide = false;
    int id = -1;
    uint64_t age = 0;
    float vel = 1.0f, cutMul = 1.0f;
    double freqTarget = 220.0, freq = 220.0;

    double carPh = 0, sawPh = 0, modPh = 0, ringPh = 0;
    double t = 0, relT = 0;        // Zeit seit Anschlag / seit Loslassen
    float amp = 0, relStart = 0;
    float pe = 0, peCoef = 0;      // Schock-Hüllkurve
    float drift = 0, driftTarget = 0, modDrift = 0;
    double driftTimer = 0;
    float dive = 1.0f;             // Tonhöhensturz beim Abtöten
    Svf filter;
    int filterCounter = 0;
    juce::Random rng;

    void start (int newId, double f, float velocity, float cm, bool isGlide, const Params& p, double sr, uint64_t stamp)
    {
        active = true; releasing = false; killed = false; glide = isGlide;
        id = newId; age = stamp; vel = velocity; cutMul = cm;
        freqTarget = freq = f;
        carPh = sawPh = 0; modPh = rng.nextDouble(); ringPh = rng.nextDouble();
        t = relT = 0; amp = 0; relStart = 0;
        drift = driftTarget = modDrift = 0; driftTimer = 0; dive = 1.0f;
        filter.reset(); filterCounter = 0;

        pe = p.penv > 0 ? std::pow (2.0f, p.penv / 12.0f) - 1.0f : 0.0f;
        const float pdec = 0.012f + juce::jmin (0.2f, p.decay * 0.12f);
        peCoef = (float) std::exp (-1.0 / (pdec * sr));
    }

    void stop (bool fast)
    {
        if (! active || (releasing && ! fast)) return;
        releasing = true;
        killed = killed || fast;
        relT = 0;
        relStart = amp;
    }

    float render (const Params& p, double sr)
    {
        const double dt = 1.0 / sr;

        // Tonhöhe (gleitend beim Abstrich in der Schale)
        if (glide) freq += (freqTarget - freq) * (1.0 - std::exp (-dt / 0.03));
        else       freq = freqTarget;

        // Mutation: alle 110 ms ein neues Driftziel, weich angefahren
        driftTimer -= dt;
        if (driftTimer <= 0) { driftTimer = 0.11; driftTarget = (rng.nextFloat() * 2.0f - 1.0f) * p.mutation; }
        drift    += (driftTarget - drift) * (float) (1.0 - std::exp (-dt / 0.09));
        modDrift += (-driftTarget * 1.7f - modDrift) * (float) (1.0 - std::exp (-dt / 0.12));

        pe *= peCoef;
        if (killed) dive = juce::jmax (0.2f, dive * (float) std::exp (std::log (0.2) * dt / 0.6));

        const double pitchMul = (1.0 + pe) * dive;
        const double fc = freq * pitchMul * std::pow (2.0, drift / 1200.0);
        const double fm = freq * p.ratio * (1.0 + pe) * std::pow (2.0, modDrift / 1200.0);

        // Modulationstiefe: beim Anschlag 1,8-fach, dann abklingend (wie ein Biss)
        const double after = juce::jmax (0.0, t - p.attack);
        const double depth = freq * p.index * (t < p.attack ? 1.8 : 1.0 + 0.8 * std::exp (-after / 0.2));

        modPh = wrap (modPh + fm * dt);
        const double mod = std::sin (kTwoPi * modPh) * depth;

        const double carF = fc + mod;
        carPh = wrap (carPh + carF * dt);
        const float car = (float) std::sin (kTwoPi * carPh);

        const double sawF = fc * 1.0052 + mod;                 // +9 Cent
        const double sawDt = std::abs (sawF) * dt;
        sawPh = wrap (sawPh + sawF * dt);
        const float saw = (float) (2.0 * sawPh - 1.0) - polyBlep (sawPh, sawDt);

        ringPh = wrap (ringPh + (freq * p.ratio * 0.5 + 31.0) * dt);
        const float tri = (float) (4.0 * std::abs (ringPh - 0.5) - 1.0);

        const float dry = car + saw * 0.12f;
        float x = dry * (1.0f - p.ring * 0.85f) + car * tri * p.ring * 1.4f;
        x += (rng.nextFloat() * 2.0f - 1.0f) * p.noise * 0.9f;

        // Filter: Grenzfrequenz öffnet sich beim Anschlag von 20 % auf 100 %
        if (filterCounter-- <= 0)
        {
            filterCounter = 16;
            const double ramp = juce::jmin (1.0, t / (p.attack + 0.25));
            const float env = (float) (0.2 * std::pow (5.0, ramp));
            filter.set (juce::jmin (18000.0f, p.cutoff * cutMul * env), filterQ (p.q, p.ftype), (float) sr);
        }
        const float y = filter.process (x, p.ftype);

        // Lautstärke: Attack linear, Decay und Release exponentiell
        const float peak = 0.28f * vel;
        if (! releasing)
        {
            if (t < p.attack) amp = peak * (float) (t / juce::jmax (0.001f, p.attack));
            else              amp += (peak * p.sustain - amp) * (float) (1.0 - std::exp (-dt / juce::jmax (0.003, p.decay / 3.0)));
        }
        else
        {
            const double rel = killed ? 0.25 : p.release;
            amp += (0.0f - amp) * (float) (1.0 - std::exp (-dt / juce::jmax (0.002, rel / 4.0)));
            relT += dt;
            if (relT > rel * 1.6 + 0.1 || (amp < 1.0e-5f && relT > 0.01)) active = false;
        }

        t += dt;
        return y * amp;
    }
};

//==============================================================================
// Ereignisse von der Oberfläche an den Audio-Thread
struct UiEvent
{
    enum Type { NoteOn, NoteOff, SwabOn, SwabMove, SwabOff, Kill } type;
    float a = 0, b = 0;
};

template <typename T, int Size>
struct Fifo
{
    juce::AbstractFifo fifo { Size };
    std::array<T, Size> buf {};

    bool push (const T& v)
    {
        const auto scope = fifo.write (1);
        if (scope.blockSize1 > 0) { buf[(size_t) scope.startIndex1] = v; return true; }
        if (scope.blockSize2 > 0) { buf[(size_t) scope.startIndex2] = v; return true; }
        return false;
    }

    template <typename Fn> void popAll (Fn&& fn)
    {
        const auto scope = fifo.read (fifo.getNumReady());
        for (int i = 0; i < scope.blockSize1; ++i) fn (buf[(size_t) (scope.startIndex1 + i)]);
        for (int i = 0; i < scope.blockSize2; ++i) fn (buf[(size_t) (scope.startIndex2 + i)]);
    }
};

//==============================================================================
class Engine
{
public:
    static constexpr int kSwabId = 1000;
    static constexpr int kArpId = 2000;

    Fifo<UiEvent, 256> uiEvents;     // Oberfläche -> Audio
    Fifo<float, 8192> scope;         // Audio -> Oberfläche (Oszilloskop)
    Fifo<int, 256> noteEvents;       // Audio -> Oberfläche (neue Kolonien)
    std::atomic<int> activeVoices { 0 };
    std::atomic<bool> killFlash { false };

    void prepare (double sampleRate, int blockSize)
    {
        sr = sampleRate;
        for (auto& v : voices) v.active = false;

        delayBuf.assign ((size_t) (sr * 2.0) + 4, 0.0f);
        writePos = 0;
        echoTone.reset();
        echoTone.set (1400.0f, 0.7f, (float) sr);
        wobblePh = 0;

        reverb.setSampleRate (sr);
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.82f; rp.damping = 0.35f; rp.wetLevel = 0.33f; rp.dryLevel = 0.0f; rp.width = 1.0f;
        reverb.setParameters (rp);
        reverb.reset();

        comp.prepare ({ sr, (juce::uint32) blockSize, 2 });
        comp.setThreshold (-14.0f); comp.setRatio (6.0f); comp.setAttack (3.0f); comp.setRelease (250.0f);
        comp.reset();

        volSmoothed.reset (sr, 0.03);
        killActive = false;
        arpCounter = 0;
        for (auto& o : arpOffs) o = -1;
    }

    void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Params& p)
    {
        params = p;
        volSmoothed.setTargetValue (p.vol);

        uiEvents.popAll ([this] (const UiEvent& e) { handleUi (e); });

        const int numSamples = buffer.getNumSamples();
        auto* left = buffer.getWritePointer (0);
        auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        auto midiIt = midi.cbegin();
        for (int i = 0; i < numSamples; ++i)
        {
            while (midiIt != midi.cend() && (*midiIt).samplePosition <= i)
            {
                handleMidi ((*midiIt).getMessage());
                ++midiIt;
            }
            if (p.arp) tickArp(); else stopArp();

            float l, r;
            renderSample (l, r);
            left[i] = l;
            if (right != nullptr) right[i] = r;
        }

        int n = 0;
        for (auto& v : voices) n += v.active ? 1 : 0;
        activeVoices.store (n);

        // Oszilloskop füttern (nur linker Kanal reicht)
        const auto w = scope.fifo.write (juce::jmin (numSamples, scope.fifo.getFreeSpace()));
        for (int i = 0; i < w.blockSize1; ++i) scope.buf[(size_t) (w.startIndex1 + i)] = left[i];
        for (int i = 0; i < w.blockSize2; ++i) scope.buf[(size_t) (w.startIndex2 + i)] = left[w.blockSize1 + i];
    }

private:
    double sr = 44100.0;
    Params params;
    std::array<Voice, kMaxVoices> voices;
    uint64_t stamp = 0;

    std::vector<float> delayBuf;
    int writePos = 0;
    Svf echoTone;
    double wobblePh = 0;
    juce::Reverb reverb;
    juce::dsp::Compressor<float> comp;
    juce::SmoothedValue<float> volSmoothed;

    // Bakterizid: Rauschsturz von 7 kHz in den Bass
    bool killActive = false;
    double killT = 0;
    Svf killFilter;
    juce::Random killRng;

    // Zellteilung (Zufalls-Arpeggiator)
    int64_t arpCounter = 0;
    int arpStep = 7, arpN = 0;
    std::array<int64_t, 16> arpOffs {};
    std::array<int, 16> arpIds {};
    juce::Random arpRng;
    bool arpWasOn = false;

    void handleMidi (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), stepFreq (m.getNoteNumber() - 48, params.tuning, params.octave),
                    0.3f + 0.7f * m.getFloatVelocity(), 1.0f, false, m.getNoteNumber() - 48);
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            for (auto& v : voices) v.stop (false);
    }

    void handleUi (const UiEvent& e)
    {
        switch (e.type)
        {
            case UiEvent::NoteOn:  { const int s = (int) e.a; noteOn (48 + s, stepFreq (s, params.tuning, params.octave), 1.0f, 1.0f, false, s); break; }
            case UiEvent::NoteOff: noteOff (48 + (int) e.a); break;
            case UiEvent::SwabOn:  noteOn (kSwabId, swabFreq (e.a), 1.0f, swabCut (e.b), true, -1); break;
            case UiEvent::SwabMove:
                for (auto& v : voices)
                    if (v.active && ! v.releasing && v.id == kSwabId) { v.freqTarget = swabFreq (e.a); v.cutMul = swabCut (e.b); }
                break;
            case UiEvent::SwabOff: noteOff (kSwabId); break;
            case UiEvent::Kill:    kill(); break;
        }
    }

    double swabFreq (float x) const { return stepFreq ((x + 0.9) / 1.8 * 24.0, params.tuning, params.octave); }
    static float swabCut (float y) { return std::pow (2.0f, -y * 2.2f); }

    void noteOn (int id, double f, float vel, float cutMul, bool glide, int uiStep)
    {
        noteOff (id);
        Voice* target = nullptr;
        for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (target == nullptr)
        {
            // älteste Stimme ablösen, ausklingende zuerst
            for (auto& v : voices)
                if (target == nullptr || (v.releasing && ! target->releasing) || (v.releasing == target->releasing && v.age < target->age))
                    target = &v;
        }
        target->start (id, f, vel, cutMul, glide, params, sr, ++stamp);
        if (uiStep >= 0) noteEvents.push (uiStep);
    }

    void noteOff (int id)
    {
        for (auto& v : voices)
            if (v.active && ! v.releasing && v.id == id) v.stop (false);
    }

    void kill()
    {
        for (auto& v : voices) if (v.active) v.stop (true);
        stopArp();
        killActive = true;
        killT = 0;
        killFilter.reset();
        killFlash.store (true);
    }

    void tickArp()
    {
        if (! arpWasOn) { arpWasOn = true; arpCounter = 0; }
        for (size_t k = 0; k < arpOffs.size(); ++k)
            if (arpOffs[k] >= 0 && --arpOffs[k] < 0) noteOff (arpIds[k]);

        if (--arpCounter > 0) return;
        const int jump = (arpRng.nextBool() ? -1 : 1) * (1 + arpRng.nextInt (5));
        arpStep = ((arpStep + jump) % 17 + 17) % 17;
        const int id = kArpId + (arpN++ % 64);
        const double f = stepFreq (arpStep, params.tuning, params.octave) * (arpRng.nextFloat() < 0.12f ? 2.0 : 1.0);
        noteOn (id, f, 0.55f + arpRng.nextFloat() * 0.4f, 1.0f, false, arpStep);
        const auto rateSamples = (int64_t) (params.rate * 0.001 * sr);
        for (size_t k = 0; k < arpOffs.size(); ++k)
            if (arpOffs[k] < 0) { arpOffs[k] = rateSamples * 7 / 10; arpIds[k] = id; break; }
        arpCounter = rateSamples * (arpRng.nextFloat() < 0.18f ? 2 : 1);
    }

    void stopArp()
    {
        if (! arpWasOn) return;
        arpWasOn = false;
        for (size_t k = 0; k < arpOffs.size(); ++k)
            if (arpOffs[k] >= 0) { noteOff (arpIds[k]); arpOffs[k] = -1; }
    }

    float shape (float x) const
    {
        const float k = params.lyse * 40.0f;
        return (1.0f + k) * x / (1.0f + k * std::abs (x));
    }

    void renderSample (float& outL, float& outR)
    {
        float bus = 0.0f;
        for (auto& v : voices)
            if (v.active) bus += v.render (params, sr);

        if (killActive)
        {
            const double t = killT;
            const float fc = (float) (7000.0 * std::pow (90.0 / 7000.0, juce::jmin (1.0, t / 1.2)));
            killFilter.set (fc, 9.0f, (float) sr);
            float g = t < 0.02 ? (float) (t / 0.02 * 0.5) : (float) (0.5 * std::pow (0.002, (t - 0.02) / 1.28));
            bus += killFilter.process (killRng.nextFloat() * 2.0f - 1.0f, 1) * g;
            killT += 1.0 / sr;
            if (killT > 1.4) killActive = false;
        }

        const float shaped = shape (bus * 0.5f);

        // Echo mit leicht schwankender Zeit und Bandpass im Rückweg
        wobblePh = wrap (wobblePh + 0.13 / sr);
        const double delaySec = 0.37 + 0.006 * std::sin (kTwoPi * wobblePh);
        const double readPos = writePos - delaySec * sr;
        const int size = (int) delayBuf.size();
        double rp = readPos < 0 ? readPos + size : readPos;
        const int i0 = (int) rp;
        const float frac = (float) (rp - i0);
        const float delayed = delayBuf[(size_t) (i0 % size)] * (1.0f - frac) + delayBuf[(size_t) ((i0 + 1) % size)] * frac;
        const float tone = echoTone.process (delayed, 1);
        delayBuf[(size_t) writePos] = shaped * params.echo + tone * params.feedback;
        writePos = (writePos + 1) % size;

        // Hall
        float rl = shaped * 0.22f, rr = rl;
        reverb.processStereo (&rl, &rr, 1);

        float l = shaped + tone + rl;
        float r = shaped + tone + rr;
        l = comp.processSample (0, l);
        r = comp.processSample (1, r);

        const float vol = volSmoothed.getNextValue();
        outL = l * vol;
        outR = r * vol;
    }
};
} // namespace bkz
