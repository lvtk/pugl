
#include <juce/juce.h>
#include <jlv2/jlv2.h>

using namespace juce;

class LV2Show : public JUCEApplication,
                public AsyncUpdater
{
public:
    LV2Show() { }

    const String getApplicationName() override       { return "LV2 Show"; }
    const String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override       { return false; }

    void initialise (const String& cli) override
    {
        auto* lv2 = new jlv2::LV2PluginFormat();
        plugins.addFormat (lv2); // takes ownership

        if (cli.isEmpty())
        {
            for (const auto& uri : lv2->searchPathsForPlugins (lv2->getDefaultLocationsToSearch(), true, false))
                Logger::writeToLog (uri);

            quit();
            return;
        }

        PluginDescription desc;
        desc.pluginFormatName = "LV2";
        desc.fileOrIdentifier = cli;

        String message;
        if (auto* instance = plugins.createPluginInstance (desc, 48000.0, 1024, message))
        {
            plugin.reset (instance);
            
            player.setProcessor (plugin.get());
            devices.initialiseWithDefaultDevices (2 ,2);
            devices.addAudioCallback (&player);
            devices.addMidiInputCallback (String(), &player);

            AudioDeviceManager::AudioDeviceSetup setup;
            devices.getAudioDeviceSetup (setup);
            
            DBG(setup.inputDeviceName);
            DBG(setup.outputDeviceName);
            DBG(devices.getCurrentAudioDevice()->getInputChannelNames().size());
            DBG(devices.getCurrentAudioDevice()->getOutputChannelNames().size());

            DBG(plugin->getName());

            AudioProcessorEditor* editor = nullptr;
            if (plugin->hasEditor())
                editor = plugin->createEditorIfNeeded();
            else
                editor = new GenericAudioProcessorEditor (instance);
            
            window.reset (new PluginWindow (*plugin));
            window->setUsingNativeTitleBar (true);
            window->setName (plugin->getName());
            window->setContentOwned (editor, true);
            window->centreWithSize (window->getWidth(), window->getHeight());
            window->setResizable (true, false);
            window->setVisible (true);
        }
        else
        {
            Logger::writeToLog (String("lv2show: ") + message);
            setApplicationReturnValue (1);
            quit();
        }
    }

    void shutdown() override
    {
        player.setProcessor (nullptr);
        devices.closeAudioDevice();
        devices.removeAudioCallback (&player);
        devices.removeMidiInputCallback (String(), &player);
        
        window.reset();
        plugin.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
        ignoreUnused (commandLine);
    }

    void handleAsyncUpdate() override
    {
       
    }

private:
    AudioDeviceManager devices;
    AudioProcessorPlayer player;
    AudioPluginFormatManager plugins;
    std::unique_ptr<DocumentWindow> window;
    std::unique_ptr<AudioPluginInstance> plugin;

    class PluginWindow : public DocumentWindow
    {
    public:
        PluginWindow (AudioProcessor& p)
            : DocumentWindow ("plugin", Colours::black, DocumentWindow::allButtons, true),
              processor (p)
        {
            menu.reset (new MenuBar (*this));
            setMenuBar (menu.get());
        }

        ~PluginWindow()
        {
            setMenuBar (nullptr);
            menu.reset();
        }
        
        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        AudioProcessor& processor;
        friend class MenuBar;
        struct MenuBar : public MenuBarModel
        {
            PluginWindow& window;
            MenuBar (PluginWindow& owner) : window (owner) {}
            
            StringArray getMenuBarNames() override
            {
                return { "File", "Presets" };
            }

            PopupMenu getMenuForIndex (int, const String& name) override
            {
                PopupMenu menu;
                if (name == "File")
                {
                    menu.addItem (1, "Save JUCE State");
                    menu.addItem (2, "Restore JUCE state");
                }
                else if (name == "Presets")
                {
                    menu.addItem (1, "Save LV2 Preset");
                    menu.addSeparator();
                    // list presets here
                }
                return menu;
            }

            void menuItemSelected (int item, int menu) override
            {
                if (menu == 0)
                {
                    if (item == 1)
                    {
                        // FileChooser chooser ("Save state to file",
                        //     File::getSpecialLocation(File::userDocumentsDirectory),
                        //     {});
                        
                        // if (chooser.browseForFileToSave (true))
                        {
                            File file (File::getSpecialLocation(File::userDesktopDirectory)
                                .getChildFile ("teststate.jlv2"));
                            MemoryBlock block;
                            window.processor.getStateInformation (block);
                            MemoryInputStream mi (block, false);
                            FileOutputStream stream (file);
                            if (stream.openedOk())
                            {
                                stream.setPosition (0);
                                stream.truncate();
                                stream.writeFromInputStream (mi, -1);
                                stream.flush();
                            }
                            else
                            {
                                DBG("couldn't write state file");
                            }
                            
                        }
                    }
                    else if (item ==2)
                    {
                          // FileChooser chooser ("Load state from file",
                        //     File::getSpecialLocation(File::userDocumentsDirectory),
                        //     "*.jlv2");
                        
                        // if (chooser.browseForFileToOpen())
                        {
                            File file (File::getSpecialLocation(File::userDesktopDirectory)
                                .getChildFile ("teststate.jlv2"));
                            FileInputStream stream (file);
                            if (stream.openedOk())
                            {
                                MemoryOutputStream mo;
                                mo.writeFromInputStream (stream, -1);
                                window.processor.setStateInformation (mo.getData(), (int) mo.getDataSize());
                            }
                            else
                            {
                                DBG("couldn't open state file");
                            }
                        }
                    }
                }
                else if (menu == 2)
                {
                    if (item == 1)
                    {
                      
                    }
                }
            }
        };

        std::unique_ptr<MenuBar> menu;
    };
};

START_JUCE_APPLICATION (LV2Show)
