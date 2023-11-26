#include "WorkManager.h"
#include "WinHooker.h"
#include <windows.h>
#include <unordered_map>
#include <iostream>

#include "handler-os/HandlerOS.h"
#include "handler-os/Message.h"

using namespace hw32;
using HOS = h7_handler_os::HandlerOS;
using Message = h7_handler_os::Message;
using Handler = h7_handler_os::Handler;

#define _ITEM_ID_UNKNOWN -1

#define _LOG(fmt, ...) \
    do{char buf[1024];\
    snprintf(buf, 1024, fmt, ##__VA_ARGS__);\
    std::cout << buf << std::endl;\
    }while(0);

#define _LOGE(fmt, ...) \
    do{char buf[1024];\
    snprintf(buf, 1024, fmt, ##__VA_ARGS__);\
    std::cerr << buf << std::endl;\
    }while(0);

#define ASSERT(t)\
    if(!(t)){\
        std::cerr <<__FUNCTION__ << ": "<< #t << std::endl;\
        abort();\
    }

enum {
    kMsg_ATTACH_INPUT,
    kMsg_KEY_EVENT,
    kMsg_MOUSE_EVENT,
};

static void _keyCallback0(KeysEvent* ke, bool act);

namespace hw32{
using ItemMap = std::unordered_map<int, WorkItem>;
struct _WorkManager_ctx{
    WinHooker hooker;
    ItemMap itemMap;
    HOS* attachHos {nullptr};
    HOS* workHos {nullptr};
    SpEventCollector eventCollector;

    _WorkManager_ctx(CWorkItems cs){
        //reg event.
        int size = cs.size();
        for(int i = 0 ; i < size ; ++i){
            itemMap[cs[i].id] = cs[i];
        }
        attachHos = new HOS();
        attachHos->start([this](Message* m){
            //
            switch (m->what) {
            case kMsg_ATTACH_INPUT:{
                hooker.attachInput();
            }break;

            case kMsg_KEY_EVENT:{
                bool down = (m->arg1 & 0xffff) != 0;
                int itemId = (m->arg1 & 0xffff0000) >> 16;
                sendKeyEvent(itemId, (u32)m->arg2, down);
            }break;

            case kMsg_MOUSE_EVENT:{
                bool absolute = (m->arg1 & 0xffff) != 0;
                int itemId = (m->arg1 & 0xffff0000) >> 16;
                int dy = m->arg2 & 0xffffffff;
                int dx = (m->arg2 & 0xffffffff00000000) >> 32;
                sendMouseEvent(itemId, dx, dy, absolute);
            }break;

            default:
            {
                _LOGE("attachHos >> unknown msg.what = %d", m->what);
            }
            }
            return true;
        });
        workHos = new HOS();
        workHos->start([this](Message* m){
            //TODO
            return true;
        });
    }
    ~_WorkManager_ctx(){
        attachHos->quit();
    }
    auto getWorkHandler(){
        return workHos->getHandler();
    }
    auto getAttachHandler(){
        return attachHos->getHandler();
    }
    void start(CString name, bool utf8){
        if(utf8){
            hooker.getWindowInfoUtf8(name);
        }else{
            hooker.getWindowInfo(name);
        }
        ASSERT(hooker.winInfo.handle != nullptr);
        getAttachHandler()->sendEmptyMessage(kMsg_ATTACH_INPUT);
        //reg event.
        auto it = itemMap.begin();
        for(; it != itemMap.end() ; ++it){
            auto& wi = it->second;
            auto ptr = std::make_shared<KeysEvent>();
            //fixed keys and F1-F12
            ptr->vkCodes = {wi.vkCodes.begin(), wi.vkCodes.end()};
            ptr->vkCodes.insert(VK_F1 + (u32)wi.id);  //left: tab + alt + F1-F12
            ptr->cb = _keyCallback0;
            ptr->context = this;
            ptr->what = wi.id;
            hooker.regKeysEvent(ptr);
        }
        //start listen
        hooker.startHookKeyboard();
    }
    void act(WorkItem* wi){
        auto& en = wi->entry;
        int size = en.size();
        auto& rect = hooker.winInfo.winRect;
        for(int i = 0 ; i < size ; ++i){
            auto& it = en[i];
            if(it.keyboard){
                sendKeyEventMsg(wi->id, it.vkCode, it.down);
            }else{
                sendMouseEventMsg(wi->id,rect.left + it.dx, rect.top + it.dy, it.absolute);
            }
        }
    }

    void sendKeyEventMsg(int itemId, u32 vkCode, bool down){
        int arg1 = ((unsigned int)itemId << 16) + (down ? 1 : 0);
        //
        auto h = getAttachHandler();
        auto msg = Message::obtain(h.get(), kMsg_KEY_EVENT, arg1, vkCode);
        h->sendMessage(msg);
    }
    void sendMouseEventMsg(int itemId,int dx, int dy, bool absolute){
        int arg1 = ((unsigned int)itemId << 16) + (absolute ? 1 : 0);
        long long dxy = ((long long)dx << 32) + dy;
        auto h = getAttachHandler();
        auto msg = Message::obtain(h.get(), kMsg_MOUSE_EVENT, arg1, dxy);
        h->sendMessage(msg);
    }
    //vkCode: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    bool sendKeyEvent(int itemId,u32 vkCode, bool down){
        if(eventCollector){
            eventCollector->onKeyEvent(itemId, vkCode, down);
            return true;
        }
        using namespace std;
        INPUT inputs[1] = {};
        ZeroMemory(inputs, sizeof(inputs));
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = vkCode;

        if (!down){
            inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
        }
        UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        if (uSent != ARRAYSIZE(inputs))
        {
            _LOGE("sendKeyEvent >> SendInput failed: 0x%x, itemId = %d",
                  HRESULT_FROM_WIN32(GetLastError()), itemId);
            return false;
        }
        else
        {
            return true;
        }
    }
    //https://blog.csdn.net/weixin_62972527/article/details/128378538
    bool sendMouseEvent(int itemId, int dx, int dy, bool absolute) {
        if(eventCollector){
            eventCollector->onMouseEvent(itemId, dx, dy, absolute);
            return true;
        }
        using namespace std;
        INPUT inputs[1] = {};
        ZeroMemory(inputs, sizeof(inputs));
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dx = dx;
        inputs[0].mi.dy = dy;
        inputs[0].mi.mouseData = 0;
        if (absolute){
            inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
        }else{
            inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE;
        }
        UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        if (uSent != ARRAYSIZE(inputs))
        {
            _LOGE("sendMouseEvent >> SendInput failed: 0x%x, itemId = %d",
                  HRESULT_FROM_WIN32(GetLastError()), itemId);
            return false;
        }
        return false;
    }
};
};

void _keyCallback0(KeysEvent* ke, bool act){
    _LOGE("======== > _keyCallback0 >> act = %d", act);
    _WorkManager_ctx* wc = (_WorkManager_ctx*)ke->context;
    if(act){
        auto wi = &wc->itemMap[ke->what];
        wc->getWorkHandler()->post([wc, wi](){
            wc->act(wi);
        });
    }
}
WorkManager::WorkManager(const WorkItem& item){
    std::vector<WorkItem> vec = {item};
    m_ptr = new _WorkManager_ctx(vec);
}
WorkManager::WorkManager(CWorkItems cs)
{
    m_ptr = new _WorkManager_ctx(cs);
}
WorkManager::~WorkManager()
{
    if(m_ptr){
        delete m_ptr;
        m_ptr = nullptr;
    }
}

void WorkManager::start(CString name, bool utf8){
    m_ptr->start(name, utf8);
}
void WorkManager::startAsync(CString name, bool utf8){
    std::thread thd([this, name, utf8](){
        m_ptr->start(name, utf8);
    });
    thd.detach();
}

void WorkManager::sendKeyEvent(u32 vkCode, bool down){
    m_ptr->sendKeyEventMsg(_ITEM_ID_UNKNOWN, vkCode, down);
}
void WorkManager::sendMouseEvent(int dx, int dy, bool absolute){
    m_ptr->sendMouseEventMsg(_ITEM_ID_UNKNOWN, dx, dy, absolute);
}
void WorkManager::setEventCollector(SpEventCollector coll){
    m_ptr->eventCollector = coll;
}
