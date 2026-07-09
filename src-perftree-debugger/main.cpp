#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <format>
#include <map>
#include <print>
#include <string>
#include "chessBoard.h"
#include "helpers.hpp"
#include "pieceMovements.hpp"


void mapBitboardSquareToDescription(size_t placeNum, char* returnBuffer) {
    returnBuffer[0] = 'a' + placeNum % 8;
    returnBuffer[1] = '8' - placeNum / 8;
}

// moving from using std::string and std::format to a return buffer increased the performance by a factor of 3
// I cant accurately assess performance because of my cpus cstate
void moveToUci(const chessBoard& board, const pieceMovement& movement, char* returnBuffer) {
    bool isWhiteTurn= board.m_board_state & board_state::WhiteTurn;
    PieceType typeOfMovingPiece = isWhiteTurn ? movement.movement1WhiteBB : movement.movement1BlackBB;
    PieceType promotingPieceType = isWhiteTurn ? movement.movement2WhiteBB : movement.movement2BlackBB;
    // pawn promotion special case
    if (std::popcount(movement.movement) == 1 && typeOfMovingPiece == PieceType::Pawn) {
        // Rook = 0b1,
        // Knight = 0b10,
        // Bishop = 0b11,
        // Queen = 0b100,
        uint64_t mov = movement.movement | movement.secondMovement;
        const uint64_t* pawnPtr = board.getPiecesByColorConst(board.m_board_state & board_state::WhiteTurn);
        auto moveFrom = std::countr_zero(mov & (*pawnPtr)); 
        auto moveTo = std::countr_zero(mov ^ (mov & (*pawnPtr))); 
        constexpr char smallLookuptable[5] {'\0', 'r', 'n', 'b', 'q'};
        char promotion = smallLookuptable[promotingPieceType];
        mapBitboardSquareToDescription(moveFrom, returnBuffer);
        mapBitboardSquareToDescription(moveTo, returnBuffer + 2);
        returnBuffer[4] = promotion;
        returnBuffer[5] = '\0';
    }  else {
        // piece movment
        // prevent indexing out of the struct
        const uint64_t* pieceBoardPtr = board.getPiecesByColorConst(board.m_board_state & board_state::WhiteTurn) + typeOfMovingPiece;
        uint64_t mov = movement.movement;
        auto moveFrom = std::countr_zero(mov & (*pieceBoardPtr)); 
        auto moveTo = std::countr_zero(mov ^ (mov & (*pieceBoardPtr))); 
        mapBitboardSquareToDescription(moveFrom, returnBuffer);
        mapBitboardSquareToDescription(moveTo, returnBuffer + 2);
        returnBuffer[4] = '\0';
    }
}

int main (int argc, char *argv[]) {
    char myargstring[256];
    auto counter {0uz};
    for (auto arr : std::span(argv, argc)) {
        if (counter == 0) {
            counter ++;
            continue;
        }

        for (auto c : std::string_view(arr)) {
            assert(counter < 256);
            myargstring[counter-1] = c;
            counter ++;
        }   
        assert(counter < 256);
        myargstring[counter-1] = ' ';
        counter ++;
    }
    myargstring[counter-1] = '\0';
    std::string_view fenArgument = argc > 1 ? std::string_view(myargstring) : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    chessBoard board{fenArgument};
    chessBoard boardCopy{board};
    
    auto allMoves = chessMoves::makeAllMoves(board);

    std::println("using make while copying the chess board and discarding the copy");

    for (const auto& mv : allMoves) {
        char uciMove[6];
        moveToUci(board, mv, uciMove);
        std::print("{}", uciMove);
        chessBoard boardClone = board.applyMovePure(mv);
        auto allMovesOrder2 = chessMoves::makeAllMoves(boardClone);
        std::println(" {}", allMovesOrder2.numitems());
    }

    std::println("using unmake and make with make move impure!!\n");

    for (const auto& mv : allMoves) {
        char uciMove[6];
        moveToUci(boardCopy, mv, uciMove);
        std::print("{}", uciMove);
        boardCopy.applyMoveImpure(mv);
        auto allMovesOrder2 = chessMoves::makeAllMoves(boardCopy);
        boardCopy.applyMoveImpure(mv);
        std::println(" {}", allMovesOrder2.numitems());
    }
    

    std::println("{} moves in this position", allMoves.numitems());

    return 0;
}
