#include <cstdint>
#include <vector>

#pragma once
namespace helpers {
inline std::vector<int> getOnes(uint64_t b) {
    std::vector<int> ones = {};
    int count = 0;
    while (b > 0) {
        if (b % 2 == 1) {
            ones.push_back(count);
        } 
        b /=2;
        count ++;
    }
    return ones;
}

inline std::vector<std::pair<uint32_t, uint32_t>> getChessCoordinates(std::vector<int> ones) {
    std::vector<std::pair<uint32_t, uint32_t>>   result = {};
    for (auto p : ones) {
        int row = p / 8;
        int col = p % 8;
        result.push_back({col, row});
    }
    return result;
}
}
