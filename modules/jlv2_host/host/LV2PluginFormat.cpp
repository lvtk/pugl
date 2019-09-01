/*
    Copyright (c) 2014-2019  Michael Fisher <mfisher@kushview.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

namespace jlv2 {

// Change this to enable logging of various LV2 activities
#ifndef LV2_LOGGING
 #define LV2_LOGGING 0
#endif

#if LV2_LOGGING
 #define JUCE_LV2_LOG(a) Logger::writeToLog(a);
#else
 #define JUCE_LV2_LOG(a)
#endif

class LV2AudioParameter : public AudioProcessorParameter
{
    Module& module;
    const uint32 portIdx;
    const String name;
    float defaultValue;
    Atomic<float> value;

public:
    LV2AudioParameter (uint32 port, Module& _module)
        : module (_module), 
          portIdx (port),
          name (module.getPortName (port))
    {
        module.getPortRange (port, range.start, range.end, defaultValue);
        value.set (getDefaultValue());
    }

    ~LV2AudioParameter() = default;

    uint32 getPort() const { return portIdx; }

    float getValue() const override
    {
        return value.get();
    }

    // update the value, but dont write to port
    void update (float newValue, bool notifyListeners = true)
    {
        newValue = range.convertTo0to1 (newValue);
        if (newValue == value.get())
            return;

        value.set (newValue);
        if (notifyListeners)
            sendValueChangedMessageToListeners (value.get());
    }

    /** Will write to Port with correct min max ratio conversion */
    void setValue (float newValue) override
    {
        value.set (newValue);
        const auto expanded = range.convertFrom0to1 (newValue);
        module.write (portIdx, sizeof(float), 0, &expanded);
    }

    float getDefaultValue() const override      { return range.convertTo0to1 (defaultValue); }
    String getName (int maxLen) const override  { return name.substring (0, maxLen); }

    // Units e.g. Hz
    String getLabel() const override { return {}; }

    String getText (float normalisedValue, int /*maximumStringLength*/) const override
    {
        return String (range.convertFrom0to1 (normalisedValue), 2);
    }

    /** Parse a string and return the appropriate value for it. */
    float getValueForText (const String& text) const override
    {
        return range.convertTo0to1 (text.getFloatValue());
    }

   #if 0
    virtual int getNumSteps() const;
    virtual bool isDiscrete() const;
    virtual bool isBoolean() const;
    virtual bool isOrientationInverted() const;
    virtual bool isAutomatable() const;
    virtual bool isMetaParameter() const;
    virtual Category getCategory() const;
    virtual String getCurrentValueAsText() const;
    virtual StringArray getAllValueStrings() const;
   #endif

private:
    NormalisableRange<float> range;
};

class LV2PluginInstance  : public AudioPluginInstance
{
public:
    LV2PluginInstance (World& world, Module* module_)
        : wantsMidiMessages (false),
          initialised (false),
          isPowerOn (false),
          tempBuffer (1, 1),
          module (module_)
    {
        LV2_URID_Map* map = nullptr;
        if (LV2Feature* feat = world.getFeatures().getFeature (LV2_URID__map))
            map = (LV2_URID_Map*) feat->getFeature()->data;

        jassert (map != nullptr);
        jassert (module != nullptr);

        atomSequence = module->map (LV2_ATOM__Sequence);
        midiEvent    = module->map (LV2_MIDI__MidiEvent);
        numPorts     = module->getNumPorts();
        midiPort     = module->getMidiPort();
        notifyPort   = module->getNotifyPort();

        for (uint32 p = 0; p < numPorts; ++p)
            if (module->isPortInput (p) && PortType::Control == module->getPortType (p))
                addParameter (new LV2AudioParameter (p, *module));    
 
        const ChannelConfig& channels (module->getChannelConfig());
        setPlayConfigDetails (channels.getNumAudioInputs(),
                              channels.getNumAudioOutputs(), 44100.0, 1024);

        if (! module->hasEditor())
        {
            jassert (module->onPortNotify == nullptr);
            using namespace std::placeholders;
            module->onPortNotify = std::bind (&LV2PluginInstance::portEvent, this, _1, _2, _3, _4);
        }
    }

    ~LV2PluginInstance()
    {
        module->onPortNotify = nullptr;
        module = nullptr;
    }

    void portEvent (uint32 port, uint32 size, uint32 protocol, const void* data)
    {
        if (protocol != 0)
            return;
        for (int i = 0; i < getParameters().size(); ++i)
            if (auto* const param = dynamic_cast<LV2AudioParameter*> (getParameters()[i]))
                if (port == param->getPort() && protocol == 0)
                    { param->update (*(float*) data, true); break; }
    };

