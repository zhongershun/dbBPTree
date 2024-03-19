#ifndef COMP_H
#define COMP_H

#include <cstdint>

template<typename T>
struct DefaultCompare {
    uint64_t operator()(const T &x, const T &y) const {
        if (x > y) {
            return 1;
        } else if (x == y) {
            return 0;
        } else {
            return -1;
        }
    }
};

#endif