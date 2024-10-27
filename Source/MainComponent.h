#pragma once

#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent, public juce::Button::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;

private:
    tracktion_engine::Engine engine;
    std::unique_ptr<tracktion_engine::Edit> edit;
    tracktion_engine::TransportControl* transport = nullptr;
    
    juce::TextButton selectFileButton { "Select Audio File" };
    juce::TextButton playButton { "Play" };
    juce::TextButton renderButton { "Render to File" };
    juce::TextButton renderButton2 { "Render to File No Params" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    void loadAudioFile(const juce::File& file);
    void play();
    void renderToFile();
    void renderToFileNoParams();


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
