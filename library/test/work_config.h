#pragma once

#include <string>
#include <vector>

namespace hw32 {

using u32 = unsigned long;
using String = std::string;
using CString = const std::string&;

struct Event{
    bool keyboard;
    u32 vkCode; bool down;
    int dx; int dy; bool absolute;
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
