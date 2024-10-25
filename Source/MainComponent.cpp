#include "MainComponent.h"


//==============================================================================
MainComponent::MainComponent():engine("Render Test")
{
    
    selectFileButton.addListener(this);
    playButton.addListener(this);
    renderButton.addListener(this);
    addAndMakeVisible(selectFileButton);
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
    
    const auto editFilePath = juce::JUCEApplication::getCommandLineParameters().replace ("-NSDocumentRevisionsDebugMode YES", "").unquoted().trim();
    const juce::File editFile (editFilePath);
    edit = tracktion_engine::createEmptyEdit (engine, editFile);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    selectFileButton.setBounds(getWidth() / 2 - 50, getHeight() / 2 - 20, 100, 40);
    playButton.setBounds(getWidth() / 2 - 50, (getHeight() / 2 - 20)+50, 100, 40);
    renderButton.setBounds(getWidth() / 2 - 50, (getHeight() / 2 - 20)+100, 100, 40);
}

void MainComponent::buttonClicked(juce::Button* button)
{
    if (button == &selectFileButton)
    {
        // Open the file chooser asynchronously
        fileChooser = std::make_unique<juce::FileChooser>("Select an audio file...",
                                                          juce::File{},
                                                          "*.wav");

        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                 [this](const juce::FileChooser& chooser)
                                 {
                                     auto file = chooser.getResult();
                                     if (file.existsAsFile())
                                     {
                                         loadAudioFile(file);
                                     }
                                 });
    } else if(button == &playButton)
    {
        play();
    } else if(button == &renderButton)
    {
        renderToFile();
    }
}

void MainComponent::loadAudioFile(const juce::File& file)
{
    DBG("Selected file: " << file.getFullPathName());
    
    edit->ensureNumberOfAudioTracks(1);
    auto track = tracktion_engine::getAudioTracks(*edit)[0];
    tracktion_engine::AudioFile audioFile(edit->engine, file);
    auto newClip = track->insertWaveClip(file.getFileNameWithoutExtension(), file, { { {}, tracktion::TimeDuration::fromSeconds(audioFile.getLength()) }, {} }, false);
    
    addAndMakeVisible(playButton);
    addAndMakeVisible(renderButton);
    
}

void MainComponent::play()
{
    transport = &edit->getTransport();
    transport->play(false);
}

void MainComponent::renderToFile()
{
    juce::File outputFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("output.wav");
    if(!outputFile.hasWriteAccess()){return;}
    tracktion::Renderer::Parameters params (*edit);
    params.destFile = outputFile;
    params.time = tracktion::TimeRange(tracktion::TimePosition::fromSeconds(0), tracktion::TimePosition::fromSeconds(4));
    params.audioFormat = engine.getAudioFileFormatManager().getWavFormat();
    juce::File renderedFile = tracktion::Renderer::renderToFile("Exporting audio", params);
    if (renderedFile.existsAsFile())
    {
        DBG("Render successful. File saved to: " << renderedFile.getFullPathName());
    }
    else
    {
        DBG("Render failed or file was not created.");
    }
}
