#include "PluginEditor.h"
#include "BakterizidUI.h"

namespace
{
juce::var object (std::initializer_list<std::pair<const char*, juce::var>> props)
{
    auto* o = new juce::DynamicObject();
    for (const auto& [k, v] : props) o->setProperty (k, v);
    return juce::var (o);
}

const char* mimeFor (const juce::String& file)
{
    if (file.endsWithIgnoreCase (".html")) return "text/html";
    if (file.endsWithIgnoreCase (".ttf"))  return "font/ttf";
    if (file.endsWithIgnoreCase (".js"))   return "text/javascript";
    if (file.endsWithIgnoreCase (".css"))  return "text/css";
    return "application/octet-stream";
}
} // namespace

juce::WebBrowserComponent::Options BakterizidEditor::makeOptions()
{
    using Opt = juce::WebBrowserComponent::Options;
    return Opt {}
        .withBackend (Opt::Backend::webview2)
        .withWinWebView2Options (Opt::WinWebView2 {}
                                     .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Bakterizid"))
                                     .withBackgroundColour (juce::Colour (0xff150b10))
                                     .withStatusBarDisabled())
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const juce::String& url) { return getResource (url); })
        .withEventListener ("ready",   [this] (juce::var)            { onReady(); })
        .withEventListener ("param",   [this] (juce::var p)          { onParam (p); })
        .withEventListener ("gesture", [this] (juce::var p)          { onGesture (p); })
        .withEventListener ("program", [this] (juce::var p)          { onProgram (p); })
        .withEventListener ("note",    [this] (juce::var p)          { onNote (p); })
        .withEventListener ("swab",    [this] (juce::var p)          { onSwab (p); })
        .withEventListener ("kill",    [this] (juce::var)            { proc.engine.uiEvents.push ({ bkz::UiEvent::Kill }); });
}

BakterizidEditor::BakterizidEditor (BakterizidProcessor& p)
    : AudioProcessorEditor (p), proc (p), web (makeOptions())
{
    addAndMakeVisible (web);
    setResizable (true, true);
    setResizeLimits (540, 360, 2160, 1440);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio (1080.0 / 720.0);
    setSize (1080, 720);
    web.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    startTimerHz (30);
}

BakterizidEditor::~BakterizidEditor()
{
    stopTimer();
}

void BakterizidEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff150b10));
}

