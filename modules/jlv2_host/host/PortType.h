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

#pragma once

#ifndef JLV2_INVALID_CHANNEL
 #define JLV2_INVALID_CHANNEL -1
#endif

#ifndef JLV2_INVALID_PORT
 #define JLV2_INVALID_PORT (uint32)-1
#endif

#ifndef JLV2_INVALID_NODE
 #define JLV2_INVALID_NODE JLV2_INVALID_PORT
#endif

namespace jlv2 {

/** The type of a port. */
class PortType {
public:
    enum ID
    {
        Control = 0,
        Audio   = 1,
        CV      = 2,
        Atom    = 3,
        Event   = 4,
        Midi    = 5,
        Unknown = 6
    };

    PortType (const Identifier& identifier)
        : type (typeForString (identifier.toString())) { }
    
    PortType (const String& identifier)
        : type (typeForString (identifier)) { }
                
    PortType (ID id) : type(id) { }

    PortType (const int t) : type (static_cast<ID> (t)) { }

    PortType (const PortType& o)
    {
        type = o.type;
    }

    /** Get a URI string for this port type */
    inline const String& getURI()  const { return typeURI (type); }
                
    /** Get a human readable name for this port type */
    inline const String& getName() const { return typeName (type); }
                
    /** Get a slug version of the port type */
    inline const String& getSlug() const { return slugName (type); }
    /** Get a slug version of the port type */
    inline static const String& getSlug (const int t) { return slugName (static_cast<unsigned> (t)); }

    /** Get the port type id. This is useful in switch statements */
    inline ID               id()   const { return type; }

    inline PortType& operator= (const int& t) {
        jassert (t >= PortType::Control && t <= PortType::Unknown);
        type = static_cast<ID> (t);
        return *this;
    }

    inline PortType& operator= (const PortType& o)
    {
        type = o.type;
        return *this;
    }

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

    /** Returns true if the @p type is in range */
    template<typename IT>
    inline static bool isValidType (const IT& type) 
    {
        return type >= PortType::Control && type < PortType::Unknown;
    }

private:
    /** @internal */
    static inline const String& typeURI (unsigned id)
    {
        jassert (id <= Midi);

        static const String uris[] = {
            String ("http://lv2plug.in/ns/lv2core#ControlPort"),
            String ("http://lv2plug.in/ns/lv2core#AudioPort"),
            String ("http://lv2plug.in/ns/lv2core#CVPort"),
            String ("http://lv2plug.in/ns/lv2core#AtomPort"),
            String ("http://lv2plug.in/ns/lv2core#EventPort"),
            String ("http://lvtoolkit.org/ns/lvtk#MidiPort"),
            String ("http://lvtoolkit.org/ns/lvtk#null")
        };

        return uris [id];
    }

    /** @internal */
    static inline const String& typeName (unsigned id)
    {
        jassert (id <= Midi);
        
        static const String uris[] = {
            String ("Control"),
            String ("Audio"),
            String ("CV"),
            String ("Atom"),
            String ("Event"),
            String ("MIDI"),
            String ("Unknown")
        };

        return uris [id];
    }

    /** @internal */
    static inline const String& slugName (unsigned id)
    {
        jassert (id <= Midi);
        static const String slugs[] = {
            String ("control"),
            String ("audio"),
            String ("cv"),
            String ("atom"),
            String ("event"),
            String ("midi"),
            String ("unknown")
        };
        return slugs [id];
    }
    
    static inline ID typeForString (const String& identifier)
    {
        for (int i = 0; i <= Midi; ++i)
        {
            if (slugName(i) == identifier || typeURI(i) == identifier ||
                typeName(i) == identifier)
            {
                return static_cast<ID> (i);
            }
        }
        return Unknown;
    }

    ID type;
};

/** Maps channel numbers to a port indexes for all port types. This is an attempt
    to handle boiler-plate port to channel mapping functions */
class ChannelMapping
{
public:
    inline ChannelMapping() { init(); }

    /** Maps an array of port types sorted by port index, to channels */
    inline ChannelMapping (const Array<PortType>& types)
    {
        init();

        for (int port = 0; port < types.size(); ++port)
            addPort (types.getUnchecked (port), (uint32) port);
    }

    inline void clear()
    {
        for (int i = 0; i < ports.size(); ++i)
            ports.getUnchecked(i)->clearQuick();
    }

    /** Add (append) a port to the map */
    inline void addPort (PortType type, uint32 index)
    {
        ports.getUnchecked(type)->add (index);
    }

    inline bool containsChannel (const PortType type, const int32 channel) const
    {
        if (type == PortType::Unknown)
            return false;

        const Array<uint32>* const a (ports.getUnchecked (type));
        return a->size() > 0 && isPositiveAndBelow (channel, a->size());
    }

    int32  getNumChannels (const PortType type) const { return ports.getUnchecked(type)->size(); }
    uint32 getNumPorts    (const PortType type) const { return ports.getUnchecked(type)->size(); }

