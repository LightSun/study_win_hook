#include <windows.h>
#include <iostream>

using namespace std;

#define Press 1
#define Realse 0
/*成员dx,dy为移动为xy距离,absolute为移动是否相对值，成功返回1*/
/*Virtually为虚拟键码,Status为状态按下Press或者Realse*/
BOOL MouseEvent(int dx, int dy, bool absolute);
BOOL KeyBoardEvent(unsigned int VirtualKey, bool Status);

BOOL MouseEvent(int dx, int dy, bool absolute)
{
    INPUT inputs[1] = {};
    ZeroMemory(inputs, sizeof(inputs));
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dx = dx;
    inputs[0].mi.dy = dy;
    inputs[0].mi.mouseData = 0;
    if (absolute == 1)
    {
        inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    }
    else if (absolute == 0)
    {
        inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE;
    }
    UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    if (uSent != ARRAYSIZE(inputs))
    {
        cout << "SendInput failed: 0x%x\n" << HRESULT_FROM_WIN32(GetLastError());
        return 0;
    }
    else
    {
        cout << "Success" << endl;
        return 1;
    }

}



BOOL KeyBoardEvent(unsigned int VirtualKey, bool Status)
{
    INPUT inputs[1] = {};
    ZeroMemory(inputs, sizeof(inputs));
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VirtualKey;

    if (Status == Press)
    {

    }
    else if (Status == Realse)
    {
        inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
    }

    UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    if (uSent != ARRAYSIZE(inputs))
    {
        cout << "SendInput failed: 0x%x\n" << HRESULT_FROM_WIN32(GetLastError());
        return 0;
    }
    else
    {
        cout << "Success" << endl;
        return 1;
    }
}
