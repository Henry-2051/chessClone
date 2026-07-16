#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <print>
#include <utility>
#include "chessBoard.h"
#include "helpers.hpp"
#include "stackStack.hpp"
#include "timer.hpp"

// #include <x86intrin.h>

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

template <size_t N>
FastStack<uint64_t, N>
seperateBitboardFastStackReturn(uint64_t pieces) {
    // Timer<Timers::SeperateBitboardFastStack> t{};
    FastStack<uint64_t, N> resultSeperatedBitboard{};
    auto num_zeros{ std::countr_zero(pieces)};
    while (pieces != 0) {
        resultSeperatedBitboard.push(1ULL << num_zeros);
        pieces &= (pieces - 1);
        num_zeros = std::countr_zero(pieces);
    }
    return resultSeperatedBitboard;
}

inline void addAttacksToStack218(uint64_t piece, uint64_t attacked_squares, PieceType typeofPiece, stackStack218& moveStack, bool isWhiteTurn, const chessBoard& board, uint64_t enemies) {
    // Timer<Timers::AddToStack218> t{};
    assert(std::popcount(attacked_squares) <= static_cast<int>(N));

    uint64_t blackLeftCorner = 0x1;
    uint64_t blackRightCorner= 0x1 << 7;
    uint64_t whiteLeftCorner = blackLeftCorner << 56;
    uint64_t whiteRightCorner = blackRightCorner << 56;


    while (attacked_squares != 0) {
        uint64_t mv = attacked_squares & -attacked_squares;
        attacked_squares &= attacked_squares -1;

        PieceType enemyPieceType = PieceType::NotAPiece;
        if (mv & enemies) 
            enemyPieceType = board.figureOutTypeOfPieceOnSquare(mv, !isWhiteTurn);

        bool destinationOccupiedByEnemy = enemyPieceType != PieceType::NotAPiece ? mv : 0;
        uint8_t pieceMovementBoardState = board_state::WhiteTurn;

        if (typeofPiece == PieceType::King) {
            auto castlingRightsLost = isWhiteTurn ? 
                board_state::WhiteLostCastlingRightsLeft | board_state::WhiteLostCastlingRightsRight : 
                board_state::BlackLostCastlingRightsLeft | board_state::BlacklostCastlingRightsRight;
            pieceMovementBoardState |= castlingRightsLost;
        } else if (typeofPiece == PieceType::Rook) {
            if (mv & (isWhiteTurn ? whiteLeftCorner : blackLeftCorner)) {
                pieceMovementBoardState |= (isWhiteTurn ? board_state::WhiteLostCastlingRightsLeft : board_state::BlackLostCastlingRightsLeft);
            } else if (mv & (isWhiteTurn ? whiteRightCorner : blackRightCorner)) {
                pieceMovementBoardState |= (isWhiteTurn ? board_state::WhiteLostCastlingRightsRight : board_state::BlacklostCastlingRightsRight);
            }
        }

        int8_t enPassantState {board.enPassantState};

        if(typeofPiece == PieceType::Pawn) {

            if ((isWhiteTurn ? std::countr_zero(piece) >= 16 : std::countr_zero(piece) <= 47) && ((isWhiteTurn ? piece >> 16 : piece << 16) & mv)) {
                int pawn_file = std::countr_zero(piece) % 8;

                bool can_have_right_bitshift_by_1 = pawn_file != 0;
                bool can_have_left_bitshift_by_1 = pawn_file != 7;

                uint64_t passingSquares = (can_have_right_bitshift_by_1 ? mv >> 1 : 0) | (can_have_left_bitshift_by_1 ? mv << 1 : 0);

                if(board.getPiecesByColorConst(!isWhiteTurn)[PieceType::Pawn] & passingSquares) {
                    enPassantState ^= std::countr_zero(isWhiteTurn ? mv << 8 : mv >> 8);
                }
            }
        }

        pieceMovementBoardState ^= board.m_board_state & board_state::allCastlingFields_const & pieceMovementBoardState;
        auto moveToPush = isWhiteTurn ? pieceMovement{mv | piece, destinationOccupiedByEnemy ? mv : 0, typeofPiece, PieceType::NotAPiece, PieceType::NotAPiece, enemyPieceType, enPassantState, pieceMovementBoardState} :
                                        pieceMovement{mv | piece, destinationOccupiedByEnemy ? mv : 0, PieceType::NotAPiece, typeofPiece, enemyPieceType, PieceType::NotAPiece, enPassantState, pieceMovementBoardState};

        moveStack.push(moveToPush);
    }
}


#endif
