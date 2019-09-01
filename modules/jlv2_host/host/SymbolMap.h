/*
    This file is part of jlv2
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

namespace jlv2 {

#if EL_ENABLE_CXX11
/** A function type for mapping uris */
typedef std::function<LV2_URID(const char*)> URIMapFunction;
/** A function type for unmapping urids */
typedef std::function<const char*(LV2_URID)> URIUnmapFunction;
#endif

/** Maintains a map of Strings/Symbols to integers
    This class also implements LV2 URID Map/Unmap features and is fully
    compatible with the current LV2 (1.6.0+) specification. */
class SymbolMap
{
public:
    /** Create an empty symbol map and initialized LV2 URID features */
    SymbolMap() = default;

    ~SymbolMap()
    {
        clear();
    }

    /** Map a symbol/uri to an unsigned integer
        @param key The symbol to map
        @return A mapped URID, a return of 0 indicates failure */
    inline LV2_URID map (const char* key)
    {
        if (! contains (key))
        {
            const uint32_t urid (1 + (LV2_URID) mapped.size());
            mapped [key] = urid;
            unmapped [urid] = std::string (key);
            return urid;
        }

        return mapped [key];
    }

    /** Containment test of a URI
        @param uri The URI to test
        @return True if found */
    inline bool contains (const char* uri)
    {
        return mapped.find (uri) != mapped.end();
    }

    /** Containment test of a URID
        @param urid The URID to test
        @return True if found */
    inline bool contains (LV2_URID urid) const
    {
        return unmapped.find (urid) != unmapped.end();
    }

    /** Unmap an already mapped id to its symbol
        @param urid The URID to unmap
        @return The previously mapped symbol or 0 if the urid isn't in the cache */
    inline const char* unmap (LV2_URID urid)
    {
        if (contains (urid))
            return (const char*) unmapped [urid].c_str();

        return "";
    }

    /** Clear the SymbolMap */
    inline void clear()
    {
        mapped.clear();
        unmapped.clear();
    }

    /** Create a URID Map LV2Feature. Thie created feature MUST be deleted
        before the SymbolMap is deleted */
    inline LV2Feature*  createMapFeature() { return new MapFeature (this); }

    /** Create a URID Unmap LV2Feature. Thie created feature MUST be deleted
        before the SymbolMap is deleted */
    inline LV2Feature*  createUnmapFeature() { return new UnmapFeature (this); }

private:
    friend class MapFeature;
    friend class UnmapFeature;
    typedef std::map<std::string, LV2_URID> Mapped;
    typedef std::map<LV2_URID, std::string> Unmapped;
    Mapped mapped; Unmapped unmapped;

    inline static LV2_URID _map (LV2_URID_Map_Handle handle, const char* uri)
    {
        SymbolMap* sym = reinterpret_cast<SymbolMap*> (handle);
        return sym->map (uri);
    }

    inline static const char* _unmap (LV2_URID_Map_Handle handle, LV2_URID urid)
    {
        SymbolMap* sym = (SymbolMap*) handle;
        return sym->unmap (urid);
    }
    
    // LV2 URID Host Implementation follows ...
    class MapFeature :  public LV2Feature
    {
    public:
        MapFeature (SymbolMap *parent)
        {
            uri = LV2_URID__map;
            feat.URI    = uri.toRawUTF8();
            data.handle = (void*) parent;
            data.map    = &SymbolMap::_map;
            feat.data   = &data;
        }

        virtual ~MapFeature() { }

        const String& getURI() const { return uri; }
        const LV2_Feature* getFeature() const { return &feat; }

    private:
        String       uri;
        LV2_Feature  feat;
        LV2_URID_Map data;
        friend class SymbolMap;
    };

    class UnmapFeature : public LV2Feature
    {
    public:
        UnmapFeature (SymbolMap* parent)
        {
            uri = LV2_URID__unmap;
            feat.URI    = uri.toRawUTF8();
            data.handle = (void*) parent;
            data.unmap  = &SymbolMap::_unmap;
            feat.data   = &data;
        }

        virtual ~UnmapFeature() { }
        const String& getURI() const { return uri; }
        const LV2_Feature* getFeature() const { return &feat; }

    private:
        String         uri;
        LV2_Feature    feat;
        LV2_URID_Unmap data;

        friend class SymbolMap;
    };

};

}
