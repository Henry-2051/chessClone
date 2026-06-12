
#include <cstddef>
#include <cstdint>
#include <utility>

#ifndef SEPERATE_BITBOARD
#define SEPERATE_BITBOARD

template <size_t N>
std::pair<std::array<uint64_t, N>, std::size_t> 
seperateBitboard(uint64_t pieces) {
    std::array<uint64_t, N> resultSeperatedBitboard{};
    int ctz_result{ __builtin_ctzll(pieces)};
    size_t count {0};
    while (pieces != 0) {
        resultSeperatedBitboard[count] = 1ULL << ctz_result;
        pieces &= (pieces - 1);
        ctz_result = __builtin_ctzll(pieces);
        ++ count;
    }
    return {resultSeperatedBitboard, count};
}

#endif
