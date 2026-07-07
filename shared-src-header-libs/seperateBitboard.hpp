#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include "stackStack.hpp"

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

template<size_t N>
void addAttacksToStack218(uint64_t piece, uint64_t attacked_squares, PieceType typeofPiece, stackStack218& moveStack) {
    if (std::popcount(attacked_squares) > static_cast<int>(N)) {
        std::println("popcount : {}", std::popcount(attacked_squares));
        std::println("type of piece: {}", getPieceTypeString(typeofPiece));
        helpers::printBitboard(attacked_squares);
        throw std::logic_error("trying to seperate a bitboard with more items than the array size");
    }
    auto [moves, num_moves] = seperateBitboard<N>(attacked_squares);
    for (size_t i = 0; i < num_moves; ++i) {
        moveStack.push({moves[i] | piece, 0, typeofPiece, PieceType::King, false});
    }
}

template <size_t N>
FastStack<uint64_t, N>
seperateBitboardFastStackReturn(uint64_t pieces) {
    FastStack<uint64_t, N> resultSeperatedBitboard{};
    auto num_zeros{ std::countr_zero(pieces)};
    while (pieces != 0) {
        resultSeperatedBitboard.push(1ULL << num_zeros);
        pieces &= (pieces - 1);
        num_zeros = std::countr_zero(pieces);
    }
    return resultSeperatedBitboard;
}

#endif
