#include "WinHooker.h"
#include <windows.h>
#include <mutex>
#include <stdatomic.h>
#include <unordered_map>
#include <algorithm>
#include <stdio.h>
#include <iostream>
#include "handler-os/HandlerOS.h"
#include "handler-os/Message.h"

#include "LockH.h"
#include "_time.h"

using namespace hw32;
using HOS = h7_handler_os::HandlerOS;

//#define _LOGE(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)\

#define _LOGE(fmt, ...) \
    do{char buf[1024];\
    snprintf(buf, 1024, fmt, ##__VA_ARGS__);\
    std::cout << buf << std::endl;\
    }while(0);
#define _LOCK(m) std::unique_lock<std::mutex> lock(m)
#define _DEBUG 0

static inline String _codes2Str(std::unordered_set<unsigned long>& vec){
    String str;
    auto it = vec.begin();
    for(; it != vec.end(); ++it){
        str += std::to_string(*it) + "_";
    }
    return str;
}


static BOOL CALLBACK lpEnumFunc(HWND hwnd, LPARAM lParam);
static String Utf8ToGbk(const char *src_str);
static LRESULT CALLBACK LowLevelKeyboardProc(_In_ int nCode,_In_ WPARAM wParam,_In_ LPARAM lParam);
static LRESULT CALLBACK LowLevelMouseProc(_In_ int nCode,_In_ WPARAM wParam,_In_ LPARAM lParam);
static String _StrReverse(const char* str);

struct Holder{
    String gbk;
    String reverseGbk;
    WinHooker* wh;
    WindowInfo* winInfo;
};

#define _MSG_FOCUS_KEY 1
using Message = h7_handler_os::Message;
namespace hw32 {
int KeysEvent::getCodesHash(){
    if(_hash != 0){
        return _hash;
    }
    using u32 = unsigned long;
    std::vector<u32> vec = {vkCodes.begin(), vkCodes.end()};
    std::sort(vec.begin(), vec.end());
    int hash = 1;
    for(int i = 0 ; i < vec.size() ; ++i){
        hash = 31 * hash + vec[i];
    }
    hash = 31 * hash + (down ? 1 : 0);
    _hash = hash;
    return hash;
}
struct _GroupKE{
    std::vector<SpKeysEvent> ske;

    bool addTriggerKey(HOS* hos, unsigned long vkCode){
        SpKeysEvent act_ske;
        for(auto se : ske){
            _LOCK(se->_mutex);
            se->_vkCodes.insert(vkCode);
            //
//            auto reqStr = _codes2Str(se->vkCodes);
//            auto actStr = _codes2Str(se->_vkCodes);
//            _LOGE("addTriggerKey >> req codes = %s, but now is %s", reqStr.data(), actStr.data());
            if(se->_vkCodes == se->vkCodes){
                act_ske = se;
                se->_acted = !se->_acted;
                //only one should trigger, so we break here.
                break;
            }
        }
        if(act_ske){
            if(_DEBUG){
                _LOGE("callback >> SpKeysEvent = %p, act = %d", act_ske.get(), act_ske->_acted);
            }
            act_ske->cb(act_ske.get(), act_ske->_acted);
            //one performed ,we should clear all.
            disableKeyCodes();
            return true;
        }else{
            //no one performed. we should send a msg delay to clear keys.
            auto msg = Message::obtain();
            msg->obj.ptr = this;
            msg->what = vkCode;
            hos->getHandler()->sendMessageDelayed(msg, 380);
            return false;
        }
    }
    void disableKeyCodes(){
        for(auto se : ske){
            _LOCK(se->_mutex);
            se->_vkCodes.clear();
            if(_DEBUG){
                _LOGE("disableKeyCodes >> SpKeysEvent = %p", se.get());
            }
        }
    }
};
struct _WinHooker_ctx{
    using u32 = unsigned long;
    using MapHKE = std::unordered_map<int, SpKeysEvent>;
    using MapKES_Down = std::unordered_map<u32, _GroupKE>;
    using MapKES_Up = std::unordered_map<u32, _GroupKE>;

    MapHKE m_map_hash;
    MapKES_Down m_map_down;
    MapKES_Up m_map_up;
    long lastMsgWhat {-1};
    ~_WinHooker_ctx(){}

    void regKeysEvent(SpKeysEvent se){
        if(hashKeysEvent(se)){
            return;
        }
        m_map_hash[se->getCodesHash()] = se;
        if(se->down){
            for(auto ele : se->vkCodes){
                if(_DEBUG){
                    _LOGE("---> m_map_down >> reg ele = %u", ele);
                }
                m_map_down[ele].ske.push_back(se);
            }
        }else{
            for(auto ele : se->vkCodes){
                m_map_up[ele].ske.push_back(se);
            }
        }
    }
    bool hashKeysEvent(SpKeysEvent se){
        return m_map_hash.find(se->getCodesHash()) != m_map_hash.end();
    }
    void onKeyDown(HOS* hos, unsigned long vkCode){
        if(lastMsgWhat >= 0){
            hos->getHandler()->removeMessages(lastMsgWhat);
            lastMsgWhat = -1;
        }
        {
        auto it = m_map_down.find(vkCode);
        if(it != m_map_down.end()){
            if(!it->second.addTriggerKey(hos, vkCode)){
                lastMsgWhat = vkCode;
            }
        }
        }
        //disable up code events?
    }
    void onKeyUp(HOS* hos, unsigned long vkCode){
        if(lastMsgWhat >= 0){
            hos->getHandler()->removeMessages(lastMsgWhat);
            lastMsgWhat = -1;
        }
        {
        auto it = m_map_up.find(vkCode);
        if(it != m_map_up.end()){
            if(!it->second.addTriggerKey(hos, vkCode)){
                lastMsgWhat = vkCode;
            }
        }
        }
    }
};
struct FocusKeys{
    HOS hos;
    WinHooker* wh {nullptr};

    h7::CppLock lock_mouse;
    MousePos lastMousePos;

    FocusKeys(){
        hos.start([this](Message* m){
            //
            _GroupKE* ge = (_GroupKE*)m->obj.ptr;
            //_LOGE("handler >> disableKeyCodes.");
            ge->disableKeyCodes();
            return true;
        });
    }
    ~FocusKeys(){
        hos.quit();
    }
    void onKeyDown(unsigned long vkCode){
        if(_DEBUG){
            std::cout << "FocusKeys::onKeyDown >> "<< vkCode << std::endl;
        }
        if(!wh){
            return;
        }
        wh->m_ptr->onKeyDown(&hos, vkCode);
    }
    void onKeyUp(unsigned long vkCode){
        if(_DEBUG){
            std::cout << "FocusKeys::onKeyUp >> "<< vkCode << std::endl;
        }
        //_LOGE("onKeyUp: %d", vkCode);
        if(!wh){
            return;
        }
        wh->m_ptr->onKeyUp(&hos, vkCode);
    }
    void onMouseEvent(int x, int y, long long time){
        lock_mouse.lock();
        lastMousePos.x = x;
        lastMousePos.y = y;
        lastMousePos.time = time;
        lock_mouse.unlock();
    }
    void getLastMousePos(MousePos& out){
        lock_mouse.lock();
        out = lastMousePos;
        lock_mouse.unlock();
    }
};
}

static FocusKeys _FK;
//-----------------------

WinHooker::WinHooker()
{
    sysInfo.width = GetSystemMetrics(SM_CXSCREEN);
    sysInfo.height = GetSystemMetrics(SM_CYSCREEN);
    sysInfo.nVirtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN) ;
    sysInfo.nVirtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN) ;
    sysInfo.nVirtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN) ;
    sysInfo.nVirtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN) ;
    _FK.wh = this;
    m_ptr = new _WinHooker_ctx();
}
WinHooker::~WinHooker(){
    _FK.wh = nullptr;
    if(m_ptr){
        delete m_ptr;
        m_ptr = nullptr;
    }
}
void WinHooker::getWindowInfoUtf8(CString u8, WindowInfo* out){
    getWindowInfo(Utf8ToGbk(u8.data()), out);
}
void WinHooker::getWindowInfo(CString gbk, WindowInfo* out){
    Holder holder;
    holder.gbk = gbk;
    holder.reverseGbk = _StrReverse(gbk.data());
    holder.wh = this;
    holder.winInfo = out ? out : &winInfo;
    EnumWindows(lpEnumFunc, (LPARAM)&holder);
}

