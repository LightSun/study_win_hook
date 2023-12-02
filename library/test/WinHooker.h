#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_set>
#include "work_config.h"

namespace hw32 {
using u32 = unsigned long;
using String = std::string;
using CString = const std::string&;

struct KeysEvent;
using SpKeysEvent = std::shared_ptr<KeysEvent>;

struct Rect {
  int left;
  int top;
  int right;
  int bottom;
};

struct WindowInfo{
    String name; //gbk
    void* handle {nullptr};
    Rect winRect;
    unsigned long processId;
    unsigned long threadId;
};

struct SysInfo{
    int width;
    int height;
    int nVirtualWidth;
    int nVirtualHeight;
    int nVirtualLeft;
    int nVirtualTop;
};

//act: true if should act. false other wise.
typedef void (*Func_KeysEvent)(KeysEvent* ke, bool act);
struct KeysEvent{
    std::unordered_set<unsigned long> vkCodes;
    void* context {nullptr};
    Func_KeysEvent cb {nullptr};
    bool down {true};
    int what;  //extern data for
    //internal
    int _hash {0};
    bool _acted {false};
    std::unordered_set<unsigned long> _vkCodes;
    std::mutex _mutex;

    int getCodesHash();
};
typedef struct _WinHooker_ctx  _WinHooker_ctx;

class WinHooker
{
public:
    using LL = long long;
    using CMousePos = const MousePos&;
    WinHooker();
    ~WinHooker();

    void getWindowInfoUtf8(CString u8, WindowInfo* out = nullptr);
    void getWindowInfo(CString gbk, WindowInfo* out = nullptr);
    void attachInput(void* handle = nullptr);
    bool startHookKeyboard();
    bool startHookMouse();

    bool hasKeysEvent(SpKeysEvent se);
    void regKeysEvent(SpKeysEvent se);
    //get mouse pos from 'startHookMouse'
    void getLastMousePos(MousePos& out);
    bool queryMousePosition(CMousePos in,MousePos& out);
    int queryMousePosition(CMousePos in, std::vector<MousePos>& out);

public:
    WindowInfo winInfo;
    SysInfo sysInfo;
private:
    friend struct FocusKeys;
    void* m_keyboard {nullptr};
    void* m_mouse {nullptr};
    _WinHooker_ctx* m_ptr;
};

}
