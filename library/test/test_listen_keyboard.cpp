#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include <unordered_set>

#include "WinHooker.h"
#include "WorkManager.h"

using namespace std;

#define _LOGE(fmt, ...) \
    do{char buf[1024];\
    snprintf(buf, 1024, fmt, ##__VA_ARGS__);\
    std::cerr << buf << std::endl;\
    }while(0);

HHOOK keyboardHook = 0;		// 钩子句柄、

static LRESULT CALLBACK LowLevelKeyboardProc(
    _In_ int nCode,		// 规定钩子如何处理消息，小于 0 则直接 CallNextHookEx
    _In_ WPARAM wParam,	// 如果是鼠标事件，则表示鼠标事件类型, https://www.cnblogs.com/chorm590/p/14199978.html
    _In_ LPARAM lParam	// 指向某个结构体的指针，这里是 KBDLLHOOKSTRUCT（低级键盘输入事件）
    ){
    KBDLLHOOKSTRUCT *ks = (KBDLLHOOKSTRUCT*)lParam;		// 包含低级键盘输入事件信息
    /*
    typedef struct tagKBDLLHOOKSTRUCT {
        DWORD     vkCode;		// 按键代号
        DWORD     scanCode;		// 硬件扫描代号，同 vkCode 也可以作为按键i的代号。
        DWORD     flags;		// 事件类型，一般按键按下为 0 抬起为 128。
        DWORD     time;			// 消息时间戳
        ULONG_PTR dwExtraInfo;	// 消息附加信息，一般为 0。
    }KBDLLHOOKSTRUCT,*LPKBDLLHOOKSTRUCT,*PKBDLLHOOKSTRUCT;
    */
    cout << "flags = " << ks->flags << ", nCode = " << nCode << endl;
        //WIN/ALT no down
   switch (wParam)
   {
   case WM_KEYDOWN:
       cout << "f0 >> key-down" << endl;
       break;
   case WM_KEYUP:
       cout << "f0 >>key-up" << endl;
       break;

   case WM_SYSKEYDOWN:
       cout << "f0 >> sys-key-down" << endl;
       break;
   case WM_SYSKEYUP:
       cout << "f0 >> sys-key-up" << endl;
       break;
   }
    if(ks->flags == 128 || ks->flags == 129)
    {
       switch (wParam)
       {
       case WM_KEYDOWN:
           cout << "key-down" << endl;
           break;
       case WM_KEYUP:
           cout << "key-up" << endl;
           break;

       case WM_SYSKEYDOWN:
           cout << "sys-key-down" << endl;
           break;
       case WM_SYSKEYUP:
           cout << "sys-key-up" << endl;
           break;
       }
        // 监控键盘
        switch(ks->vkCode){
        case 0x30: case 0x60:
            cout << "listn key(0): " << "0" << endl;
            break;
        case 0x31: case 0x61:
            cout << "listn key(1): " << "1" << endl;
            break;
        }

        //return 1;		// 使按键失效
    }

    // 将消息传递给钩子链中的下一个钩子
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static void test_WinHooker();
static void test_WorkManager();

int main_test3()
{
    {
       // unordered_set<int> s1 = {1,2,3};
       // unordered_set<int> s2 = {3,2,1};
       // cout << "unordered_set >>  " << (s1 == s2) << endl;
        test_WinHooker();
        // test_WorkManager();
        return 0;
    }
    SetConsoleOutputCP(65001);// 更改cmd编码为utf8
    // 安装钩子
    keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,			// 钩子类型，WH_KEYBOARD_LL 为键盘钩子
        LowLevelKeyboardProc,	// 指向钩子函数的指针
        GetModuleHandleA(NULL),	// Dll 句柄
        NULL
        );
    if (keyboardHook == 0){cout << "挂钩键盘失败" << endl; return -1;}

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
    // 删除钩子
    UnhookWindowsHookEx(keyboardHook);

    return 0;
}

struct Collector0 : public hw32::EventCollector{
    void onKeyEvent(int itemId, unsigned long vkCode, bool down) override{
        _LOGE("onKeyEvent: itemId = %d, vkCode = %u, down = %d", itemId, vkCode, down);
    }
    void onMouseEvent(int itemId, int dx, int dy, bool absolute) override{
        _LOGE("onMouseEvent: itemId = %d, dx = %d, dy = %d, absolute = %d",
              itemId, dx, dy, absolute);
    }
};

void test_WorkManager(){
    using namespace hw32;
    WorkItem item;
    item.id = 1;
    item.vkCodes = {VK_LCONTROL, VK_LMENU};

    Events& es = item.entry;
//    {
//        Event e;
//        e.keyboard = true;
//        e.vkCode = 0x46; //F
//        e.down = true;
//        es.push_back(e);
//    }
//    {
//        Event e;
//        e.keyboard = true;
//        e.vkCode = 0x46; //F
//        e.down = false;
//        es.push_back(e);
//    }
    {
        Event e;
        e.keyboard = false;
        e.dx = 300;
        e.dy = 300;
        e.absolute = false;
        es.push_back(e);
    }
    {
        Event e;
        e.keyboard = false;
        e.dx = 300;
        e.dy = 300;
        e.absolute = false;
        e.mouseClickType = kMouse_LEFT;
        e.mouseUpDelayMs = 10;
        es.push_back(e);
    }
    WorkManager wm(item);
   // wm.setEventCollector<Collector0>();
    wm.start("Google Chrome");
}

void test_WinHooker(){
    using namespace hw32;
    WinHooker wh;
    {
    auto ptr = std::make_shared<KeysEvent>();
    ptr->vkCodes = {VK_LCONTROL, VK_LMENU}; //left: ctrl + alt
    ptr->cb = [](KeysEvent* ke, bool act){
         cout << "DOWN >> left: ctrl + alt occurs." << endl;
    };
    wh.regKeysEvent(ptr);
    }
    {
    auto ptr = std::make_shared<KeysEvent>();
    ptr->vkCodes = {VK_TAB, VK_LMENU};
    ptr->cb = [](KeysEvent* ke, bool act){
         cout << "DOWN >> tab + alt occurs." << endl;
    };
    ptr->down = true;
    wh.regKeysEvent(ptr);
    }
   // wh.startHookKeyboard();
    wh.startHookMouse();
}
