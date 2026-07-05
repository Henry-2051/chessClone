
#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>

#ifndef SEPERATE_BITBOARD
#define SEPERATE_BITBOARD

// undefined behavior will result if the popcount of pieces is greater than N, either make sure this cant logically happen or add a runtime check
template <size_t N>
std::pair<std::array<uint64_t, N>, std::size_t> 
seperateBitboard(uint64_t pieces) {
    std::array<uint64_t, N> resultSeperatedBitboard{};
    auto num_zeros{ std::countr_zero(pieces)};
    size_t count {0};
    while (pieces != 0) {
        resultSeperatedBitboard[count] = 1ULL << num_zeros;
        pieces &= (pieces - 1);
        num_zeros = std::countr_zero(pieces);
        ++ count;
    }
    return {resultSeperatedBitboard, count};
}

#endif
