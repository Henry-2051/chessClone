#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <sys/types.h>
#include <utility>
#include "stackStack.hpp"
namespace chessMoves {

enum ScanState
{
    hitFriendlyPiece,
    hitEnemyKing,
    continueIteration
};

inline uint64_t
identityMove(uint64_t)

inline uint64_t
blackPawnMove(uint64_t black_pawns, uint64_t enemies, uint64_t friendly)
{
    uint64_t enemy_or_empty = ~friendly;
    uint64_t front_pawn_row = 0xff00;
    uint64_t black_pawns_move =
       (((black_pawns << 8) | 
        ((black_pawns & front_pawn_row) << 16)) & enemy_or_empty) |
        ((1ULL << 7) & enemies ? (1ULL << 7) : 0ULL) |
        ((1ULL << 9) & enemies ? (1ULL << 9) : 0ULL);
    return black_pawns_move;
}

inline uint64_t
knightMove(uint64_t knights, uint64_t enemies, uint64_t friendly)
{
    uint64_t enemy_or_empty = ~ friendly;
    uint64_t black_knight_move =
        ((knights >> 15) + (knights >> 17) | (knights >> 10) | (knights << 6) |
         (knights >> 6) | (knights << 10) | (knights << 15) | (knights << 17)) &
        enemy_or_empty;
    return black_knight_move;
}

inline uint64_t
whitePawnMove(uint64_t white_pawns, uint64_t enemies, uint64_t friendly)
{
    uint64_t enemy_or_empty = ~friendly;
    uint64_t front_pawn_row = 0xff000000000000;
    uint64_t white_pawns_move =
        ((white_pawns >> 8) |
        (((white_pawns & front_pawn_row) >> 16) & enemy_or_empty)) |
        ((1ULL >> 7) & enemies ? (1ULL >> 7) : 0ULL) |
        ((1ULL >> 9) & enemies ? (1ULL) >> 9 : 0ULL);
    return white_pawns_move;
}

inline uint64_t
_whitePawnMoveEPP(uint64_t white_pawns, uint64_t enemies) {
    uint64_t ep_capture = 0 | ((white_pawns << 1) & enemies) >> 8 | ((white_pawns >> 1) & enemies) >> 8;
    return ep_capture;
}

inline uint64_t 
_blackPawnMoveEPP(uint64_t black_pawns, uint64_t enemies) {
    uint64_t ep_capture = 0 | ((black_pawns << 1) & enemies) << 8 | ((black_pawns >> 1) & enemies) << 8;
    return ep_capture;
}

inline uint64_t
whitePawnMoveEPP(uint64_t white_pawns, uint64_t enemies, uint64_t friendly) {
    return whitePawnMove(white_pawns, enemies, friendly) | _whitePawnMoveEPP(white_pawns, enemies);
}

inline uint64_t
blackPawnMoveEPP(uint64_t black_pawns, uint64_t enemies, uint64_t friendly) {
    return blackPawnMove(black_pawns, enemies, friendly) | _blackPawnMoveEPP(black_pawns, enemies);
}

template<typename LoopingFunction>
uint64_t scanPinRay(uint64_t friendly, uint64_t enemy_king, int rook_rank, int rook_file, int dx, int dy, LoopingFunction loop_checker) {

    auto attack_square =
        [friendly, enemy_king](uint64_t looking_at) {
            if (looking_at & enemy_king) {
                return hitEnemyKing;
            }
            if (looking_at & friendly) {
                return hitFriendlyPiece;
            }
            return continueIteration;
        };
    
    uint64_t trial_attack = 0;
    for (int r = rook_rank + dy, f = rook_file + dx; loop_checker(r,f); r += dy, f += dx) {
        uint64_t looking_at = 1ULL << (f + 8 * r);
        ScanState temp = attack_square(looking_at);
        if (temp == ScanState::continueIteration) {
            trial_attack |= looking_at;
        } else if (temp == ScanState::hitFriendlyPiece) {
            return 0;
        } else if (temp == ScanState::hitEnemyKing) {
            return trial_attack;
        } else {
            throw std::runtime_error("in _singleRookPin, fell through state if statement, invalid program state\n");
        }
    };
    return 0;
}

inline uint64_t
singleRookPin(int rook_place,
              uint64_t enemy,
              uint64_t friendly,
              uint64_t enemy_king)
{
    int rook_rank = rook_place / 8;
    int rook_file = rook_place % 8;

    // logic above is correct, below is how I would like the above code to look
    // but the below code is not logically correct
    
    auto innerPinChecker = [friendly, enemy_king, rook_rank, rook_file](int dx, int dy, auto loop_checker) {
        return scanPinRay(friendly, enemy_king, rook_rank, rook_file, dx, dy, loop_checker);
    };


    return innerPinChecker(1, 0,  [](int r, int f){return (f <= 7);}) 
         | innerPinChecker(-1,0,  [](int r, int f){return (f >= 0);}) 
         | innerPinChecker(0, 1,  [](int r, int f){return (r <= 7);})
         | innerPinChecker(0,-1,  [](int r, int f){return (r >= 0);});
}

inline uint64_t
singleBishopPin(int bishop_place, uint64_t enemy, uint64_t friendly, uint64_t enemy_king) 
{
    int bishop_rank = bishop_place / 8;
    int bishop_file = bishop_place % 8;

    auto innerPinChecker = [friendly, enemy_king, bishop_rank, bishop_file](int dx, int dy, auto loop_checker) {
        return scanPinRay(friendly, enemy_king, bishop_rank, bishop_file, dx, dy, loop_checker);
    };

    return innerPinChecker(1, 1,  [](int r, int f){return (f <= 7) & (r <= 7) ;})
         | innerPinChecker(1,-1,  [](int r, int f){return (f <= 7) & (r >= 0) ;})
         | innerPinChecker(-1,1,  [](int r, int f){return (f >= 0) & (r <= 7) ;})  
         | innerPinChecker(-1,-1, [](int r, int f){return (f >= 0) & (r >= 0) ;});
}

inline uint64_t
singleQueenPin(int queen_place, uint64_t enemy, uint64_t friendly, uint64_t enemy_king) 
{
    int queen_rank = queen_place / 8;
    int queen_file = queen_place % 8;

    auto innerPinChecker = [friendly, enemy_king, queen_rank, queen_file](int dx, int dy, auto loop_checker) {
        return scanPinRay(friendly, enemy_king, queen_rank, queen_file, dx, dy, loop_checker);
    };

    return innerPinChecker(1, 0,  [](int r, int f){return (f <= 7);}) 
         | innerPinChecker(-1,0,  [](int r, int f){return (f >= 0);}) 
         | innerPinChecker(0, 1,  [](int r, int f){return (r <= 7);})
         | innerPinChecker(0,-1,  [](int r, int f){return (r >= 0);})
         | innerPinChecker(1, 1,  [](int r, int f){return (f <= 7) & (r <= 7) ;})
         | innerPinChecker(1,-1,  [](int r, int f){return (f <= 7) & (r >= 0) ;})
         | innerPinChecker(-1,1,  [](int r, int f){return (f >= 0) & (r <= 7) ;})  
         | innerPinChecker(-1,-1, [](int r, int f){return (f >= 0) & (r >= 0) ;});
}

inline uint64_t
_singleRookMove(uint64_t the_ROOOOK,
                int rook_place,
                uint64_t enemy,
                uint64_t friendly)
{
    uint64_t attacked_squares = 0;

    int rook_rank = rook_place / 8;
    int rook_file = rook_place % 8;

    auto attack_square = [&attacked_squares, enemy, friendly](int rr, int ff) {
        uint64_t looking_at = 1ULL << (ff + 8 * rr);
        if (looking_at & enemy) {
            attacked_squares |= looking_at;
            return true;
        } else if (looking_at & friendly) {
            return true;
        }

        attacked_squares |= looking_at;
        return false;
    };

    for (int r = rook_rank, f = rook_file + 1; f <= 7; f++) {
        if (attack_square(r, f)) {
            break;
        };
    };
    for (int r = rook_rank, f = rook_file - 1; f >= 0; f--) {
        if (attack_square(r, f)) {
            break;
        };
    };
    for (int r = rook_rank + 1, f = rook_file; r <= 7; r++) {
        if (attack_square(r, f)) {
            break;
        };
    };
    for (int r = rook_rank - 1, f = rook_file; r >= 0; r--) {
        if (attack_square(r, f)) {
            break;
        };
    };

    return attacked_squares;
}

inline uint64_t
singleRookMove(int rook_place, uint64_t enemy, uint64_t friendly)
{
    uint64_t the_ROOOOK = 1ULL << rook_place;
    uint64_t attacked_squares =
        _singleRookMove(the_ROOOOK, rook_place, enemy, friendly);
    return attacked_squares;
}

inline uint64_t
_singleBishopMove(uint64_t the_bishop,
                  int bishop_place,
                  uint64_t enemy,
                  uint64_t friendly)
{
    uint64_t attacked_squares = 0;

    int bishop_rank = bishop_place / 8;
    int bishop_file = bishop_place % 8;

    auto attack_square = [&attacked_squares, enemy, friendly](int rr, int ff) {
        uint64_t looking_at = 1ULL << (ff + 8 * rr);
        if (looking_at & enemy) {
            attacked_squares |= looking_at;
            return true;
        } else if (looking_at & friendly) {
            return true;
        }

        attacked_squares |= looking_at;
        return false;
    };

    for (int r = bishop_rank + 1, f = bishop_file + 1; f <= 7 && r <= 7;
         f++, r++) {
        if (attack_square(r, f)) {
            break;
        };
    };
    for (int r = bishop_rank + 1, f = bishop_file - 1; f >= 0 && r <= 7;
         f--, r++) {
        if (attack_square(r, f)) {
            break;
        };
    };
    for (int r = bishop_rank - 1, f = bishop_file + 1; f <= 7 && r >= 0;
         f++, r--) {
        if (attack_square(r, f)) {
            break;
        };
    };
    for (int r = bishop_rank - 1, f = bishop_file - 1; r >= 0 && f >= 0;
         f--, r--) {
        if (attack_square(r, f)) {
            break;
        };
    };

    return attacked_squares;
}
inline uint64_t
singleBishopMove(int bishop_place, uint64_t enemy, uint64_t friendly)
{
    uint64_t the_bishop = 1ULL << bishop_place;

    uint64_t attacked_squares =
        _singleBishopMove(the_bishop, bishop_place, enemy, friendly);

    return attacked_squares;
}

template<typename Function>
uint64_t
iterateThroughBitboard(uint64_t pieces,
                       uint64_t enemy,
                       uint64_t friendly,
                       Function operation)
{
    uint64_t resultBitboard = 0;
    int ctz_result{ __builtin_ctzll(pieces) };
    while (pieces != 0) {
        resultBitboard |= operation(ctz_result, enemy, friendly);
        pieces &= (pieces - 1);
        ctz_result = __builtin_ctzll(pieces);
    }

    return resultBitboard;
}

template<typename Function>
uint64_t
iterateThroughBitboard_pin(uint64_t pieces,
                           uint64_t enemy,
                           uint64_t friendly,
                           Function operation,
                           uint64_t enemy_king)
{
    uint64_t resultBitboard = 0;
    int ctz_result{ __builtin_ctzll(pieces) };
    while (pieces != 0) {
        resultBitboard |= operation(ctz_result, enemy, friendly, enemy_king);
        pieces &= (pieces - 1);
        ctz_result = __builtin_ctzll(pieces);
    }

    return resultBitboard;
}

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

template <size_t Cap>
stackStack<uint64_t, Cap>
seperateBitboardIntoStack(uint64_t pieces) {
    std::pair<std::array<uint64_t, Cap>, std::size_t> result = seperateBitboard<Cap>(pieces); 
    return stackStack(result.first, result.second);
}

inline uint64_t
rookPins(uint64_t rooks, uint64_t enemy, uint64_t friendly, uint64_t enemy_king)
{
    return iterateThroughBitboard_pin(rooks, enemy, friendly, singleRookPin, enemy_king);
}

inline uint64_t
rookMove(uint64_t rooks, uint64_t enemy, uint64_t friendly)
{
    return iterateThroughBitboard(rooks, enemy, friendly, singleRookMove);
}

inline uint64_t
bishopMove(uint64_t bishops, uint64_t enemy, uint64_t friendly)
{
    return iterateThroughBitboard(bishops, enemy, friendly, singleBishopMove);
}
inline uint64_t
queenMove(uint64_t black_queens, uint64_t enemy, uint64_t friendly)
{
    auto singleQueenMove = [](int count, uint64_t friendly, uint64_t enemy) {
        return singleBishopMove(count, enemy, friendly) |
               singleRookMove(count, enemy, friendly);
    };
    return iterateThroughBitboard(
        black_queens, enemy, friendly, singleQueenMove);
}
}
