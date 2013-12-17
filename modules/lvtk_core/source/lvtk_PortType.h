/*
    This file is part of the lvtk_core module for the JUCE Library
    Copyright (c) 2013 - Michael Fisher <mfisher31@gmail.com>.

    Permission to use, copy, modify, and/or distribute this software for any purpose with
    or without fee is hereby granted, provided that the above copyright notice and this
    permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
    TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
    NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
    IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
    CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef LVTK_JUCE_PORTTYPE_H
#define LVTK_JUCE_PORTTYPE_H

/** The type of a port. */
class PortType {
public:

    enum ID {
        Control = 0,
        Audio   = 1,
        CV      = 2,
        Atom    = 3,
        Event   = 4,
        Midi    = 5,
        Unknown = 6
    };

    explicit PortType (const String& uri)
        : type (Unknown)
    {
        if (uri == typeURI (Audio))
            type = Audio;
        else if (uri == typeURI (Control))
            type = Control;
        else if (uri == typeURI (CV))
            type = CV;
        else if (uri == typeURI (Atom))
            type = Atom;
    }

    PortType (ID id) : type(id) { }

    /** Get a URI string for this port type */
    inline const String& getURI()  const { return typeURI (type); }
    /** Get a human readable name for this port type */
    inline const String& getName() const { return typeName (type); }
    /** Get the port type id. This is useful in switch statements */
    inline ID               id()   const { return type; }

    inline bool operator== (const ID& id) const { return (type == id); }
    inline bool operator!= (const ID& id) const { return (type != id); }
    inline bool operator== (const PortType& t) const { return (type == t.type); }
    inline bool operator!= (const PortType& t) const { return (type != t.type); }
    inline bool operator<  (const PortType& t) const { return (type < t.type); }

    inline operator int() const { return (int) this->type; }
    
    inline bool isAudio()   const { return type == Audio; }
    inline bool isControl() const { return type == Control; }
    inline bool isCv()      const { return type == CV; }
    inline bool isAtom()    const { return type == Atom; }
    inline bool isMidi()    const { return type == Midi; }
    inline bool isEvent()   const { return type == Event; }

    /** Return true if two port types can connect to one another */
    static inline bool canConnect (const PortType& sourceType, const PortType& destType)
    {
        if (sourceType == PortType::Unknown || destType == PortType::Unknown)
            return false;

        if (sourceType == destType)
            return true;

        if (sourceType == PortType::Audio && destType == PortType::CV)
            return true;

        if (sourceType == PortType::Control && destType == PortType::CV)
            return true;

        return false;
    }

    /** Return true if this port type can connect to another
        @param other The other port type
        @param isOutput Set true if 'this' is the output (source) type */
    inline bool canConnect (const PortType& other, bool isOutput = true) const
    {
        const bool res = isOutput ? canConnect (*this, other) : canConnect (other, *this);
        return res;
    }

    
private:

    /** @internal */
    static inline const String&
    typeURI (unsigned id)
    {
        assert (id <= Atom);
        
        static const String uris[] = {
            String (LV2_CORE__ControlPort),
            String (LV2_CORE__AudioPort),
            String (LV2_CORE__CVPort),
            String (LV2_ATOM__AtomPort),
            String (LV2_EVENT__EventPort),
            String ("http://lvtoolkit.org/ns#null")
        };
        
        return uris [id];
    }

    /** @internal */
    static inline const String&
    typeName (unsigned id)
    {
        assert (id <= Atom);
        static const String uris[] = {
            String ("Control"),
            String ("Audio"),
            String ("CV"),
            String ("Atom"),
            String ("Unknown")
        };
        return uris [id];
    }

    ID type;
};

/** Maps channel numbers to a port indexes for all port types. This is an attempt
 to handle boiler-plate port to channel mapping functions */
class ChannelMapping {
public:
    
    inline ChannelMapping() { init(); }
    
    /** Maps an array of port types sorted by port index, to channels */
    inline ChannelMapping (const Array<PortType>& types)
    {
        init();
        
        for (uint32 port = 0; port < types.size(); ++port)
            addPort (types.getUnchecked (port), port);
    }
    
    inline void
    clear()
    {
        for (int i = 0; i < ports.size(); ++i)
            ports.getUnchecked(i)->clearQuick();
    }
    
    /** Add (append) a port to the map */
    inline void
    addPort (PortType type, uint32 index)
    {
        ports.getUnchecked(type)->add (index);
    }
    
    inline bool
    containsChannel (const PortType type, const int32 channel) const
    {
        if (type == PortType::Unknown)
            return false;
        
        const Array<uint32>* const a (ports.getUnchecked (type));
        return a->size() > 0 && isPositiveAndBelow (channel, a->size());
    }
    
    int32  getNumChannels (const PortType type) const { return ports.getUnchecked(type)->size(); }
    uint32 getNumPorts    (const PortType type) const { return ports.getUnchecked(type)->size(); }
    
    /** Get a port index for a channel */
    inline uint32
    getPortChecked (const PortType type, const int32 channel) const
    {
        if (! containsChannel (type, channel))
            return LV2UI_INVALID_PORT_INDEX;
        
        const Array<uint32>* const a (ports.getUnchecked (type));
        return a->getUnchecked (channel);
    }
    