    /** Get a port index for a channel */
    inline uint32 getPortChecked (const PortType type, const int32 channel) const
    {
        if (! containsChannel (type, channel))
            return JLV2_INVALID_PORT;

        const Array<uint32>* const a (ports.getUnchecked (type));
        return a->getUnchecked (channel);
    }

    const Array<uint32>& getPorts (const PortType type) const { return *ports.getUnchecked (type); }

    inline uint32 getPort (const PortType type, const int32 channel) const
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

    inline void init()
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

    inline void addPort (const PortType type, const uint32 port, const bool isInput)
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

    inline int32 getNumChannels (const PortType type, bool isInput) const
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

/** A detailed descption of a port */
struct PortDescription
{
    PortDescription() { }
    PortDescription (int32 portType, int32 portIndex, int32 portChannel, 
                     const String& portSymbol, const String& portName, 
                     const bool isInput)
        : type (portType), index (portIndex), channel (portChannel),
          symbol (portSymbol), name (portName), input (isInput) { }

    int32   type     { 0 };
    int32   index    { 0 };
    int32   channel  { 0 };
    String  symbol   { };
    String  name     { };
    bool    input    { false };
};

struct PortIndexComparator
{
    static int compareElements (const PortDescription* const first, const PortDescription* const second)
    {
        return (first->index < second->index) ? -1 
            : ((second->index < first->index) ? 1
            : 0);
    }
};

class PortList
{
public:
    PortList() = default;
    ~PortList()
    {
        ports.clear();
    }

    inline void clear() { ports.clear(); }
    inline void clearQuick() { ports.clearQuick (true); }
    inline int size() const { return ports.size(); }
    inline int size (int type, bool input) const
    {
        int n = 0;
        for (const auto* port : ports)
            if (port->type == type && port->input == input)
                ++n;
        return n;
    }

    inline void add (PortDescription* port)
    {
        jassert (port != nullptr);
        jassert (port->type >= PortType::Control && port->type < PortType::Unknown);
        jassert (nullptr == findByIndexInternal (port->index));
        jassert (nullptr == findByChannelInternal (port->type, port->channel, port->input));
        PortIndexComparator sorter;
        ports.addSorted (sorter, port);
    }

    inline void add (int32 type, int32 index, int32 channel, 
                     const String& symbol, const String& name,
                     const bool input)
    {
        add (new PortDescription (type, index, channel, symbol, name, input));
    }

    template<typename IT>
    const PortDescription* get (const IT& index) const
    {
        return findByIndexInternal (static_cast<int> (index));
    }

    inline int getChannelForPort (const int port) const
    {
        if (auto* const desc = findByIndexInternal (port))
            return desc->channel;
        return JLV2_INVALID_CHANNEL;
    }

    inline int getPortForChannel (int type, int channel, bool input) const
    {
        if (auto* const desc = findByChannelInternal (type, channel, input))
            return desc->index;
        return static_cast<int> (JLV2_INVALID_PORT);
    }

    inline int getType (const int port) const
    {
        if (auto* const desc = findByIndexInternal (port))
            return desc->type;
        return PortType::Unknown;
    }

    inline bool isInput (const int port, const bool defaultRet = false) const
    {
        if (auto* const desc = findByIndexInternal (port))
            return desc->input;
        return defaultRet;
    }
    inline bool isOutput (const int port, const bool defaultRet = true) const {
        return ! isInput (port, defaultRet);
    }

    inline const OwnedArray<PortDescription>& getPorts() const { return ports; }
    inline void swapWith (PortList& o) { ports.swapWith (o.ports); }

private:
    OwnedArray<PortDescription> ports;

    inline PortDescription* findByIndexInternal (int index) const
    {
        for (auto* port : ports)
            if (port->index == index)
                return port;
        return nullptr;
    }

    inline PortDescription* findBySymbolInternal (const String& symbol) const
    {
        for (auto* port : ports)
            if (port->symbol == symbol)
                return port;
        return nullptr;
    }

    inline PortDescription* findByChannelInternal (int type, int channel, bool isInput) const
    {
        for (auto* port : ports)
            if (port->type == type && port->channel == channel && port->input == isInput)
                return port;
        return nullptr;
    }

#if JUCE_MODULE_AVAILABLE_juce_data_structures
public:
    inline ValueTree createValueTree (const int port) const
    {
        if (const auto* desc = findByIndexInternal (port))
        {
            ValueTree data ("port");
            data.setProperty ("index",     desc->index, nullptr)
                .setProperty ("channel",   desc->channel, nullptr)
                .setProperty ("type",      PortType::getSlug (desc->type), nullptr)
                .setProperty ("input",     desc->input, nullptr)
                .setProperty ("name",      desc->name, nullptr)
                .setProperty ("symbol",    desc->symbol, nullptr);
            return data;
        }

        return ValueTree();
    }
#endif
};

}
