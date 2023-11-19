#include <iostream>
#include <windows.h>

using namespace std;

static HWND global_hwnd = 0;
static string _dnf_name;

// 将字符串逆序
char * Reverse(char str[])
{
  int n = strlen(str);
  int i;
  char temp;
  for (i = 0; i < (n / 2); i++)
  {
    temp = str[i];
    str[i] = str[n - i - 1];
    str[n - i - 1] = temp;
  }
  return str;
}
string GbkToUtf8(const char *src_str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len + 1];
    memset(wstr, 0, len + 1);
    MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[len + 1];
    memset(str, 0, len + 1);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    string strTemp = str;
    if (wstr) delete[] wstr;
    if (str) delete[] str;
    return strTemp;
}
string Utf8ToGbk(const char *src_str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
    wchar_t* wszGBK = new wchar_t[len + 1];
    memset(wszGBK, 0, len * 2 + 2);
    MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
    len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
    char* szGBK = new char[len + 1];
    memset(szGBK, 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
    string strTemp(szGBK);
    if (wszGBK) delete[] wszGBK;
    if (szGBK) delete[] szGBK;
    return strTemp;
}

// 窗体枚举回调函数
BOOL CALLBACK lpEnumFunc(HWND hwnd, LPARAM lParam)
{
  char WindowName[MAXBYTE] = { 0 };
  DWORD ThreadId, ProcessId = 0;

  GetWindowText(hwnd, WindowName, sizeof(WindowName));
  ThreadId = GetWindowThreadProcessId(hwnd, &ProcessId);
  printf("hw_handle: %-9d --> thread ID: %-6d --> process ID: %-6d --> win_name: %s \n", hwnd, ThreadId, ProcessId, WindowName);

  // 首先逆序输出字符串,然后比较前13个字符
  //if (strncmp(Reverse(WindowName), "emorhC elgooG", 13) == 0)
  if (strcmp(WindowName, _dnf_name.data()) == 0)
  {
    global_hwnd = hwnd;
    RECT rect;
    bool ret = GetWindowRect(hwnd, &rect);
    printf("GetWindowRect: %d, left,top,right, bottom = %d, %d, %d, %d\n", ret,
           rect.left, rect.top, rect.right, rect.bottom);
    return false;
  }
  return TRUE;
}

static void handle(){

    if ((GetKeyState('C') & (1 << 15)) != 0) // C键按下
    {
        INPUT input;

        ZeroMemory(&input, sizeof(input));

        input.type = INPUT_KEYBOARD;

        input.ki.wVk = 'Z';

        input.ki.wScan = MapVirtualKey(input.ki.wVk, MAPVK_VK_TO_VSC);

        SendInput(1, &input, sizeof(INPUT)); // 按下Z键

        Sleep(100); // 可能东方是在处理逻辑时检测一下Z键是否按下才发弹幕，如果这时Z键刚好弹起就没有反应，所以要延迟一下

        input.ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(1, &input, sizeof(INPUT)); // 弹起Z键
    }
}

//地下城与勇士
int main_tets2()
{
    _dnf_name = Utf8ToGbk("地下城与勇士");
  // 枚举Google浏览器句柄
  EnumWindows(lpEnumFunc, 0);
  std::cout << "chrome handle: " << global_hwnd << std::endl;

  if (global_hwnd != 0)
  {
    // 获取系统屏幕宽度与高度 (像素)
    int cx = GetSystemMetrics(SM_CXSCREEN);
    int cy = GetSystemMetrics(SM_CYSCREEN);
    std::cout << "screen_X: " << cx << " screen_Y: " << cy << std::endl;

    // 传入指定的HWND句柄
    HWND hForeWnd = GetForegroundWindow();
    DWORD dwCurID = GetCurrentThreadId();
    DWORD dwForeID = GetWindowThreadProcessId(hForeWnd, NULL);
    AttachThreadInput(dwCurID, dwForeID, TRUE);

    // 将该窗体呼出到顶层
    ShowWindow(global_hwnd, SW_SHOWNORMAL);
    SetWindowPos(global_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SetWindowPos(global_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SetForegroundWindow(global_hwnd);
    AttachThreadInput(dwCurID, dwForeID, FALSE);

    // 发送最大化消息
   // SendMessage(global_hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);    // 最大化
    // SendMessage(global_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0); // 最小化
    // SendMessage(global_hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);    // 关闭
  }

  //system("pause");
  return 0;
}