    const Array<uint32>& getPorts (const PortType type) const { return *ports.getUnchecked (type); }
    
    inline uint32
    getPort (const PortType type, const int32 channel) const
    {
        return ports.getUnchecked(type)->getUnchecked(channel);
    }
    
    inline uint32 getAtomPort    (const int32 channel) const { return ports.getUnchecked(PortType::Atom)->getUnchecked(channel); }
    inline uint32 getAudioPort   (const int32 channel) const { return ports.getUnchecked(PortType::Audio)->getUnchecked(channel); }
    inline uint32 getControlPort (const int32 channel) const { return ports.getUnchecked(PortType::Control)->getUnchecked(channel); }
    inline uint32 getCVPort      (const int32 channel) const { return ports.getUnchecked(PortType::CV)->getUnchecked(channel); }
    inline uint32 getEventPort   (const int32 channel) const { return ports.getUnchecked(PortType::Event)->getUnchecked(channel); }
    inline uint32 getMidiPort    (const int32 channel) const { return ports.getUnchecked(PortType::Midi)->getUnchecked(channel); }
    
    
private:
    
    // owned arrays of arrays....
    OwnedArray<Array<uint32> > ports;
    
    inline void
    init()
    {
        ports.ensureStorageAllocated (PortType::Unknown + 1);
        for (int32 p = 0; p <= PortType::Unknown; ++p)
            ports.add (new Array<uint32> ());
    }
};

/** Contains two ChannelMappings.  One for inputs and one for outputs */
class ChannelConfig
{
public:
    
    ChannelConfig()  { }
    ~ChannelConfig() { }
    
    inline void
    addPort (const PortType type, const uint32 port, const bool isInput)
    {
        ChannelMapping& mapping = isInput ? inputs : outputs;
        mapping.addPort (type, port);
    }
    
    inline void addInput  (const PortType type, const uint32 port) { inputs.addPort (type, port); }
    inline void addOutput (const PortType type, const uint32 port) { outputs.addPort (type, port); }
    
    inline const ChannelMapping& getChannelMapping (const bool isInput) const { return isInput ? inputs : outputs; }
    inline const ChannelMapping& getInputs()  const { return inputs; }
    inline const ChannelMapping& getOutputs() const { return outputs; }
    
    inline uint32 getPort (PortType type, int32 channel, bool isInput) const { return getChannelMapping(isInput).getPort (type, channel); }
    inline uint32 getInputPort  (const PortType type, const int32 channel) const { return inputs.getPort (type, channel); }
    inline uint32 getOutputPort (const PortType type, const int32 channel) const { return outputs.getPort (type, channel); }
    
    inline uint32 getAtomPort (int32 channel, bool isInput) const { return getChannelMapping(isInput).getAudioPort(channel); }
    inline uint32 getAudioPort (int32 channel, bool isInput) const { return getChannelMapping(isInput).getAudioPort(channel); }
    inline uint32 getControlPort (int32 channel, bool isInput) const { return getChannelMapping(isInput).getAudioPort(channel); }
    inline uint32 getCVPort (int32 channel, bool isInput) const { return getChannelMapping(isInput).getAudioPort(channel); }
    
    inline uint32 getAudioInputPort    (const int32 channel) const { return inputs.getAudioPort (channel); }
    inline uint32 getAudioOutputPort   (const int32 channel) const { return outputs.getAudioPort (channel); }
    inline uint32 getControlInputPort  (const int32 channel) const { return inputs.getControlPort (channel); }
    inline uint32 getControlOutputPort (const int32 channel) const { return outputs.getControlPort (channel); }
    
    inline int32
    getNumChannels (const PortType type, bool isInput) const
    {
        return isInput ? inputs.getNumChannels (type) : outputs.getNumChannels (type);
    }
    
    inline int32 getNumAtomInputs()     const { return inputs.getNumChannels (PortType::Atom); }
    inline int32 getNumAtomOutputs()    const { return outputs.getNumChannels(PortType::Atom); }
    inline int32 getNumAudioInputs()    const { return inputs.getNumChannels (PortType::Audio); }
    inline int32 getNumAudioOutputs()   const { return outputs.getNumChannels(PortType::Audio); }
    inline int32 getNumControlInputs()  const { return inputs.getNumChannels (PortType::Control); }
    inline int32 getNumControlOutputs() const { return outputs.getNumChannels(PortType::Control); }
    inline int32 getNumCVInputs()       const { return inputs.getNumChannels (PortType::CV); }
    inline int32 getNumCVOutputs()      const { return outputs.getNumChannels(PortType::CV); }
    inline int32 getNumEventInputs()    const { return inputs.getNumChannels (PortType::Event); }
    inline int32 getNumEventOutputs()   const { return outputs.getNumChannels(PortType::Event); }
    
private:
    
    ChannelMapping inputs, outputs;
    
};

/** A detailed descption of a port. (not used currently) */
struct PortDescription
{
    PortDescription() : index(0), isInput (false), type (PortType::Unknown) { }
    uint32      index;
    String      symbol;
    bool        isInput;
    uint32      type;
};

#endif /* LVTK_JUCE_PORTTYPE_H */