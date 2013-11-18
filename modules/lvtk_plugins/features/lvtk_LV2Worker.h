
#ifndef LVTK_JUCE_LV2WORKER_H
#define LVTK_JUCE_LV2WORKER_H

class LV2Worker :  public LV2Feature,
                   public Worker
{
public:
    
    LV2Worker (WorkThread& thread, uint32 bufsize,
               LV2_Handle handle = nullptr,
               LV2_Worker_Interface* iface = nullptr);
    
    ~LV2Worker();
    
    void setInterface (LV2_Handle handle, LV2_Worker_Interface* iface);
    
    const String& getURI() const;
    const LV2_Feature* getFeature() const;
    
    void endRun();
    void processRequest (uint32 size, const void* data);
    void processResponse (uint32 size, const void* data);
    
private:
    
    String uri;
    LV2_Worker_Interface* worker;
    LV2_Handle plugin;
    LV2_Worker_Schedule data;
    LV2_Feature feat;
    
};


#endif /* LVTK_JUCE_LV2WORKER_H */
