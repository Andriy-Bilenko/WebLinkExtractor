#include "../inc/dataStructs.hpp"

#include "../inc/defines.hpp"

std::ostream& operator<<(std::ostream& out, const LinkData& linkdata) {
    if (linkdata.alive) {
        out << ANSI_GREEN << "{depth:" << linkdata.depth_level << " link:" << linkdata.link << "}\n\r" << ANSI_RESET;
    } else {
        out << ANSI_RED << "{depth:" << linkdata.depth_level << " link:" << linkdata.link << "}\n\r" << ANSI_RESET;
    }
    return out;
}