    //=========================================================================
    void fillInPluginDescription (PluginDescription& desc) const
    {
        desc.name = module->getName();

        desc.descriptiveName = String();
        if (desc.descriptiveName.isEmpty())
            desc.descriptiveName = desc.name;

        desc.fileOrIdentifier = module->getURI();
        desc.uid = desc.fileOrIdentifier.hashCode();

//        desc.lastFileModTime = 0;
        desc.pluginFormatName = "LV2";

        desc.category = module->getClassLabel();
        desc.manufacturerName = module->getAuthorName();
        desc.version = "";

        desc.numInputChannels  = module->getNumPorts (PortType::Audio, true);
        desc.numOutputChannels = module->getNumPorts (PortType::Audio, false);
        desc.isInstrument = module->getMidiPort() != LV2UI_INVALID_PORT_INDEX;
    }

    void initialise()
    {
        if (initialised)
            return;

       #if JUCE_WINDOWS
        // On Windows it's highly advisable to create your plugins using the message thread,
        // because many plugins need a chance to create HWNDs that will get their
        // messages delivered by the main message thread, and that's not possible from
        // a background thread.
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
       #endif

        wantsMidiMessages = midiPort != LV2UI_INVALID_PORT_INDEX;

        initialised = true;
        setLatencySamples (0);
    }

    double getTailLengthSeconds() const { return 0.0f; }
    void* getPlatformSpecificData()  { return module->getHandle(); }
    const String getName() const     { return module->getName(); }
    bool silenceInProducesSilenceOut() const { return false; }
    bool acceptsMidi()  const        { return wantsMidiMessages; }
    bool producesMidi() const        { return notifyPort != LV2UI_INVALID_PORT_INDEX; }

    //==============================================================================
    void prepareToPlay (double sampleRate, int blockSize)
    {
        const ChannelConfig& channels (module->getChannelConfig());
        setPlayConfigDetails (channels.getNumAudioInputs(),
                              channels.getNumAudioOutputs(),
                              sampleRate, blockSize);
        initialise();

        if (initialised)
        {
            module->setSampleRate (sampleRate);
            tempBuffer.setSize (jmax (1, getTotalNumOutputChannels()), blockSize);
            module->activate();
        }
    }

    void releaseResources()
    {
        if (initialised)
            module->deactivate();

        tempBuffer.setSize (1, 1);
    }

    void processBlock (AudioSampleBuffer& audio, MidiBuffer& midi)
    {
        const int32 numSamples = audio.getNumSamples();

        if (! initialised)
        {
            for (int i = 0; i < getTotalNumOutputChannels(); ++i)
                audio.clear (i, 0, numSamples);

            return;
        }

        if (AudioPlayHead* const playHead = getPlayHead ())
        {
            AudioPlayHead::CurrentPositionInfo position;
            playHead->getCurrentPosition (position);

            if (position.isLooping)
            { }
            else
            { }
        }

        const ChannelConfig& chans (module->getChannelConfig());

        if (wantsMidiMessages)
        {
            PortBuffer* const buf = module->getPortBuffer (midiPort);
            buf->reset();
            MidiBuffer::Iterator iter (midi);
            const uint8* d = nullptr;  int s = 0, f = 0;
            while (iter.getNextEvent (d, s, f))
                buf->addEvent (static_cast<uint32> (f), static_cast<uint32> (s), midiEvent, d);
        }
        
        module->referAudioReplacing (audio);
        module->run ((uint32) numSamples);
        midi.clear();

        // if (notifyPort != LV2UI_INVALID_PORT_INDEX)
        // {
        //     PortBuffer* const buf = buffers.getUnchecked (notifyPort);
        //     jassert (buf != nullptr);

        //     midi.clear();
        //     LV2_ATOM_SEQUENCE_FOREACH ((LV2_Atom_Sequence*) buf->getPortData(), ev)
        //     {
        //         if (ev->body.type == uris->midi_MidiEvent)
        //         {
        //             midi.addEvent (LV2_ATOM_BODY_CONST (&ev->body),
        //                            ev->body.size, (int32)ev->time.frames);
        //         }
        //     }
        // }
        // else
        // {
        //     midi.clear();
        // }
    }

    bool hasEditor() const { return module->hasEditor(); }

    AudioProcessorEditor* createEditor();

    const String getInputChannelName (int index) const
    {
        const ChannelConfig& chans (module->getChannelConfig());
        if (! isPositiveAndBelow (index, chans.getNumAudioInputs()))
            return String ("Audio In ") + String (index + 1);
        return module->getPortName (chans.getAudioPort (index, true));
    }

    bool isInputChannelStereoPair (int index) const { return false; }

    const String getOutputChannelName (int index) const
    {
        const ChannelConfig& chans (module->getChannelConfig());
        if (! isPositiveAndBelow (index, chans.getNumAudioOutputs()))
            return String ("Audio Out ") + String (index + 1);

        return module->getPortName (chans.getAudioPort (index, false));
    }

