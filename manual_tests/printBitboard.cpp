#include <cstdint>
#include <iostream>

void printBitboard(uint64_t bitboard) {
    for (int rank = 7; rank >= 0; --rank) {     // ranks from 7 (top) down to 0 (bottom)
        for (int file = 0; file <= 7; ++file) { // files from 0 (left) to 7 (right)
            int squareIndex = rank * 8 + file; // bit index from 0 (LSB) to 63 (MSB)
            // Use mask to check bit; bit 0 at LSB
            uint64_t mask = 1ULL << squareIndex;
            std::cout << ((bitboard & mask) ? '#' : '.') << ' ';
        }
        std::cout << "\n";
    }
}

