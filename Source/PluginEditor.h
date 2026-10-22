#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

// Das Plugin-Fenster zeigt die HTML-Oberfläche (UI/index.html) in einer eingebetteten Web-Ansicht.
// Die Oberfläche schickt Ereignisse (Regler, Tasten, Abstrich, Bakterizid) an das Plugin,
// das Plugin schickt 30-mal pro Sekunde Parameterwerte, Oszilloskop und neue Noten zurück.
class BakterizidEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit BakterizidEditor (BakterizidProcessor&);
    ~BakterizidEditor() override;

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url) const;
    juce::WebBrowserComponent::Options makeOptions();

    void onReady();
    void onParam (const juce::var& payload);
    void onGesture (const juce::var& payload);
    void onProgram (const juce::var& payload);
    void onNote (const juce::var& payload);
    void onSwab (const juce::var& payload);

    BakterizidProcessor& proc;
    juce::WebBrowserComponent web;

    bool ready = false;
    std::map<juce::String, float> lastSent;
    int lastProgram = -1;
    std::vector<float> scopeHistory = std::vector<float> (512, 0.0f);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakterizidEditor)
};
