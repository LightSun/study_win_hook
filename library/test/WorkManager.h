#pragma once

#include <memory>
#include "work_config.h"

namespace hw32 {

typedef struct _WorkManager_ctx _WorkManager_ctx;


struct EventCollector{
    virtual void onKeyEvent(int itemId, u32 vkCode, bool down) = 0;
    virtual void onMouseEvent(int itemId, int dx, int dy, bool absolute) = 0;
};

using SpEventCollector = std::shared_ptr<EventCollector>;

class WorkManager
{
public:
    WorkManager(CWorkItems cs);
    WorkManager(const WorkItem& cs);
    ~WorkManager();

    void start(CString name, bool utf8 = true);
    void startAsync(CString name, bool utf8 = true);

    void sendKeyEvent(u32 vkCode, bool down);
    void sendMouseEvent(int dx, int dy, bool absolute);

    //used for debug
    void setEventCollector(SpEventCollector coll);
    template<typename T>
    void setEventCollector(){
        setEventCollector(std::make_shared<T>());
    }

private:
    _WorkManager_ctx* m_ptr;
};

}
