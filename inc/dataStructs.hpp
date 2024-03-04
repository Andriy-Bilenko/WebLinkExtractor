#pragma once

#include <iostream>
#include <string>
#include <thread>

struct ThreadData {
    std::thread::id id;
    bool notify_condition = false;
};

struct LinkData {
    int depth_level;
    std::string link;
    std::string full_route;
    int alive{-1};  // 0 - false, 1 - true, -1 - have no idea just yet
    friend std::ostream& operator<<(std::ostream& out, const LinkData& linkdata);
};

std::ostream& operator<<(std::ostream& out, const LinkData& linkdata);