void WinHooker::attachInput(void* handle){
    if(handle == nullptr){
        handle = winInfo.handle;
    }
    //
    HWND hForeWnd = GetForegroundWindow();
    DWORD dwCurID = GetCurrentThreadId();
    DWORD dwForeID = GetWindowThreadProcessId(hForeWnd, NULL);
    AttachThreadInput(dwCurID, dwForeID, TRUE);

    // 将该窗体呼出到顶层
    ShowWindow((HWND)handle, SW_SHOWNORMAL);
    SetWindowPos((HWND)handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SetWindowPos((HWND)handle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SetForegroundWindow((HWND)handle);
    AttachThreadInput(dwCurID, dwForeID, FALSE);
}

bool WinHooker::startHookKeyboard(){
    //SetConsoleOutputCP(65001);// 更改cmd编码为utf8
    // 安装钩子
    m_keyboard = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,	// callbck func
        GetModuleHandleA(NULL),	// Dll handle
        NULL
        );
    if (m_keyboard == nullptr){
        return false;
    }

    //不可漏掉消息处理，不然程序会卡死
    MSG msg;
    while(1)
    {
        // 如果消息队列中有消息
        if (PeekMessageA(
            &msg,		// MSG 接收这个消息
            NULL,		// 检测消息的窗口句柄，NULL：检索当前线程所有窗口消息
            NULL,		// 检查消息范围中第一个消息的值，NULL：检查所有消息（必须和下面的同时为NULL）
            NULL,		// 检查消息范围中最后一个消息的值，NULL：检查所有消息（必须和上面的同时为NULL）
            PM_REMOVE	// 处理消息的方式，PM_REMOVE：处理后将消息从队列中删除
            )){
                // 把按键消息传递给字符消息
                TranslateMessage(&msg);

                // 将消息分派给窗口程序
                DispatchMessageW(&msg);
        }
        else
            Sleep(0);    //避免CPU全负载运行
    }
    UnhookWindowsHookEx((HHOOK)m_keyboard);
    return true;
}
bool WinHooker::startHookMouse(){
    m_mouse = SetWindowsHookEx(
        WH_MOUSE_LL,
        LowLevelMouseProc,
        GetModuleHandleA(NULL),
        NULL
        );
    if (m_mouse == nullptr){
        return false;
    }

    MSG msg;
    while(1)
    {
        if (PeekMessageA(
            &msg,
            NULL,
            NULL,
            NULL,
            PM_REMOVE
            )){
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
        }
        else
            Sleep(0);
    }
    UnhookWindowsHookEx((HHOOK)m_mouse);
    return true;
}

void WinHooker::regKeysEvent(SpKeysEvent se){
    m_ptr->regKeysEvent(se);
}
bool WinHooker::hasKeysEvent(SpKeysEvent se){
    return m_ptr->hashKeysEvent(se);
}
void WinHooker::getMousePos(MousePos& out){
    return _FK.getLastMousePos(out);
}
bool WinHooker::queryMousePosition(CMousePos in, MousePos& out){
    std::vector<MousePos> vec;
    if(queryMousePosition(in, vec) > 0){
        out = vec[0];
        return true;
    }
    return false;
}
int WinHooker::queryMousePosition(CMousePos in, std::vector<MousePos>& out){
    if(out.size() == 0){
        out.resize(1);
    }
    int cpt = 0 ;
    int mode = GMMP_USE_DISPLAY_POINTS ;

    MOUSEMOVEPOINT mp_in ;
    MOUSEMOVEPOINT mp_out[64] ;

    ZeroMemory(&mp_in, sizeof(mp_in)) ;
    mp_in.x = in.x & 0x0000FFFF ;//Ensure that this number will pass through.
    mp_in.y = in.y & 0x0000FFFF ;
    cpt = GetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT),&mp_in, (MOUSEMOVEPOINT*)&mp_out, out.size(), mode) ;

    for (int i = 0; i < cpt; i++)
    {
       switch(mode)
       {
       case GMMP_USE_DISPLAY_POINTS:
          if (mp_out[i].x > 32767){
             out[i].x = mp_out[i].x - 65536 ;
          }
          if (mp_out[i].y > 32767){
              out[i].y = mp_out[i].y - 65536 ;
          }
          break ;
       case GMMP_USE_HIGH_RESOLUTION_POINTS:
          out[i].x = ((mp_out[i].x * (sysInfo.nVirtualWidth - 1)) - (sysInfo.nVirtualLeft * 65536)) / sysInfo.nVirtualWidth ;
          out[i].y = ((mp_out[i].y * (sysInfo.nVirtualHeight - 1)) - (sysInfo.nVirtualTop * 65536)) / sysInfo.nVirtualHeight ;
          break ;
       }
    }
    return cpt;
}
//------------------------------
LRESULT CALLBACK LowLevelKeyboardProc(
    _In_ int nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    ){
    //system msg. dispatch to next.
    if(nCode < 0){
         return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    KBDLLHOOKSTRUCT *ks = (KBDLLHOOKSTRUCT*)lParam;
    using namespace std;

    bool down;
    switch (wParam)
    {
    case WM_KEYDOWN:
        down = true;
        break;
    case WM_KEYUP:
        down = false;
        break;

    case WM_SYSKEYDOWN:
        down = true;
        break;
    case WM_SYSKEYUP:
        down = false;
        break;

    default:
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    //cout << "------- FK --------" << endl;
    if(down){
        _FK.onKeyDown(ks->vkCode);
    }else{
        _FK.onKeyUp(ks->vkCode);
    }
    // call to next
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(_In_ int nCode,
                                   _In_ WPARAM wParam,_In_ LPARAM lParam){
    if(nCode < 0){
         return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    if(nCode == HC_ACTION){
        //wParam: 512 = WM_MOUSEMOVE
        MOUSEHOOKSTRUCT* mh = (MOUSEHOOKSTRUCT*)lParam;
        //_LOGE("lParam.xy = %d, %d", mh->pt.x, mh->pt.y);
        //_FK.setMousePos(mh->pt.x, mh->pt.y, h7_handler_os::getCurTime());
        _FK.onMouseEvent(mh->pt.x, mh->pt.y, 0);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL CALLBACK lpEnumFunc(HWND hwnd, LPARAM lParam)
{
    Holder* h = (Holder*)lParam;
    //
    char WindowName[MAXBYTE] = { 0 };
    GetWindowText(hwnd, WindowName, sizeof(WindowName));
    if(strlen(WindowName) > 0){
        _LOGE("WindowName = '%s'", WindowName);
    }
    //if (strncmp(WindowName, h->gbk.data(), h->gbk.length()) == 0) {
    auto wname = _StrReverse(WindowName);
    if (strncmp(wname.data(), h->reverseGbk.data(), h->reverseGbk.length()) == 0) {
        auto& wInfo = *h->winInfo;
        wInfo.threadId = GetWindowThreadProcessId(hwnd, &wInfo.processId);
        wInfo.handle = hwnd;
        wInfo.name = h->gbk;
        bool ret = GetWindowRect(hwnd, (RECT*)&wInfo.winRect);
        if(!ret){
            _LOGE("GetWindowRect failed.");
        }
        return false;
    }
    return TRUE;
}
String Utf8ToGbk(const char *src_str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
    wchar_t* wszGBK = new wchar_t[len + 1];
    memset(wszGBK, 0, len * 2 + 2);
    MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
    len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
    char* szGBK = new char[len + 1];
    memset(szGBK, 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
    String strTemp(szGBK);
    if (wszGBK) delete[] wszGBK;
    if (szGBK) delete[] szGBK;
    return strTemp;
}
String _StrReverse(const char* _str)
{
    String ret(_str);
    auto str = (char*)ret.data();
    int n = ret.length();
    int i;
    char temp;
    for (i = 0; i < (n / 2); i++)
    {
    temp = str[i];
    str[i] = str[n - i - 1];
    str[n - i - 1] = temp;
    }
    return ret;
}
