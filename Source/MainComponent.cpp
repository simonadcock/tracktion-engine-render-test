#include "MainComponent.h"

class ExtendedUIBehaviour : public tracktion_engine::UIBehaviour
{
public:
    ExtendedUIBehaviour() = default;

    void runTaskWithProgressBar(tracktion_engine::ThreadPoolJobWithProgress& job) override
    {
        // This is a simple implementation that just runs the job without a UI.
        while (! job.shouldExit())
        {
            job.runJob();
        }
    }
};

//==============================================================================
MainComponent::MainComponent():engine("Render Test",
                                      std::make_unique<ExtendedUIBehaviour>(),
                                      std::make_unique<tracktion_engine::EngineBehaviour>())
{
    
    selectFileButton.addListener(this);
    playButton.addListener(this);
    renderButton.addListener(this);
    renderButton2.addListener(this);
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
    
    const juce::File editFile (juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("edit"));
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
    selectFileButton.setBounds(getWidth() / 2 - 50, getHeight() / 2 - 20, 150, 40);
    playButton.setBounds(getWidth() / 2 - 50, (getHeight() / 2 - 20)+50, 150, 40);
    renderButton.setBounds(getWidth() / 2 - 50, (getHeight() / 2 - 20)+100, 150, 40);
    renderButton2.setBounds(getWidth() / 2 - 50, (getHeight() / 2 - 20)+150, 150, 40);
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
    }else if(button == &renderButton2)
    {
        renderToFileNoParams();
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
    addAndMakeVisible(renderButton2);
}

void MainComponent::play()
{
    transport = &edit->getTransport();
    if(transport->isPlaying())
    {
        transport->stop(false,true);
    } else
    {
        transport->play(false);
    }
}

void MainComponent::renderToFile()
{
    juce::File outputFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("output.wav");
    if(!outputFile.hasWriteAccess()){
        DBG("No write access");
        return;
    }
    auto tracks = tracktion_engine::getAllTracks(*edit);
    auto bitset = tracktion_engine::toBitSet(tracks);
    tracktion::Renderer::Parameters params (*edit);
    params.destFile = outputFile;
    params.time = tracktion::TimeRange(tracktion::TimePosition::fromSeconds(0), tracktion::TimePosition::fromSeconds(4));
    params.audioFormat = engine.getAudioFileFormatManager().getWavFormat();
    params.tracksToDo = bitset;
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

void MainComponent::renderToFileNoParams()
{
    juce::File outputFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("output.wav");
    if(!outputFile.hasWriteAccess()){
        DBG("No write access");
        return;
    }
    //auto success = tracktion::Renderer::renderToFile(*edit, outputFile, false); // this works too
    auto timeRange = tracktion::TimeRange(tracktion::TimePosition::fromSeconds(0), tracktion::TimePosition::fromSeconds(4));
    
    juce::BigInteger bitset;
    bitset.setBit (0);
        
    auto success = tracktion::Renderer::renderToFile({}, outputFile, *edit, timeRange, bitset, true, true, {}, false);
    if (success)
    {
        DBG("Render successful. File saved to: " << outputFile.getFullPathName());
    }
    else
    {
        DBG("Render failed or file was not created.");
    }
}
