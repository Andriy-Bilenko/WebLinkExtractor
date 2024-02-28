#include "../inc/dataStructs.hpp"

std::ostream& operator<<(std::ostream& out, const LinkData& linkdata) {
    out << "linkdata{link:" << linkdata.link << "}\n";
    return out;
}
