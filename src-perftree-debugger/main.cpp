#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <format>
#include <map>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include "chessBoard.h"
#include "helpers.hpp"
#include "pieceMovements.hpp"
#include "timer.hpp"


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

template <bool recursiveCall>
void perftreeRun(size_t perftnumber, chessBoard& board, size_t& numMoves) {
    stackStack218 allMoves = chessMoves::makeAllMoves(board);

    if(recursiveCall && perftnumber > 0) 
    {
        for (const auto& mv : allMoves) {
            board.applyMoveImpure(mv);
            perftreeRun<true>(perftnumber - 1, board, numMoves);
            board.applyMoveImpure(mv);
        }
    } 
    else if (recursiveCall && perftnumber == 0) 
    {
        numMoves += allMoves.numitems();
    } 
    else if (!recursiveCall && perftnumber > 0) 
    {
        numMoves = 0;

        for (const auto& mv : allMoves) {
            char uciMove[6];
            moveToUci(board, mv, uciMove);
            std::print("{}", uciMove);
            board.applyMoveImpure(mv);
            size_t movesPerMove {0};
            perftreeRun<true>(perftnumber - 1, board, movesPerMove);
            board.applyMoveImpure(mv);
            std::println(" {}", movesPerMove);
            numMoves += movesPerMove;
        }
        std::println("\n{}", numMoves);
    } 
    else
    {
        numMoves = allMoves.numitems();

        for (const auto& mv : allMoves) {
            char uciMove[6];
            moveToUci(board, mv, uciMove);
            std::println("{} 1", uciMove);
        }

        std::println("\n{}", numMoves);
    } 
}



void perftree(size_t perftnumber=1, std::string_view fenArgument="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", std::string_view movesToMake = "") {
    chessBoard board {fenArgument};

    if(movesToMake != "" && !chessMoves::makeMovesFromUciSequence(board, movesToMake))
        return;

    size_t numMoves;
    perftreeRun<false>(perftnumber-1, board, numMoves);
    
    // Timer<Timers::SlidingAttackRook>::printAverageTimeNanoseconds();
    //
    // Timer<Timers::SlidingAttackRook>::printTotalTimeMilliseconds();
}

int main (int argc, char *argv[]) {

    // perftreeRun<false>(3, board1);
    // perftree();
    switch(argc) {
        case (1) : perftree(); break;
        case (2) : perftree(std::stoul(argv[1])); break;
        case (3) : perftree(std::stoul(argv[1]), argv[2]); break;
        case (4) : perftree(std::stoul(argv[1]), argv[2], argv[3]); break;
    }

    return 0;
}