    bool isOutputChannelStereoPair (int index) const { return false; }

    //==============================================================================
    int getNumPrograms()          { return 1; }
    int getCurrentProgram()       { return 0; }
    void setCurrentProgram (int /*index*/) { }
    const String getProgramName (int /*index*/)  { return String ("Default"); }
    void changeProgramName (int /*index*/, const String& /*name*/) { }

    //==============================================================================
    void getStateInformation (MemoryBlock& mb)
    {
        const auto state = module->getStateString();
        mb.append (state.toRawUTF8(), state.length());
    }

    void getCurrentProgramStateInformation (MemoryBlock& mb)    { ; }
    
    void setStateInformation (const void* data, int size)
    {
        MemoryInputStream stream (data, (size_t)size, false);
        module->setStateString (stream.readEntireStreamAsString());
    }
    
    void setCurrentProgramStateInformation (const void* data, int size) { ; }

    //==============================================================================
    void timerCallback() { }

    void handleAsyncUpdate()
    {
        // indicates that something about the plugin has changed..
        // updateHostDisplay();
    }

private:
    CriticalSection lock, midiInLock;
    bool wantsMidiMessages, initialised, isPowerOn;
    mutable StringArray programNames;

    AudioSampleBuffer tempBuffer;
    ScopedPointer<Module> module;
    OwnedArray<PortBuffer> buffers;

    uint32 numPorts;
    uint32 midiPort;
    uint32 notifyPort;
    uint32 atomSequence, midiEvent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LV2PluginInstance)
};

class LV2EditorJuce : public AudioProcessorEditor,
                      public Timer
{
public:
    LV2EditorJuce (LV2PluginInstance* p, ModuleUI::Ptr _ui)
        : AudioProcessorEditor (p),
          plugin (*p),
          ui (_ui)
    {
        setOpaque (true);

        if (ui && ui->isNative())
        {
           #if JUCE_MAC
            native.reset (new NSViewComponent());
           #elif JUCE_LINUX
            native.reset (new XEmbedComponent (true, false));
           #endif

            jassert (native);
            addAndMakeVisible (native.get());

            setSize (ui->getClientWidth() > 0 ?  ui->getClientWidth() : 240,
                     ui->getClientHeight() > 0 ? ui->getClientHeight() : 100);
            startTimerHz (60);
            setResizable (true, false);
        }
        else
        {
            widget.setNonOwned ((Component*) ui->getWidget());
            if (widget)
            {
                addAndMakeVisible (widget.get());
                setSize (widget->getWidth(), widget->getHeight());
            }
            else
            {
                jassertfalse;
                setSize (320, 180);
            }
        }
    }

    ~LV2EditorJuce()
    {
        if (ui && ui->isNative())
        {
           #if JUCE_MAC
            native->setView (nullptr);
           #elif JUCE_LINUX
           #endif
            native.reset();          
        }
        else
        {
            removeChildComponent (widget.get());
            widget.clear();
        }
        
        plugin.editorBeingDeleted (this);
        if (ui)
        {
            ui->unload();
        }
        ui = nullptr;
    }

    void timerCallback() override
    {
        if (!ui || !ui->isNative())
            return stopTimer();

        if (! nativeViewSetup)
        {
           #if JUCE_MAC
            if (auto* const peer = native->getPeer()) 
            {
                if (native->isVisible()) 
                {
                    ui->setParent ((intptr_t) peer->getNativeHandle());
                    ui->instantiate();
                    if (ui->loaded())
                    {
                        native->setView (ui->getWidget());
                        nativeViewSetup = true;
                    }
                }
            }
           #elif JUCE_LINUX
            ui->setParent ((intptr_t) native->getHostWindowID());
            ui->instantiate();
            nativeViewSetup = ui->loaded();
           #endif
        }

        if (nativeViewSetup)
        {
            if (ui->haveIdleInterface())
                ui->idle();
            else
                stopTimer();
        }
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
    }

    void resized() override
    {
        if (ui->isNative())
        {
            native->setBounds (getLocalBounds());
        }

        if (widget)
            widget->setBounds (0, 0, widget->getWidth(), widget->getHeight());
    }

private:
    LV2PluginInstance& plugin;
    ModuleUI::Ptr ui = nullptr;
    OptionalScopedPointer<Component> widget;
    bool nativeViewSetup = false;
   #if JUCE_MAC
    std::unique_ptr<NSViewComponent> native;
   #elif JUCE_WINDOWS
    std::unique_ptr<Component> native;
   #else
    std::unique_ptr<XEmbedComponent> native;
   #endif
};

