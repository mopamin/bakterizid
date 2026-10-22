#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
using Range = juce::NormalisableRange<float>;

// Logarithmischer Regelweg für Frequenzen und Zeiten
Range logRange (float min, float max)
{
    return Range (min, max,
                  [] (float a, float b, float n) { return a * std::pow (b / a, n); },
                  [] (float a, float b, float v) { return std::log (v / a) / std::log (b / a); });
}

juce::String fmtDec (float v, int d) { return juce::String (v, d).replace (".", ","); }
juce::String fmtMs (float v)  { return v >= 1.0f ? fmtDec (v, 2) + " s" : juce::String (juce::roundToInt (v * 1000.0f)) + " ms"; }
juce::String fmtHz (float v)  { return v >= 1000.0f ? fmtDec (v / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; }
juce::String fmtPct (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
} // namespace

const juce::StringArray& BakterizidProcessor::paramIds()
{
    static const juce::StringArray ids { "ratio", "index", "ring", "mutation", "cutoff", "q", "lyse",
                                         "attack", "decay", "sustain", "release", "penv", "noise",
                                         "echo", "feedback", "rate", "vol", "ftype", "octave", "tuning", "arp" };
    return ids;
}

juce::AudioProcessorValueTreeState::ParameterLayout BakterizidProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using A = juce::AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> ps;

    auto add = [&ps] (const char* id, const char* name, Range range, float def, std::function<juce::String (float)> fmt)
    {
        ps.push_back (std::make_unique<P> (juce::ParameterID { id, 1 }, name, range, def,
                                           A().withStringFromValueFunction ([fmt] (float v, int) { return fmt (v); })));
    };

    add ("ratio",    "Kultur (FM-Verhältnis)",      Range (0.25f, 8.0f, 0.01f),  3.17f, [] (float v) { return fmtDec (v, 2) + " x"; });
    add ("index",    "Toxizität (FM-Index)",        Range (0.0f, 14.0f, 0.1f),   6.0f,  [] (float v) { return fmtDec (v, 1); });
    add ("ring",     "Plasmid (Ringmodulation)",    Range (0.0f, 1.0f, 0.01f),   0.45f, fmtPct);
    add ("mutation", "Mutation (Drift)",            Range (0.0f, 200.0f, 1.0f),  40.0f, [] (float v) { return "+/- " + juce::String (juce::roundToInt (v)) + " ct"; });
    add ("cutoff",   "Durchlass (Filter)",          logRange (80.0f, 12000.0f),  3200.0f, fmtHz);
    add ("q",        "Resonanz",                    Range (0.5f, 24.0f, 0.1f),   12.0f, [] (float v) { return "Q " + fmtDec (v, 1); });
    add ("lyse",     "Lyse (Sättigung)",            Range (0.0f, 1.0f, 0.01f),   0.3f,  fmtPct);
    add ("attack",   "Latenz (Attack)",             logRange (0.001f, 2.0f),     0.02f, fmtMs);
    add ("decay",    "Zerfall (Decay)",             logRange (0.01f, 6.0f),      1.2f,  fmtMs);
    add ("sustain",  "Haftung (Sustain)",           Range (0.0f, 1.0f, 0.01f),   0.65f, fmtPct);
    add ("release",  "Absterben (Release)",         logRange (0.03f, 6.0f),      1.4f,  fmtMs);
    add ("penv",     "Schock (Tonhöhen-Hüllkurve)", Range (0.0f, 48.0f, 1.0f),   0.0f,  [] (float v) { return "+" + juce::String (juce::roundToInt (v)) + " HT"; });
    add ("noise",    "Sporen (Rauschen)",           Range (0.0f, 1.0f, 0.01f),   0.0f,  fmtPct);
    add ("echo",     "Generationen (Echo)",         Range (0.0f, 1.0f, 0.01f),   0.4f,  fmtPct);
    add ("feedback", "Vererbung (Echo-Rückkopplung)", Range (0.0f, 0.9f, 0.01f), 0.6f,  fmtPct);
    add ("rate",     "Teilung (Arpeggiator-Takt)",  Range (70.0f, 900.0f, 1.0f), 180.0f, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; });
    add ("vol",      "Pegel",                       Range (0.0f, 1.0f, 0.01f),   0.8f,  [] (float v) { return v < 0.005f ? juce::String ("-inf dB") : fmtDec (20.0f * std::log10 (v / 0.8f), 1) + " dB"; });

    ps.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "ftype", 1 }, "Filtertyp",
                                                                juce::StringArray { "Tiefpass", "Bandpass", "Hochpass" }, 0));
    ps.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "octave", 1 }, "Oktave", -2, 2, 0));
    ps.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "tuning", 1 }, "Stimmung",
                                                                juce::StringArray { "Terra 12", "Xeno 19", "Bohlen-Pierce", "Obertonreihe" }, 0));
    ps.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "arp", 1 }, "Zellteilung", false));

    return { ps.begin(), ps.end() };
}

BakterizidProcessor::BakterizidProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Bakterizid", createLayout())
{
    setCurrentProgram (bkz::startPresetIndex());
}

bool BakterizidProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void BakterizidProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
}

bkz::Params BakterizidProcessor::readParams() const
{
    auto f = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    bkz::Params p;
    p.ratio = f ("ratio"); p.index = f ("index"); p.ring = f ("ring"); p.mutation = f ("mutation");
    p.cutoff = f ("cutoff"); p.q = f ("q"); p.lyse = f ("lyse");
    p.attack = f ("attack"); p.decay = f ("decay"); p.sustain = f ("sustain"); p.release = f ("release");
    p.penv = f ("penv"); p.noise = f ("noise"); p.echo = f ("echo"); p.feedback = f ("feedback");
    p.rate = f ("rate"); p.vol = f ("vol");
    p.ftype = (int) f ("ftype"); p.octave = (int) f ("octave"); p.tuning = (int) f ("tuning");
    p.arp = f ("arp") > 0.5f;
    return p;
}

void BakterizidProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    engine.process (buffer, midi, readParams());
}

int BakterizidProcessor::getNumPrograms()
{
    return (int) bkz::presets().size();
}

void BakterizidProcessor::setCurrentProgram (int index)
{
    const auto& list = bkz::presets();
    if (index < 0 || index >= (int) list.size()) return;
    currentProgram.store (index);

    auto set = [this] (const char* id, float value)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };
    for (const auto& v : bkz::kBase) set (v.id, v.value);
    for (const auto& v : list[(size_t) index].values) set (v.id, v.value);
}

const juce::String BakterizidProcessor::getProgramName (int index)
{
    const auto& list = bkz::presets();
    return index >= 0 && index < (int) list.size() ? juce::String::fromUTF8 (list[(size_t) index].name) : juce::String();
}

void BakterizidProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BakterizidProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram.store ((int) state.getProperty ("program", bkz::startPresetIndex()));
            apvts.replaceState (state);
        }
    }
}

juce::AudioProcessorEditor* BakterizidProcessor::createEditor()
{
    return new BakterizidEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BakterizidProcessor();
}
