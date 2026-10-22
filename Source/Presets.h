#pragma once

#include <vector>

// Factory-Presets. Jedes Preset setzt zuerst die Grundwerte (kBase) und überschreibt dann seine eigenen.
// ftype: 0 Tiefpass, 1 Bandpass, 2 Hochpass. octave: -2..+2. stain bestimmt die Farbe der Oberfläche.

namespace bkz
{
struct PresetValue { const char* id; float value; };

struct Preset
{
    const char* category;
    const char* name;
    const char* stain;   // pos, neg, spore, phage
    std::vector<PresetValue> values;
};

inline const std::vector<PresetValue> kBase {
    { "ratio", 2.0f }, { "index", 2.0f }, { "ring", 0.0f }, { "mutation", 0.0f },
    { "cutoff", 3000.0f }, { "q", 2.0f }, { "lyse", 0.3f },
    { "attack", 0.01f }, { "decay", 1.2f }, { "sustain", 0.65f }, { "release", 0.6f },
    { "penv", 0.0f }, { "noise", 0.0f }, { "echo", 0.2f }, { "feedback", 0.3f },
    { "ftype", 0.0f }, { "octave", 0.0f }
};

inline const std::vector<Preset>& presets()
{
    static const std::vector<Preset> list {
        { "Drums", "Kick · Sporenherz", "spore", { { "ratio", 1 }, { "index", .4f }, { "cutoff", 700 }, { "q", 1 }, { "lyse", .65f }, { "attack", .002f }, { "decay", .38f }, { "sustain", 0 }, { "release", .25f }, { "penv", 36 }, { "noise", .04f }, { "echo", 0 }, { "feedback", 0 }, { "octave", -2 } } },
        { "Drums", "Kick · Zellkern 808", "spore", { { "ratio", 1 }, { "index", .1f }, { "cutoff", 400 }, { "q", .7f }, { "lyse", .4f }, { "attack", .002f }, { "decay", 1.6f }, { "sustain", 0 }, { "release", 1 }, { "penv", 24 }, { "echo", 0 }, { "feedback", 0 }, { "octave", -2 } } },
        { "Drums", "Snare · Zellwand", "neg", { { "ratio", 1.48f }, { "index", 1.5f }, { "ring", .2f }, { "ftype", 1 }, { "cutoff", 2200 }, { "q", 1.2f }, { "lyse", .5f }, { "attack", .001f }, { "decay", .18f }, { "sustain", 0 }, { "release", .15f }, { "penv", 12 }, { "noise", .8f }, { "echo", .1f }, { "feedback", .15f } } },
        { "Drums", "Rim · Kapsid", "neg", { { "ratio", 3.5f }, { "index", 2 }, { "ftype", 1 }, { "cutoff", 1800 }, { "q", 6 }, { "attack", .001f }, { "decay", .05f }, { "sustain", 0 }, { "release", .06f }, { "penv", 20 }, { "noise", .3f }, { "echo", .1f }, { "octave", 1 } } },
        { "Drums", "Hi-Hat · Flagellum", "phage", { { "ratio", 7.07f }, { "index", 8 }, { "ring", .8f }, { "ftype", 2 }, { "cutoff", 7500 }, { "q", 1.5f }, { "attack", .001f }, { "decay", .05f }, { "sustain", 0 }, { "release", .05f }, { "noise", .9f }, { "echo", .05f }, { "octave", 1 } } },
        { "Drums", "Open Hat · Zilien", "phage", { { "ratio", 7.07f }, { "index", 8 }, { "ring", .8f }, { "ftype", 2 }, { "cutoff", 6800 }, { "q", 1.5f }, { "attack", .001f }, { "decay", .35f }, { "sustain", 0 }, { "release", .3f }, { "noise", .9f }, { "echo", .08f }, { "octave", 1 } } },
        { "Drums", "Tom · Vakuole", "spore", { { "ratio", 1 }, { "index", .8f }, { "ring", .1f }, { "cutoff", 1800 }, { "q", 3 }, { "attack", .001f }, { "decay", .32f }, { "sustain", 0 }, { "release", .25f }, { "penv", 14 }, { "noise", .1f }, { "echo", .1f }, { "octave", -1 } } },
        { "Drums", "Perc · Tropfen", "phage", { { "ratio", 2.76f }, { "index", 3 }, { "ring", .3f }, { "ftype", 1 }, { "cutoff", 2500 }, { "q", 10 }, { "attack", .001f }, { "decay", .09f }, { "sustain", 0 }, { "release", .1f }, { "penv", 24 }, { "echo", .35f }, { "feedback", .45f }, { "octave", 1 } } },
        { "Bass", "Bass · Quecksilberlarve", "pos", { { "ratio", 1 }, { "index", 2.2f }, { "ring", .15f }, { "mutation", 6 }, { "cutoff", 900 }, { "q", 9 }, { "lyse", .55f }, { "attack", .005f }, { "decay", .4f }, { "sustain", .55f }, { "release", .2f }, { "penv", 2 }, { "echo", .05f }, { "feedback", .2f }, { "octave", -1 } } },
        { "Bass", "Bass · Sub-Symbiont", "pos", { { "ratio", .5f }, { "index", .5f }, { "cutoff", 380 }, { "q", 1 }, { "lyse", .25f }, { "attack", .01f }, { "decay", 1 }, { "sustain", .9f }, { "release", .2f }, { "echo", 0 }, { "feedback", 0 }, { "octave", -2 } } },
        { "Bass", "Bass · Säurefraß", "neg", { { "ratio", 1 }, { "index", 1.2f }, { "cutoff", 600 }, { "q", 20 }, { "lyse", .7f }, { "attack", .003f }, { "decay", .25f }, { "sustain", .3f }, { "release", .15f }, { "echo", .15f }, { "feedback", .3f }, { "octave", -1 } } },
        { "Keys & Plucks", "Keys · Gram-positiv", "pos", { { "ratio", 2 }, { "index", 2.5f }, { "ring", .1f }, { "mutation", 12 }, { "cutoff", 1800 }, { "q", 4 }, { "lyse", .5f }, { "attack", .01f }, { "release", .8f }, { "echo", .25f }, { "feedback", .4f } } },
        { "Keys & Plucks", "Pluck · Kristallvirus", "neg", { { "ratio", 3.5f }, { "index", 5 }, { "ring", .2f }, { "mutation", 4 }, { "cutoff", 4500 }, { "q", 6 }, { "lyse", .2f }, { "attack", .001f }, { "decay", .35f }, { "sustain", 0 }, { "release", .5f }, { "echo", .45f }, { "feedback", .55f }, { "octave", 1 } } },
        { "Keys & Plucks", "Glocke · Kieselalge", "phage", { { "ratio", 3.53f }, { "index", 4 }, { "ring", .1f }, { "cutoff", 9000 }, { "q", 1 }, { "attack", .001f }, { "decay", 2.5f }, { "sustain", 0 }, { "release", 2.5f }, { "echo", .3f }, { "feedback", .4f }, { "octave", 1 } } },
        { "Leads", "Lead · Xenoglossie", "neg", { { "ratio", 1.5f }, { "index", 4 }, { "ring", .3f }, { "mutation", 30 }, { "ftype", 1 }, { "cutoff", 1400 }, { "q", 14 }, { "lyse", .5f }, { "attack", .02f }, { "decay", .6f }, { "sustain", .7f }, { "release", .4f }, { "echo", .35f }, { "feedback", .45f } } },
        { "Leads", "Lead · Geißelsäge", "neg", { { "ratio", 1 }, { "index", 3 }, { "mutation", 10 }, { "cutoff", 5000 }, { "q", 4 }, { "lyse", .6f }, { "attack", .01f }, { "decay", 1 }, { "sustain", .8f }, { "release", .3f }, { "echo", .25f }, { "feedback", .35f } } },
        { "Pads", "Pad · Myzel-Kommunikation", "phage", { { "ratio", 2.01f }, { "index", 1.8f }, { "ring", .25f }, { "mutation", 25 }, { "cutoff", 1600 }, { "q", 6 }, { "lyse", .15f }, { "attack", .9f }, { "decay", 2 }, { "sustain", .8f }, { "release", 3 }, { "noise", .08f }, { "echo", .5f }, { "feedback", .6f } } },
        { "Pads", "Pad · Biofilm", "spore", { { "ratio", 3.5f }, { "index", .8f }, { "ring", .1f }, { "mutation", 60 }, { "cutoff", 2400 }, { "q", 4 }, { "lyse", .1f }, { "attack", 1.5f }, { "decay", 3 }, { "sustain", .7f }, { "release", 4.5f }, { "noise", .3f }, { "echo", .6f }, { "feedback", .7f } } },
        { "Pads", "Pad · Plasma-Chor", "pos", { { "ratio", 1 }, { "index", 1.2f }, { "ring", .05f }, { "mutation", 18 }, { "ftype", 1 }, { "cutoff", 900 }, { "q", 8 }, { "attack", .6f }, { "decay", 2 }, { "sustain", .85f }, { "release", 2.5f }, { "noise", .05f }, { "echo", .45f }, { "feedback", .55f } } },
        { "Kulturen & FX", "Gram-negativ", "neg", { { "ratio", 3.17f }, { "index", 6 }, { "ring", .45f }, { "mutation", 40 }, { "cutoff", 3200 }, { "q", 12 }, { "lyse", .3f }, { "attack", .02f }, { "release", 1.4f }, { "echo", .4f }, { "feedback", .6f } } },
        { "Kulturen & FX", "Sporen", "spore", { { "ratio", .51f }, { "index", 9 }, { "ring", .7f }, { "mutation", 90 }, { "cutoff", 900 }, { "q", 18 }, { "lyse", .2f }, { "attack", .6f }, { "release", 3.5f }, { "echo", .55f }, { "feedback", .72f } } },
        { "Kulturen & FX", "Phage", "phage", { { "ratio", 7.07f }, { "index", 12 }, { "ring", .9f }, { "mutation", 160 }, { "cutoff", 5200 }, { "q", 20 }, { "lyse", .8f }, { "attack", .005f }, { "release", .5f }, { "echo", .3f }, { "feedback", .5f } } },
        { "Kulturen & FX", "Drone · Ereignishorizont", "pos", { { "ratio", .5f }, { "index", 3 }, { "ring", .5f }, { "mutation", 120 }, { "cutoff", 700 }, { "q", 16 }, { "attack", 1.2f }, { "decay", 4 }, { "sustain", .9f }, { "release", 6 }, { "echo", .5f }, { "feedback", .7f }, { "octave", -1 } } },
    };
    return list;
}

inline int startPresetIndex() { return 19; } // Gram-negativ
} // namespace bkz