AudioProcessorEditor* LV2PluginInstance::createEditor()
{
    jassert (module->hasEditor());
    ModuleUI::Ptr ui = module->hasEditor() ? module->createEditor() : nullptr;
    return ui != nullptr ? new LV2EditorJuce (this, ui) : nullptr;
}

//=============================================================================
class LV2PluginFormat::Internal
{
public:
    Internal()
    {
        useExternalData = false;
        world.setOwned (new World ());
        init();
    }

    Internal (World& w)
    {
        useExternalData = true;
        world.setNonOwned (&w);
        init();
    }

    ~Internal()
    {
        world.clear();
    }

    Module* createModule (const String& uri)
    {
        return world->createModule (uri);
    }

    OptionalScopedPointer<World> world;
    SymbolMap symbols;

private:
    bool useExternalData;
    void init() { }
};

//=============================================================================
LV2PluginFormat::LV2PluginFormat() : priv (new Internal()) { }
LV2PluginFormat::~LV2PluginFormat() { priv = nullptr; }

//=============================================================================
void LV2PluginFormat::findAllTypesForFile (OwnedArray <PluginDescription>& results,
                                           const String& fileOrIdentifier)
{
    if (! fileMightContainThisPluginType (fileOrIdentifier)) {
        return;
    }

    ScopedPointer<PluginDescription> desc (new PluginDescription());
    desc->fileOrIdentifier = fileOrIdentifier;
    desc->pluginFormatName = String ("LV2");
    desc->uid = 0;

    try
    {
        ScopedPointer<AudioPluginInstance> instance (createInstanceFromDescription (*desc.get(), 44100.0, 1024));
        if (LV2PluginInstance* const p = dynamic_cast <LV2PluginInstance*> (instance.get()))
        {
            p->fillInPluginDescription (*desc.get());
            results.add (desc.release());
        }
    }
    catch (...)
    {
        JUCE_LV2_LOG("crashed: " + String(desc->name));
    }
}

bool LV2PluginFormat::fileMightContainThisPluginType (const String& fileOrIdentifier)
{
    bool maybe = fileOrIdentifier.contains ("http:") ||
                 fileOrIdentifier.contains ("https:") ||
                 fileOrIdentifier.contains ("urn:");

    if (! maybe && File::isAbsolutePath (fileOrIdentifier))
    {
        const File file (fileOrIdentifier);
        maybe = file.getChildFile("manifest.ttl").existsAsFile();
    }

    return maybe;
}

String LV2PluginFormat::getNameOfPluginFromIdentifier (const String& fileOrIdentifier)
{
    const auto name = priv->world->getPluginName (fileOrIdentifier);
    return name.isEmpty() ? fileOrIdentifier : name;
}

StringArray LV2PluginFormat::searchPathsForPlugins (const FileSearchPath& paths, bool, bool)
{
    if (paths.getNumPaths() > 0)
    {
       #if JUCE_WINDOWS
        setenv ("LV2_PATH", paths.toString().toRawUTF8(), 0);
       #else
        setenv ("LV2_PATH", paths.toString().replace(";",":").toRawUTF8(), 0);
       #endif
    }

    StringArray list;
    priv->world->getSupportedPlugins (list);
    return list;
}

FileSearchPath LV2PluginFormat::getDefaultLocationsToSearch()
{
    FileSearchPath paths;
   #if JUCE_LINUX
    paths.add (File ("/usr/lib/lv2"));
    paths.add (File ("/usr/local/lib/lv2"));
   #elif JUCE_MAC
    paths.add (File ("/Library/Audio/Plug-Ins/LV2"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory)
        .getChildFile ("Library/Audio/Plug-Ins/LV2"));
   #endif
    return paths;
}

bool LV2PluginFormat::doesPluginStillExist (const PluginDescription& desc)
{
    StringArray plugins (searchPathsForPlugins (FileSearchPath(), true));
    return plugins.contains (desc.fileOrIdentifier);
}

void LV2PluginFormat::createPluginInstance (const PluginDescription& desc, double initialSampleRate,
                                            int initialBufferSize, void* userData,
                                            PluginCreationCallback callback)
{
    if (desc.pluginFormatName != String ("LV2"))
    {
        callback (userData, nullptr, "Not an LV2 plugin");
        return;
    }

    if (Module* module = priv->createModule (desc.fileOrIdentifier))
    {
        Result res (module->instantiate (initialSampleRate));
        if (res.wasOk())
        {
            auto instance = std::unique_ptr<LV2PluginInstance> (new LV2PluginInstance (*priv->world, module));
            callback (userData, instance.release(), {});
        }
        else
        {
            deleteAndZero (module);
            callback (userData, nullptr, res.getErrorMessage());
        }
    }
    else
    {
        JUCE_LV2_LOG ("Failed creating LV2 plugin instance");
        callback (userData, nullptr, "Failed creating LV2 plugin instance");
    }
}

}
