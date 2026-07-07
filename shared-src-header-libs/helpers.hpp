#include <cstdint>
#include <iostream>
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

inline void printBitboard(uint64_t bitboard) {
    std::cout << "0-=-=-=-=-=-=-7\n";
    for (int rank = 0; rank <= 7; ++rank) {     // ranks from 0 (black) to 7 (white)
        for (int file = 0; file <= 7; ++file) { // files from 0 (left) to 7 (right)
            int squareIndex = rank * 8 + file; // bit index from 0 (LSB) to 63 (MSB)
            // Use mask to check bit; bit 0 at LSB
            uint64_t mask = 1ULL << squareIndex;
            std::cout << ((bitboard & mask) ? '#' : '.') << ' ';
        }
        std::cout << "\n";
    }
    std::cout << "56-=-=-=-=-=-63\n";
}
}
