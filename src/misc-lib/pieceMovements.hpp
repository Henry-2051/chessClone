#include <cstdint>
#include <sys/types.h>
#include <utility>
#include "stackStack.hpp"
#include "boardState.hpp"
#include "chessBoard.h"
#pragma once

namespace chessMoves {

struct movegenEngineData {
    uint64_t enemyAttacksMushed;
};

// uint64_t generateSimpleBlackPawnCaptureNoTeleport(uint64_t black_pawn, uint64_t occupied);

// uint64_t generateSimpleWhitePawnCaptureNoTeleport(uint64_t white_pawn, uint64_t occupied);

// FastStack<uint64_t, 13> calculate_pin_lines(const chessBoard& board);

// uint64_t singleRookMoveNoPinOrCheck_forLoop(uint64_t rook, uint64_t enemies, uint64_t friendly);

// uint64_t singleRookMove(uint64_t rook, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks);

// bihop!!!!
// uint64_t singleBihopMoveNoPinOrCheck_forLoop(uint64_t bishop, uint64_t enemies, uint64_t friendly);

// uint64_t singleBishopMove(uint64_t bishop, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks);

// uint64_t singleQueenMove(uint64_t queen, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks);

// uint64_t generateKnightMovesNoPinCheckTeleport(uint64_t knight, uint64_t friendly);

// evil and intimidating horse
// uint64_t singleKnightMove(uint64_t knight, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks);

// we can pass in the sliding piece attacks since we now know this function will only be ran once per turn due to simply not generating moves that end with us in check
// passing in the sliding piece attacks saves on computation, limiting the number of sliding piece calculations we need to perform
// FastStack<uint64_t, 2> calculateChecks(bool isWhiteTheColorBeingChecked, const chessBoard& board);

// uint64_t dummyKingMoveGenerationNoTeleportation(uint64_t king, uint64_t friendly);

stackStack218 makeAllMoves(const chessBoard& boardInput);

// in the above function the moveStack is a static variable, which means its the same accross all threads, 
// if we have 2 movegens running on seperate threads they will corrupt each others data 
// and for performance reasons we dont want to allocate and then copy out movestacks, 5.2 KB is too much and 
// doing this measurably degrades performance
movegenEngineData makeAllMovesWithDataReturn(const chessBoard& boardInput, stackStack218& moveStack);

bool makeMovesFromUciSequence(chessBoard& board, std::string_view uciSeq);
}
