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

template<PieceType typeofPiece>
inline void addAttacksToStack218(uint64_t piece, uint64_t attacked_squares, stackStack218& moveStack, bool isWhiteTurn, const chessBoard& board, uint64_t enemies) {
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

        int8_t enPassantState {board.enPassantState};

        // this shortcircuit is an optimisation
        // basically the cpu should branch here 
        if (typeofPiece == PieceType::Knight || typeofPiece == PieceType::Bishop || typeofPiece == PieceType::Queen) {
            auto moveToPush = isWhiteTurn 
            ? pieceMovement{mv | piece, destinationOccupiedByEnemy ? mv : 0, typeofPiece, PieceType::NotAPiece, PieceType::NotAPiece, enemyPieceType, 
                static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)), enPassantState, pieceMovementBoardState} 
            : pieceMovement{mv | piece, destinationOccupiedByEnemy ? mv : 0, PieceType::NotAPiece, typeofPiece, enemyPieceType, PieceType::NotAPiece, 
                static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)), enPassantState, pieceMovementBoardState};

            moveStack.push(std::move(moveToPush));
            continue;
        }

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


        if(typeofPiece == PieceType::Pawn) {
            uint64_t secondRank {0xff000000000000};
            uint64_t fourthRank {0xff00000000};
            uint64_t seventhRank {0xff00};
            uint64_t fifthRank {0xff000000};

            if ((isWhiteTurn ? (piece & secondRank) : (piece & seventhRank)) && 
                    ((isWhiteTurn ? (mv & fourthRank) : (mv & fifthRank)))) {
                // Right bit shift is legal, ie wont teleport the pawn
                uint64_t haveRbs {0x1010101010100};
                uint64_t haveLbs {0x80808080808000};
                haveRbs = ~haveRbs;
                haveLbs = ~haveLbs;

                uint64_t passingSquares = ((mv & haveRbs) >> 1 ) | ((mv & haveLbs) << 1 );

                if(board.getPiecesByColorConst(!isWhiteTurn)[PieceType::Pawn] & passingSquares) {
                    enPassantState ^= std::countr_zero(isWhiteTurn ? mv << 8 : mv >> 8);
                }
            }
        }

        pieceMovementBoardState ^= board.m_board_state & board_state::allCastlingFields_const & pieceMovementBoardState;
        auto moveToPush = isWhiteTurn 
        ? pieceMovement{mv | piece, destinationOccupiedByEnemy ? mv : 0, typeofPiece, PieceType::NotAPiece, PieceType::NotAPiece, enemyPieceType,
            static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)), enPassantState, pieceMovementBoardState} 
        : pieceMovement{mv | piece, destinationOccupiedByEnemy ? mv : 0, PieceType::NotAPiece, typeofPiece, enemyPieceType, PieceType::NotAPiece,
            static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)), enPassantState, pieceMovementBoardState};

        moveStack.push(std::move(moveToPush));
    }
}

#endif
