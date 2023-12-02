#pragma once

#include <string>
#include <vector>

namespace hw32 {

using u32 = unsigned long;
using String = std::string;
using CString = const std::string&;

enum{
    kMouse_NONE,
    kMouse_LEFT,
    kMouse_RIGHT,
    kMouse_MIDDLE,
};

struct MousePos{
    int x {0};
    int y {0};
    long long time;
};

struct Event{
    bool keyboard;
    u32 vkCode; bool down;
    //mouseType: kMouse_LEFT or etc. used for send mouse down/up
    int dx; int dy; bool absolute;
    int mouseClickType {kMouse_NONE};
    int mouseUpDelayMs {0}; //mouseUpDelayMs only used for mouse down event
};

using Events = std::vector<Event>;

struct WorkItem{
    int id; //0 - 11 -> F1-F12
    std::vector<u32> vkCodes;
    Events entry;
};
using WorkItems = std::vector<WorkItem>;
using CWorkItems = const WorkItems&;

}
