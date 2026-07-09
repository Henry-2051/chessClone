#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <sys/types.h>
#include <tuple>
#include <utility>
#include "stackStack.hpp"
#include "boardState.hpp"
#include "chessBoard.h"
#include "seperateBitboard.hpp"
#include "helpers.hpp"

namespace chessMoves {

uint64_t generateSimpleBlackPawnCaptureNoTeleport(uint64_t black_pawn, uint64_t occupied);

uint64_t generateSimpleWhitePawnCaptureNoTeleport(uint64_t white_pawn, uint64_t occupied);

FastStack<uint64_t, 13> calculate_pin_lines(const chessBoard& board);

uint64_t singleRookMoveNoPinOrCheck_forLoop(uint64_t rook, uint64_t enemies, uint64_t friendly);

uint64_t singleRookMove(uint64_t rook, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks);

// bihop!!!!
uint64_t singleBihopMoveNoPinOrCheck_forLoop(uint64_t bishop, uint64_t enemies, uint64_t friendly);

uint64_t singleBishopMove(uint64_t bishop, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks);

uint64_t singleQueenMove(uint64_t queen, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks);

uint64_t generateKnightMovesNoPinCheckTeleport(uint64_t knight, uint64_t friendly);

// evil and intimidating horse
uint64_t singleKnightMove(uint64_t knight, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks);

// we can pass in the sliding piece attacks since we now know this function will only be ran once per turn due to simply not generating moves that end with us in check
// passing in the sliding piece attacks saves on computation, limiting the number of sliding piece calculations we need to perform
FastStack<uint64_t, 2> calculateChecks(bool isWhiteTheColorBeingChecked, const chessBoard& board);

uint64_t dummyKingMoveGenerationNoTeleportation(uint64_t king, uint64_t friendly);

// must also take into account the enemies attacked squares so we must either pass in an array of attack lines or a single bitboard of all the attacked squares, 
// passing the array seems like the better option since 
stackStack218 makeAllMoves(const chessBoard& boardInput);
}
