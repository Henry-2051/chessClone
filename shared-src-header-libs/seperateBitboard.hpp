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
    FastStack<uint64_t, N> resultSeperatedBitboard{};
    auto num_zeros{ std::countr_zero(pieces)};
    while (pieces != 0) {
        resultSeperatedBitboard.push(1ULL << num_zeros);
        pieces &= (pieces - 1);
        num_zeros = std::countr_zero(pieces);
    }
    return resultSeperatedBitboard;
}

template<size_t N>
void addAttacksToStack218(uint64_t piece, uint64_t attacked_squares, PieceType typeofPiece, stackStack218& moveStack, bool isWhiteTurn, const chessBoard& board) {
    assert(std::popcount(attacked_squares) <= static_cast<int>(N));

    // if (std::popcount(attacked_squares) > static_cast<int>(N)) {
    //     std::println("popcount : {}", std::popcount(attacked_squares));
    //     std::println("type of piece: {}", getPieceTypeString(typeofPiece));
    //     helpers::printBitboard(attacked_squares);
    //     throw std::logic_error("trying to seperate a bitboard with more items than the array size");
    // }
    uint64_t blackLeftCorner = 0x1;
    uint64_t blackRightCorner= 0x1 << 7;
    uint64_t whiteLeftCorner = blackLeftCorner << 56;
    uint64_t whiteRightCorner = blackRightCorner << 56;

    FastStack<uint64_t, N> seperatedmoves = seperateBitboardFastStackReturn<N>(attacked_squares);
    for (uint64_t mv : seperatedmoves) {

        // if (std::popcount(mv) != 1) {
        //     std::println("seperation failed : ");
        //     helpers::printBitboard(mv);
        //     throw std::runtime_error("seperation failed");
        // }

        PieceType enemyPieceType = board.figureOutTypeOfPieceOnSquare(mv, !isWhiteTurn);
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

            // std::println("hereeeeeeeeeeeeeeeeeeeeeeeeeeee");
            // std::println("{}", std::countr_zero(piece));
            // helpers::printBitboard(((isWhiteTurn ? piece >> 16 : piece << 16) & mv));
            if ((isWhiteTurn ? std::countr_zero(piece) >= 16 : std::countr_zero(piece) <= 47) && ((isWhiteTurn ? piece >> 16 : piece << 16) & mv)) {
                int pawn_file = std::countr_zero(piece) % 8;

                bool can_have_right_bitshift_by_1 = pawn_file != 0;
                bool can_have_left_bitshift_by_1 = pawn_file != 7;

                uint64_t passingSquares = (can_have_right_bitshift_by_1 ? mv >> 1 : 0) | (can_have_left_bitshift_by_1 ? mv << 1 : 0);

                if(board.getPiecesByColorConst(!isWhiteTurn)[PieceType::Pawn] & passingSquares) {
                    // std::println("en passant move!!");
                    // helpers::printBitboard(mv | piece);
                    // helpers::printBitboard(squaresToCheck);
                    // std::println("pawnFile : {}", pawn_file);
                    enPassantState = std::countr_zero(isWhiteTurn ? mv << 8 : mv >> 8);
                }
            }
        }

        pieceMovementBoardState ^= board.m_board_state & board_state::allCastlingFields_const & pieceMovementBoardState;
        auto moveToPush = isWhiteTurn ? pieceMovement{mv | piece, destinationOccupiedByEnemy ? mv : 0, typeofPiece, PieceType::NotAPiece, PieceType::NotAPiece, enemyPieceType, enPassantState, pieceMovementBoardState} :
                                        pieceMovement{mv | piece, destinationOccupiedByEnemy ? mv : 0, PieceType::NotAPiece, typeofPiece, enemyPieceType, PieceType::NotAPiece, enPassantState, pieceMovementBoardState};

        // if (typeofPiece == PieceType::Pawn) {
        //     moveToPush.printThis();
        // }
        moveStack.push(moveToPush);
    }
}


#endif