void BakterizidEditor::resized()
{
    web.setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> BakterizidEditor::getResource (const juce::String& url) const
{
    auto file = url.upToFirstOccurrenceOf ("?", false, false).fromLastOccurrenceOf ("/", false, false);
    if (file.isEmpty()) file = "index.html";

    for (int i = 0; i < BakterizidUI::namedResourceListSize; ++i)
    {
        if (file == BakterizidUI::originalFilenames[i])
        {
            int size = 0;
            const auto* data = BakterizidUI::getNamedResource (BakterizidUI::namedResourceList[i], size);
            std::vector<std::byte> bytes ((size_t) size);
            std::memcpy (bytes.data(), data, (size_t) size);
            return juce::WebBrowserComponent::Resource { std::move (bytes), mimeFor (file) };
        }
    }
    return std::nullopt;
}

void BakterizidEditor::onReady()
{
    juce::Array<juce::var> presetList;
    for (const auto& pr : bkz::presets())
        presetList.add (object ({ { "cat", juce::String::fromUTF8 (pr.category) },
                                  { "name", juce::String::fromUTF8 (pr.name) },
                                  { "stain", juce::String (pr.stain) } }));

    auto* params = new juce::DynamicObject();
    for (const auto& id : BakterizidProcessor::paramIds())
    {
        const float v = proc.apvts.getRawParameterValue (id)->load();
        params->setProperty (id, v);
        lastSent[id] = v;
    }
    lastProgram = proc.getCurrentProgram();

    web.emitEventIfBrowserIsVisible ("init", object ({ { "presets", presetList },
                                                       { "params", juce::var (params) },
                                                       { "program", lastProgram } }));
    ready = true;
}

void BakterizidEditor::onParam (const juce::var& payload)
{
    const auto id = payload.getProperty ("id", {}).toString();
    if (auto* p = proc.apvts.getParameter (id))
    {
        const float v = (float) payload.getProperty ("value", 0.0);
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        lastSent[id] = proc.apvts.getRawParameterValue (id)->load();   // nicht zurückschicken
    }
}

void BakterizidEditor::onGesture (const juce::var& payload)
{
    if (auto* p = proc.apvts.getParameter (payload.getProperty ("id", {}).toString()))
    {
        if ((bool) payload.getProperty ("on", false)) p->beginChangeGesture();
        else                                            p->endChangeGesture();
    }
}

void BakterizidEditor::onProgram (const juce::var& payload)
{
    proc.setCurrentProgram ((int) payload.getProperty ("index", 0));
    proc.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
}

void BakterizidEditor::onNote (const juce::var& payload)
{
    const float step = (float) (int) payload.getProperty ("s", 0);
    const bool on = (bool) payload.getProperty ("on", false);
    proc.engine.uiEvents.push ({ on ? bkz::UiEvent::NoteOn : bkz::UiEvent::NoteOff, step, 0.0f });
}

void BakterizidEditor::onSwab (const juce::var& payload)
{
    const auto phase = payload.getProperty ("phase", "move").toString();
    const float x = (float) payload.getProperty ("x", 0.0), y = (float) payload.getProperty ("y", 0.0);
    const auto type = phase == "on" ? bkz::UiEvent::SwabOn : phase == "off" ? bkz::UiEvent::SwabOff : bkz::UiEvent::SwabMove;
    proc.engine.uiEvents.push ({ type, x, y });
}

void BakterizidEditor::timerCallback()
{
    // Oszilloskop-Daten immer abholen, damit der Puffer nicht volläuft
    std::vector<float> fresh;
    proc.engine.scope.popAll ([&fresh] (float v) { fresh.push_back (v); });
    std::vector<int> notes;
    proc.engine.noteEvents.popAll ([&notes] (int s) { notes.push_back (s); });

    if (! ready) return;

    if (fresh.size() >= scopeHistory.size())
        std::copy (fresh.end() - (long) scopeHistory.size(), fresh.end(), scopeHistory.begin());
    else if (! fresh.empty())
    {
        std::move (scopeHistory.begin() + (long) fresh.size(), scopeHistory.end(), scopeHistory.begin());
        std::copy (fresh.begin(), fresh.end(), scopeHistory.end() - (long) fresh.size());
    }

    // Geänderte Parameter (z. B. durch Automation in der DAW) an die Oberfläche
    auto* changed = new juce::DynamicObject();
    for (const auto& id : BakterizidProcessor::paramIds())
    {
        const float v = proc.apvts.getRawParameterValue (id)->load();
        if (! juce::exactlyEqual (lastSent[id], v)) { changed->setProperty (id, v); lastSent[id] = v; }
    }
    const juce::var changedVar (changed);
    if (changed->getProperties().size() > 0)
        web.emitEventIfBrowserIsVisible ("params", changedVar);

    juce::Array<juce::var> scopeArr;
    float peak = 0.0f;
    for (size_t i = 0; i < scopeHistory.size(); i += 2)
    {
        scopeArr.add (std::round (scopeHistory[i] * 1000.0f) / 1000.0f);
        peak = juce::jmax (peak, std::abs (scopeHistory[i]));
    }
    juce::Array<juce::var> noteArr;
    for (int s : notes) noteArr.add (s);

    web.emitEventIfBrowserIsVisible ("state", object ({ { "scope", scopeArr },
                                                        { "peak", peak },
                                                        { "voices", proc.engine.activeVoices.load() },
                                                        { "notes", noteArr },
                                                        { "kill", proc.engine.killFlash.exchange (false) },
                                                        { "program", proc.getCurrentProgram() } }));
}
