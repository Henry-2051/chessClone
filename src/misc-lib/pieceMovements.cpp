#include "pieceMovements.hpp"
#include "chessBoard.h"
#include "helpers.hpp"
#include "timer.hpp"
#include <bit>
#include <cassert>
#include <cstdint>
#include <optional>
#include <print>
#include "seperateBitboard.hpp"


struct pawnMoveReturn {
    uint64_t normalMovedTo{0};
    uint64_t enPassantPawnCapture {0};
    uint64_t enPassantMoveToSquare {0};
};


namespace chessMoves {

static constexpr int how_the_rook_moves[4][2] = {
    { 1,  0},
    { -1, 0},
    {0,  1},
    {0, -1},
};

static constexpr int how_the_bishop_moves[4][2] = {
    { 1,  1},
    { 1, -1},
    {-1,  1},
    {-1, -1},
};

inline uint64_t applyPinsToPiece(uint64_t piece, uint64_t attacked_squares, const FastStack<uint64_t, 13>& pins) {
    for (const auto& p: pins) {
        if (p & piece) {
            attacked_squares &= p;
        }
    }
    return attacked_squares;
}

inline uint64_t applyChecksToPiece(uint64_t piece, uint64_t attacked_squares, const FastStack<uint64_t, 2>& checkingAttacks) {
    for (const auto& c : checkingAttacks) {
        attacked_squares &= c;
    }
    return attacked_squares;
}


inline uint64_t simpleWhitePawnCapture(uint64_t white_pawn, uint64_t enemies) {

    // Right bit shift is legal, ie wont teleport the pawn
    uint64_t haveRbs {0x1010101010100};
    uint64_t haveLbs {0x80808080808000};
    haveRbs = ~haveRbs;
    haveLbs = ~haveLbs;

    return (((white_pawn & haveLbs) >> 7) & enemies) |  (((white_pawn & haveRbs) >> 9) & enemies);
}

inline uint64_t simpleBlackPawnCapture(uint64_t black_pawn, uint64_t enemies) {

    uint64_t haveRbs {0x1010101010100};
    uint64_t haveLbs {0x80808080808000};
    haveRbs = ~haveRbs;
    haveLbs = ~haveLbs;
    
    return (((black_pawn & haveRbs)  << 7) & enemies) | (((black_pawn & haveLbs)  << 9) & enemies);
}

inline uint64_t simpleWhitePawnMovingForward(uint64_t white_pawn, uint64_t occupation) {
    uint64_t secondRank {0xff000000000000};

    uint64_t singleMove {(white_pawn >> 8) & (~occupation)};

    uint64_t doubleMove {((white_pawn & secondRank) >> 16) & (~occupation) & (singleMove >> 8)};
    return singleMove | doubleMove;
}

inline uint64_t simpleBlackPawnMovingForward(uint64_t black_pawn, uint64_t occupation) {
    uint64_t seventhRank {0xff00};

    uint64_t singleMove {(black_pawn << 8) & (~occupation)};

    uint64_t doubleMove {((black_pawn & seventhRank) << 16) & (~occupation) & (singleMove << 8)};
    return singleMove | doubleMove;
}

template <bool isWhiteTurn>
void
normalPawnMove(uint64_t pawn, uint64_t enemies, uint64_t friendly, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& enemyCheckingAttacks, pawnMoveReturn& returnVal)
{
    if (isWhiteTurn) {
        returnVal.normalMovedTo = simpleWhitePawnMovingForward(pawn, enemies | friendly) | simpleWhitePawnCapture(pawn, enemies);
    } else {
        returnVal.normalMovedTo = simpleBlackPawnMovingForward(pawn, enemies | friendly) | simpleBlackPawnCapture(pawn, enemies);
    }

    if (pinLines.notEmpty())
        returnVal.normalMovedTo = applyPinsToPiece(pawn, returnVal.normalMovedTo, pinLines);

    if (enemyCheckingAttacks.notEmpty())
        returnVal.normalMovedTo = applyChecksToPiece(pawn, returnVal.normalMovedTo, enemyCheckingAttacks);
}


template <bool isWhiteTurn>
void pawnMoveEPP(uint64_t pawn, uint64_t enemies, uint64_t friendly, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& enemyCheckingAttacks, uint8_t boardEPPState, pawnMoveReturn& returnStruct) {
    assert(boardEPPState != -1);
    // std::println("epp state of board, {}", board.enPassantState);

    uint64_t eppCaptureSquare = 1ULL << boardEPPState;

    // helpers::printBitboard(eppCaptureSquare);

    uint64_t locationMovedTo = isWhiteTurn ? simpleWhitePawnCapture(pawn, eppCaptureSquare) : simpleBlackPawnCapture(pawn, eppCaptureSquare);

    if (!locationMovedTo) {
        return;
    }

    uint64_t pawnWeCapture = isWhiteTurn ? locationMovedTo << 8 : locationMovedTo >> 8;
    // helpers::printBitboard(pawnWeCapture);

    if (pinLines.notEmpty())
        locationMovedTo = applyPinsToPiece(pawn, locationMovedTo, pinLines);

    if (enemyCheckingAttacks.notEmpty())
        pawnWeCapture = applyChecksToPiece(pawn, pawnWeCapture, enemyCheckingAttacks);

    if (locationMovedTo == 0 || pawnWeCapture == 0) {
        return;
    }

    returnStruct.enPassantMoveToSquare = locationMovedTo;
    returnStruct.enPassantPawnCapture = pawnWeCapture;
}


template <bool isWhiteTurn>
pawnMoveReturn
singlePawnMove(uint64_t attacking_pawn, uint64_t enemies, uint64_t friendly,  const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, int8_t enPassantState) {
    pawnMoveReturn retval;

    normalPawnMove<isWhiteTurn>(attacking_pawn, enemies, friendly, pinLines, checkingAttacks, retval);

    if (enPassantState != -1) 
        pawnMoveEPP<isWhiteTurn>(attacking_pawn, enemies, friendly,  pinLines, checkingAttacks, enPassantState, retval);

    return retval;
}

inline
uint64_t attacksForSlidingPiece(uint64_t enemies, uint64_t friendly, std::pair<int, int> starting_rf, std::pair<int, int> increment_rf) {
    uint64_t attacked_squares = 0;

    auto [r, f] = starting_rf;
    auto [dr, df] = increment_rf;

    r += dr;
    f += df;

    while (r < 8 && r >= 0 && f < 8 && f >= 0) {
        uint64_t looking_at = 1ULL << (f + 8 * r);

        if (looking_at & enemies) {
            return attacked_squares | looking_at;
        } 
        else if (looking_at & friendly) {
            return attacked_squares | looking_at; // here we 'attack' friendly pieces but this actually simulates defenders, when generating king moves, when generating rook, bishop or queen moves 
                                                  // we actually rectify this with &= ~friendly so we dont allow capturing friendly pieces
        }

        attacked_squares |= looking_at;

        r += dr;
        f += df;
    };
    return attacked_squares;
};

uint64_t pseudoLegalRookMoves(uint64_t rook, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::AttackCreationRook> t{};
    int rook_place = __builtin_ctzll(rook);

    int rook_rank = rook_place / 8;
    int rook_file = rook_place % 8;

    uint64_t attacked_squares = 0;

    for (const auto &d : how_the_rook_moves) {
        attacked_squares |= attacksForSlidingPiece(
            enemies, friendly,
            {rook_rank, rook_file},
            {d[0], d[1]}
        );
    }

    return attacked_squares;
}

uint64_t pseudoLegalBishopMoves(uint64_t bishop, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::AttackCreationBishop> t{};
    int bishop_place = __builtin_ctzll(bishop);

    int bishop_rank = bishop_place / 8;
    int bishop_file = bishop_place % 8;

    uint64_t attacked_squares = 0;

    for (auto &d : how_the_bishop_moves) {
        attacked_squares |= attacksForSlidingPiece(
            enemies, friendly,
            {bishop_rank, bishop_file},
            {d[0], d[1]}
        );
    }
    return attacked_squares;
}

// using AttackGen = uint64_t (*) (uint64_t, uint64_t, uint64_t);
// template <AttackGen aGen>
// requires(aGen == pseudoLegalBishopMoves || aGen == pseudoLegalRookMoves)
// inline uint64_t addPinsToPinArray(uint64_t candiatePinners, uint64_t friendly_king, uint64_t enemies, uint64_t friendly,FastStack<uint64_t, 13>& pins) {
//
//     uint64_t kingRays {aGen(friendly_king, enemies, 0)};
//     uint64_t pinsMushed {0};
//
//     candiatePinners &= kingRays;
//     while (candiatePinners) {
//         uint64_t candiate {candiatePinners & -candiatePinners};
//         candiatePinners &= candiatePinners -1;
//
//         uint64_t candiatePin {(aGen(candiate, friendly_king, enemies) | candiate) & kingRays};
//
//         if (std::popcount(candiatePin & friendly) == 1) {
//             pins.pushVal(candiatePin);
//             pinsMushed |= candiatePin;
//         }
//     }
//     return pinsMushed;
// }

inline uint64_t calculatePinLineMasks(const chessBoard& board, FastStack<uint64_t, 13>& pinArray, uint64_t enemies, uint64_t friendly, bool isWhiteTurn) {
    // Timer<Timers::MakeAllMoves> t{};

    uint64_t pinsMushed {0};
    uint64_t friendly_king = isWhiteTurn ? board.bitboards[PieceType::King + 6] : board.bitboards[PieceType::King];

    uint64_t kingScanRayRookLike = pseudoLegalRookMoves(friendly_king, enemies, 0);
    uint64_t kingScanRayBishopLike = pseudoLegalBishopMoves(friendly_king, enemies, 0);

    uint64_t enemyQueens {board.pieceToBitboardConst<PieceType::Queen>(!isWhiteTurn)};
    uint64_t enemyRooksAndQueens  {board.pieceToBitboardConst<PieceType::Rook>(!isWhiteTurn) | enemyQueens};
    uint64_t enemyBishopsAndQueens {board.pieceToBitboardConst<PieceType::Bishop>(!isWhiteTurn) | enemyQueens};
    
    // pinsMushed |= addPinsToPinArray<pseudoLegalRookMoves>(enemyRooksAndQueens, friendly_king, enemies, friendly, pinArray);
    // pinsMushed |= addPinsToPinArray<pseudoLegalBishopMoves>(enemyBishopsAndQueens, friendly_king, enemies, friendly, pinArray);


    // old version is a lot cleaner than the templated version
    if (kingScanRayRookLike & enemyRooksAndQueens) {
        uint64_t seenRooks {kingScanRayRookLike & enemyRooksAndQueens};
        while (seenRooks) {
            uint64_t piece = seenRooks & -seenRooks;
            seenRooks &= seenRooks -1;

            uint64_t rookRayIncRook {pseudoLegalRookMoves(piece, friendly_king, enemies) | piece};
            uint64_t candiatePin {kingScanRayRookLike & rookRayIncRook};

            if (std::popcount(candiatePin & friendly) == 1) {
                pinArray.push(std::move(candiatePin));
                pinsMushed |= candiatePin;
            }
        }
    }


    if (kingScanRayBishopLike & enemyBishopsAndQueens) {
        uint64_t seenBishops {kingScanRayBishopLike & enemyBishopsAndQueens};
        while (seenBishops) {
            uint64_t piece = seenBishops & -seenBishops;
            seenBishops &= seenBishops -1;

            uint64_t bishopRayIncBishop = pseudoLegalBishopMoves(piece, friendly_king, enemies) | piece;
            uint64_t candiatePin = kingScanRayBishopLike & bishopRayIncBishop;

            if (std::popcount(candiatePin & friendly) == 1) {
                pinArray.push(std::move(candiatePin));
                pinsMushed |= candiatePin;
            }
        }
    }
    return pinsMushed;
}


inline uint64_t singleRookMove(uint64_t rook, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::SlidingAttackRook> t{};

    uint64_t attacked_squares = pseudoLegalRookMoves(rook, enemies, friendly);

    attacked_squares ^= (attacked_squares & friendly); // un attacks the friendly pieces
    
    if (pinLines.notEmpty())
        attacked_squares = applyPinsToPiece(rook, attacked_squares, pinLines);

    if (checkingAttacks.notEmpty())
        attacked_squares = applyChecksToPiece(rook, attacked_squares, checkingAttacks);

    return attacked_squares;
}


inline uint64_t singleBishopMove(uint64_t bishop, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::SlidingAttackBishop> t{};
    uint64_t attacked_squares = pseudoLegalBishopMoves(bishop, enemies, friendly);

    attacked_squares ^= (attacked_squares & friendly); // un attacks the friendly pieces
    if (pinLines.notEmpty())
        attacked_squares = applyPinsToPiece(bishop, attacked_squares, pinLines);
    if (checkingAttacks.notEmpty())
        attacked_squares = applyChecksToPiece(bishop, attacked_squares, checkingAttacks);

    return attacked_squares;
}

inline uint64_t singleQueenMove(uint64_t queen, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::QueenAttack> t{};
    uint64_t attacked_squares = pseudoLegalRookMoves(queen, enemies, friendly) | pseudoLegalBishopMoves(queen, enemies, friendly);

    attacked_squares &= (~friendly); // un attacks the friendly pieces
    
    if (pinLines.notEmpty())
        attacked_squares = applyPinsToPiece(queen, attacked_squares, pinLines);
    if (checkingAttacks.notEmpty())
        attacked_squares = applyChecksToPiece(queen, attacked_squares, checkingAttacks);

    return attacked_squares;
}

inline uint64_t pseudoLegalKnightMoves(uint64_t knight, uint64_t friendly) {
    // key:
    // top right right bitshift
    // 0-=-=-=-=-=-=-7
    // . . . . . . . . 
    // . . . . . X . . 
    // . . . . . . X . 
    // . . . . # . . . 
    // . . . . . . . . 
    // . . . . . . . . 
    // . . . . . . . . 
    // . . . . . . . . 
    // 56-=-=-=-=-=-63
    int trrBS1 = 15;  // up2right1
    int trrBS2 = 6;   // up1right2
    int tlrBS1 = 17;  // up2left1
    int tlrBS2 = 10;  // up1left2

    int brlBS1 = 17;  // down2right1
    int brlBS2 = 10;  // down1right2
    int bllBS1 = 15;  // down2left1
    int bllBS2 = 6;   // down1left2

    uint64_t trrBS1_legal {0x808080808080ffff};
    uint64_t trrBS2_legal {0xc0c0c0c0c0c0c0ff};
    uint64_t tlrBS1_legal {0x10101010101ffff};
    uint64_t tlrBS2_legal {0x3030303030303ff};

    uint64_t brlBS1_legal {0xffff808080808080};
    uint64_t brlBS2_legal {0xffc0c0c0c0c0c0c0};
    uint64_t bllBS1_legal {0xffff010101010101};
    uint64_t bllBS2_legal {0xff03030303030303};

    trrBS1_legal = ~trrBS1_legal;
    trrBS2_legal = ~trrBS2_legal;
    tlrBS1_legal = ~tlrBS1_legal;
    tlrBS2_legal = ~tlrBS2_legal;
    brlBS1_legal = ~brlBS1_legal;
    brlBS2_legal = ~brlBS2_legal;
    bllBS1_legal = ~bllBS1_legal;
    bllBS2_legal = ~bllBS2_legal;

    uint64_t attacked_squares {
        (knight & trrBS1_legal) >> trrBS1 | 
        (knight & trrBS2_legal) >> trrBS2 |
        (knight & tlrBS1_legal) >> tlrBS1 |
        (knight & tlrBS2_legal) >> tlrBS2 |
        (knight & brlBS1_legal) << brlBS1 |
        (knight & brlBS2_legal) << brlBS2 |
        (knight & bllBS1_legal) << bllBS1 |
        (knight & bllBS2_legal) << bllBS2 
    };

    return attacked_squares & (~friendly);
}

// evil and intimidating horse
inline uint64_t singleKnightMove(uint64_t knight, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, uint64_t friendly) {
    // std::println("generating knight move, checks here : ");
    // for (auto ch : checkingAttacks) {
    //     helpers::printBitboard(ch);
    // }
    uint64_t attacked_squares = pseudoLegalKnightMoves(knight, friendly);

    if (pinLines.notEmpty())
        attacked_squares = applyPinsToPiece(knight, attacked_squares, pinLines);
    if (checkingAttacks.notEmpty())
        attacked_squares = applyChecksToPiece(knight, attacked_squares, checkingAttacks);

    return attacked_squares;
}

inline void calculateCheckMasks(bool isWhiteTheColorBeingChecked, const chessBoard& board, uint64_t enemies, uint64_t friendly, FastStack<uint64_t, 2>& checks, uint64_t enemyPawns) {
    // Timer<Timers::ComputeCheckMasks> t{};
   
    uint64_t king_possibly_checked = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::King + 6] : board.bitboards[PieceType::King];

    uint64_t kingSeesLikePawn = isWhiteTheColorBeingChecked ? simpleWhitePawnCapture(king_possibly_checked, enemyPawns) : simpleBlackPawnCapture(king_possibly_checked, enemyPawns); 
    uint64_t kingScanRayBishopLike = pseudoLegalBishopMoves(king_possibly_checked, enemies, friendly);
    uint64_t kingScanRayRookLike = pseudoLegalRookMoves(king_possibly_checked, enemies, friendly);

    // uint64_t center = 0x3c3c3c3c0000;
    uint64_t kingSeesLikeKnight = pseudoLegalKnightMoves(king_possibly_checked, friendly);
    // if (king_possibly_checked & center)
    //     kingSeesLikeKnight = pseudoLegalKnightMoves<false>(king_possibly_checked, friendly);
    // else
    //     kingSeesLikeKnight = pseudoLegalKnightMoves<true>(king_possibly_checked, friendly);


    if (kingSeesLikePawn & enemyPawns) {
        checks.push(kingSeesLikePawn & enemyPawns);
    }

    uint64_t enemyKnights = board.pieceToBitboardConst<PieceType::Knight>(!isWhiteTheColorBeingChecked);
    if (kingSeesLikeKnight & enemyKnights) {
        // we cant be double checked by 2 knights since this would require a discovery and knights cant be blocked 
        checks.push(kingSeesLikeKnight & enemyKnights);
    }

    // its still check if the enemy piece is pinned, since checkmate is capturing the opponents king even through this capture is never played
    
    uint64_t enemyRooks = board.pieceToBitboardConst<PieceType::Rook>(!isWhiteTheColorBeingChecked);
    if (kingScanRayRookLike & enemyRooks) {
        assert(std::popcount(kingScanRayRookLike & enemyRooks) == 1);
        uint64_t enemyRookAttack = pseudoLegalRookMoves(kingScanRayRookLike & enemyRooks, friendly, enemies);
        if (enemyRookAttack & king_possibly_checked) {
            checks.push((enemyRookAttack & kingScanRayRookLike) | (kingScanRayRookLike & enemyRooks));
        }
    }

    uint64_t enemyBishops = board.pieceToBitboardConst<PieceType::Bishop>(!isWhiteTheColorBeingChecked);
    // same as for the rooks, if we are on a diagonal with a king then we need two moves to check from another diagonal therefore we cant have 2 bishop checks
    if (kingScanRayBishopLike & enemyBishops) {
        assert(std::popcount(kingScanRayBishopLike & enemyBishops) == 1);
        uint64_t enemyBishopAttack = pseudoLegalBishopMoves(kingScanRayBishopLike & enemyBishops, friendly, enemies);
        if (enemyBishopAttack & king_possibly_checked) {
            checks.push((enemyBishopAttack & kingScanRayBishopLike) | (kingScanRayBishopLike & enemyBishops));
        }
    }

    uint64_t kingScanRayQueenLike = kingScanRayBishopLike | kingScanRayRookLike;
    uint64_t enemyQueens = board.pieceToBitboardConst<PieceType::Queen>(!isWhiteTheColorBeingChecked);
    if (kingScanRayQueenLike & enemyQueens) {
        uint64_t checkingQueens {kingScanRayQueenLike & enemyQueens};
        while (checkingQueens) {
            uint64_t chQueen {1ULL << std::countr_zero(checkingQueens)};
            checkingQueens &= ~chQueen;
            // it is important to deconstruct the attack like this or we may move a defending piece to any spot where the rays intersect instead of only being able to block and capture 
            uint64_t rookLikeAttack = pseudoLegalRookMoves(chQueen, friendly, enemies);
            uint64_t bishopLikeAttack = pseudoLegalBishopMoves(chQueen, friendly, enemies);

            uint64_t rooklikeAttackIncPiece = rookLikeAttack | chQueen;
            uint64_t bishoplikeAttackIncPiece = bishopLikeAttack | chQueen;

            if (rooklikeAttackIncPiece & king_possibly_checked) 
            {
                checks.push(rooklikeAttackIncPiece & kingScanRayRookLike);
            } 
            else if (bishoplikeAttackIncPiece & king_possibly_checked) 
            {
                checks.push(bishoplikeAttackIncPiece & kingScanRayBishopLike);
            }
        }
    }
}

inline uint64_t pseudoLegalKingMoves(uint64_t king, uint64_t friendly) {
    uint64_t moveUpLegal    {0xff};
    uint64_t moveDownLegal  {0xff00000000000000};
    uint64_t moveRightLegal {0x8080808080808080};
    uint64_t moveLeftLegal  {0x101010101010101};

    moveUpLegal    = ~moveUpLegal   ; 
    moveDownLegal  = ~moveDownLegal ; 
    moveRightLegal = ~moveRightLegal;   
    moveLeftLegal  = ~moveLeftLegal ;   

    // key up right(>>) bishift 
    int up__rBS {8}; // up
    int lef_rBS {1}; // left
    int dow_lBS {8}; // down
    int rig_lBS {1}; // right

    uint64_t attackedSquares {
        (king & moveUpLegal)    >> up__rBS | 
        (king & (moveUpLegal   & moveLeftLegal))  >> (up__rBS + lef_rBS) |
        (king & (moveUpLegal   & moveRightLegal)) >> (up__rBS - rig_lBS) |
        (king & moveDownLegal)  << dow_lBS |
        (king & (moveDownLegal & moveLeftLegal))  << (dow_lBS - lef_rBS) |
        (king & (moveDownLegal & moveRightLegal)) << (up__rBS + rig_lBS) |
        (king & moveRightLegal) << rig_lBS |
        (king & moveLeftLegal)  >> lef_rBS 
    };

    return attackedSquares & ~friendly;
};


// this function does way to much, should have seperate functions for right and left castling, and push to the stack in the outer function 
inline 
uint64_t singleKingMove(uint64_t king, const chessBoard& board, uint64_t attackMask, uint64_t enemies, uint64_t friendly, stackStack218& moves) {
    // Timer<Timers::KingMoveFunction> t{};
    uint64_t attacked_squares {pseudoLegalKingMoves(king, friendly)};

    // helpers::printBitboard(retVal.normalMoves);

    attacked_squares &= ~attackMask;
    attacked_squares &= ~friendly;

    return attacked_squares;
}

inline
std::optional<pieceMovement> kingCastlingShort(uint64_t king, const chessBoard& board, bool isWhiteTurn, uint64_t all_pieces, uint64_t attackMask) {

    if (((board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsRight : board_state::BlacklostCastlingRightsRight))))
        return std::nullopt;

    uint64_t bKingStart = 0x10, bSquaresEmptyAndNotAttacked = 0x60, bShortRookStart = 0x80;
    uint64_t squaresNotAttacked = bKingStart | bSquaresEmptyAndNotAttacked;
    uint64_t squaresNotOccupied = bSquaresEmptyAndNotAttacked;
    // check the squares have what they should, redundant by design with the above check
    if((isWhiteTurn ? squaresNotAttacked << 56 : squaresNotAttacked) & attackMask)
        return std::nullopt;

    if ((isWhiteTurn ? squaresNotOccupied << 56 : squaresNotOccupied) & all_pieces)
        return std::nullopt;

    if (!(board.pieceToBitboardConst<PieceType::Rook>(isWhiteTurn) & (isWhiteTurn ? bShortRookStart << 56 : bShortRookStart)))
        return std::nullopt;
    
    uint8_t boardStateToXor = isWhiteTurn ? 
            board_state::WhiteTurn | board_state::WhiteLostCastlingRightsRight | board_state::WhiteLostCastlingRightsLeft : 
            board_state::WhiteTurn | board_state::BlacklostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft;

    boardStateToXor ^= board.m_board_state & boardStateToXor & board_state::allCastlingFields_const;

    return (isWhiteTurn ?
        pieceMovement{
            (bKingStart << 2 | bKingStart) << 56, 
            (bShortRookStart | bShortRookStart >> 2) << 56, 
            PieceType::King, PieceType::NotAPiece,
            PieceType::Rook, PieceType::NotAPiece, 
            static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)),
            board.enPassantState, boardStateToXor
        } :
        pieceMovement{
            bKingStart << 2 | bKingStart,
            bShortRookStart | bShortRookStart >> 2,
            PieceType::NotAPiece, PieceType::King, 
            PieceType::NotAPiece, PieceType::Rook, 
            static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)),
            board.enPassantState, boardStateToXor
    });
    
}

inline 
std::optional<pieceMovement> kingCastlingLong(uint64_t king, const chessBoard& board, bool isWhiteTurn, uint64_t all_pieces, uint64_t attackMask) {

    if ((board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsLeft : board_state::BlackLostCastlingRightsLeft)))
        return std::nullopt;

    uint64_t bKingStart = 0x10, bLongRookStart = 0x1, bSquareNotAttacked= 0xc, bSquaresEmpty = 0xe;

    if ((isWhiteTurn ? bSquaresEmpty << 56 : bSquaresEmpty) & all_pieces)
        return std::nullopt;

    if ((isWhiteTurn ? bSquareNotAttacked << 56 : bSquareNotAttacked) & attackMask)
        return std::nullopt;

    if (!(board.pieceToBitboardConst<PieceType::Rook>(isWhiteTurn) & (isWhiteTurn ? bLongRookStart << 56 : bLongRookStart)))
        return std::nullopt;

    uint8_t boardStateToXor = isWhiteTurn ? 
            board_state::WhiteTurn | board_state::WhiteLostCastlingRightsRight | board_state::WhiteLostCastlingRightsLeft : 
            board_state::WhiteTurn | board_state::BlacklostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft;

    boardStateToXor ^= board.m_board_state & boardStateToXor & board_state::allCastlingFields_const;

    return(
        isWhiteTurn ? 
        pieceMovement{
            (bKingStart >> 2 | bKingStart) << 56, 
            (bLongRookStart | bLongRookStart << 3) << 56, 
            PieceType::King, PieceType::NotAPiece, 
            PieceType::Rook, PieceType::NotAPiece, 
            static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)),
            board.enPassantState, boardStateToXor
        }
        : 
        pieceMovement{
            bKingStart >> 2 | bKingStart, 
            bLongRookStart | bLongRookStart << 3, 
            PieceType::NotAPiece, PieceType::King, 
            PieceType::NotAPiece, PieceType::Rook, 
            static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)),
            board.enPassantState, boardStateToXor
        }
    );
    
    return std::nullopt;
}


template <PieceType pt>
uint64_t enemyAttackmaskLoop(uint64_t friendly, uint64_t enemies, const chessBoard& board, bool isWhiteTurn) {
    uint64_t pieces = board.pieceToBitboardConst<pt>(!isWhiteTurn);


    if (pt == PieceType::Pawn) {
        if (!isWhiteTurn) {
            return simpleWhitePawnCapture(pieces, ~0ULL);
        } else {
            return simpleBlackPawnCapture(pieces, ~0ULL);
        }
    } else if (pt == PieceType::Knight) {
        return chessMoves::pseudoLegalKnightMoves(pieces, 0ULL);
    } else if (pt ==PieceType::King) {
        return pseudoLegalKingMoves(pieces, 0ULL);
    }

    uint64_t attacks_mushed {0};

    while (pieces) {
        uint64_t piece = pieces & -pieces;
        pieces &= pieces - 1;
        if (pt ==PieceType::Rook) {
            attacks_mushed |= chessMoves::pseudoLegalRookMoves(piece, friendly, enemies);
        } else if (pt == PieceType::Bishop) {
            attacks_mushed |= chessMoves::pseudoLegalBishopMoves(piece, friendly, enemies);
        } else if (pt == PieceType::Queen) {
            attacks_mushed |= chessMoves::pseudoLegalBishopMoves(piece, friendly, enemies);
            attacks_mushed |= chessMoves::pseudoLegalRookMoves(piece, friendly, enemies);
        } 
    }

    return attacks_mushed;
}

template <PieceType pt>
void makeLegalMoves(uint64_t enemies, uint64_t friendly, uint64_t enemy_pawns, uint64_t enemy_attacks_mushed, uint64_t pinsMushed, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, const chessBoard& board, stackStack218& moveStack, bool isWhiteTurn) {
    uint64_t pieces = board.pieceToBitboardConst<pt>(isWhiteTurn);
    while (pieces) {
        uint64_t piece = 1ULL << __builtin_ctzll(pieces);
        pieces &= pieces - 1;
        if (pt == PieceType::Pawn) {
            // pawnMoveReturn pawnRet = chessMoves::singlePawnMove(piece, enemies, friendly,  enemy_pawns, pinLines, checkingAttacks, board, doPinnedPieces);
            pawnMoveReturn pawnRet;
            if (isWhiteTurn) {
                pawnRet = chessMoves::singlePawnMove<true>(piece, enemies, friendly, pinLines, checkingAttacks, board.enPassantState);
            } else {
                pawnRet = chessMoves::singlePawnMove<false>(piece, enemies, friendly, pinLines, checkingAttacks, board.enPassantState);
            }
            uint64_t back_row = isWhiteTurn ? static_cast<uint64_t>(0xff) : static_cast<uint64_t>(0xff) << 56;

            if (pawnRet.normalMovedTo & ~back_row) {
                addAttacksToStack218<PieceType::Pawn>(piece, pawnRet.normalMovedTo, moveStack, isWhiteTurn, board, enemies);
            }

            if (pawnRet.enPassantPawnCapture) {
                if (isWhiteTurn){
                    moveStack.push({piece | pawnRet.enPassantMoveToSquare, pawnRet.enPassantPawnCapture, PieceType::Pawn, PieceType::NotAPiece, 
                            PieceType::NotAPiece, PieceType::Pawn,
                            static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)) , board.enPassantState});
                } else {
                    moveStack.push({piece | pawnRet.enPassantMoveToSquare, pawnRet.enPassantPawnCapture, PieceType::NotAPiece, PieceType::Pawn, PieceType::Pawn, PieceType::NotAPiece, 
                            static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)), board.enPassantState});
                }
            }

            if (pawnRet.normalMovedTo & back_row) {
                uint64_t pawnPromotionSquares = pawnRet.normalMovedTo & back_row;
                
                while (pawnPromotionSquares) {
                    uint64_t pp {1ULL << std::countr_zero(pawnPromotionSquares)};
                    pawnPromotionSquares &= ~pp;
                    
                    PieceType enemyPieceTypeOnPromotionSquare = PieceType::NotAPiece;
                    if (pp & enemies) {
                        enemyPieceTypeOnPromotionSquare = board.figureOutTypeOfPieceOnSquare(pp, !isWhiteTurn);
                    }
                    for (uint8_t j = 1; j < 5; ++j) {
                        pieceMovement promotionMove;
                        if (isWhiteTurn) {
                            promotionMove = pieceMovement{
                                piece, pp, 
                                PieceType::Pawn, PieceType::NotAPiece, 
                                static_cast<PieceType>(j), enemyPieceTypeOnPromotionSquare, 
                                static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)),
                                board.enPassantState
                            };
                        } else {
                            promotionMove = pieceMovement{
                                piece, pp,
                                PieceType::NotAPiece, PieceType::Pawn,
                                enemyPieceTypeOnPromotionSquare, static_cast<PieceType>(j), 
                                static_cast<uint16_t>(board.numPlys ^ (board.numPlys + 1)),
                                board.enPassantState
                            };
                        }
                        // std::println("generated promotion move:");
                        // promotionMove.printThis();
                        moveStack.push(std::move(promotionMove));
                    }
                }
            }
        } else if (pt ==PieceType::Rook) {
            // uint64_t normalRet = chessMoves::singleRookMove(piece, board, pinLines, checkingAttacks, enemies, friendly, doPinnedPieces);
            uint64_t normalRet = chessMoves::singleRookMove(piece, board, pinLines, checkingAttacks, enemies, friendly);
            addAttacksToStack218<pt>(piece, normalRet, moveStack, isWhiteTurn, board, enemies);
        } else if (pt == PieceType::Knight) {
            uint64_t normalRet = chessMoves::singleKnightMove(piece, board, pinLines, checkingAttacks, friendly);
            addAttacksToStack218<pt>(piece, normalRet, moveStack, isWhiteTurn, board, enemies);
        } else if (pt == PieceType::Bishop) {
            uint64_t normalRet = chessMoves::singleBishopMove(piece, board, pinLines, checkingAttacks, enemies, friendly);
            addAttacksToStack218<pt>(piece, normalRet, moveStack, isWhiteTurn, board, enemies);
        } else if (pt == PieceType::Queen) {
            uint64_t normalRet = chessMoves::singleQueenMove(piece, board, pinLines, checkingAttacks, enemies, friendly);
            addAttacksToStack218<pt>(piece, normalRet, moveStack, isWhiteTurn, board, enemies);
        } else if (pt == PieceType::King) {
            uint64_t normalRet = chessMoves::singleKingMove(piece, board, enemy_attacks_mushed, enemies, friendly, moveStack);
            addAttacksToStack218<pt>(piece, normalRet, moveStack, isWhiteTurn, board, enemies);

            uint64_t all_pieces = enemies | friendly;

            auto cas = kingCastlingLong(piece, board, isWhiteTurn, all_pieces, enemy_attacks_mushed);
            if (cas.has_value())
                moveStack.push(std::move(*cas));

            cas = kingCastlingShort(piece, board, isWhiteTurn, all_pieces, enemy_attacks_mushed);
            if (cas.has_value())
                moveStack.push(std::move(*cas));
        }

    }
}

stackStack218 makeAllMoves(const chessBoard& board) {
    // Timer<Timers::MakeAllMoves> t {};
    static stackStack218 moveStack {};
    moveStack.currentNumberItems = 0;



    bool isWhiteTurn = (board_state::WhiteTurn & board.m_board_state) != 0;
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();
    uint64_t enemies = !isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t enemyPawns = board.pieceToBitboardConst<PieceType::Pawn>(!isWhiteTurn);
    uint64_t ourKing =    board.pieceToBitboardConst<PieceType::King>(isWhiteTurn);

    uint64_t friendlyInner = friendly & ~ourKing;

    uint64_t enemy_attacks_mushed = 
        enemyAttackmaskLoop<PieceType::Pawn>  (friendlyInner, enemies, board, isWhiteTurn)   |
        enemyAttackmaskLoop<PieceType::Rook>  (friendlyInner, enemies, board, isWhiteTurn)   |
        enemyAttackmaskLoop<PieceType::Knight>(friendlyInner, enemies, board, isWhiteTurn) |
        enemyAttackmaskLoop<PieceType::Bishop>(friendlyInner, enemies, board, isWhiteTurn) |
        enemyAttackmaskLoop<PieceType::Queen> (friendlyInner, enemies, board, isWhiteTurn)  |
        enemyAttackmaskLoop<PieceType::King>  (friendlyInner, enemies, board, isWhiteTurn);


    FastStack<uint64_t, 13> pinLines {};
    uint64_t pinsMushed {chessMoves::calculatePinLineMasks(board, pinLines, enemies, friendly, isWhiteTurn)};

    FastStack<uint64_t, 2> checkingAttacks{};
    chessMoves::calculateCheckMasks(isWhiteTurn, board, enemies, friendly, checkingAttacks, enemyPawns);

    makeLegalMoves<PieceType::Pawn>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);
    makeLegalMoves<PieceType::Rook>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);
    makeLegalMoves<PieceType::Knight>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);
    makeLegalMoves<PieceType::Bishop>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);
    makeLegalMoves<PieceType::Queen>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);
    makeLegalMoves<PieceType::King>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);

    return moveStack;
}

movegenEngineData makeAllMovesWithDataReturn(const chessBoard& board, stackStack218& moveStack) {
    // Timer<Timers::MakeAllMoves> t {};
    moveStack.currentNumberItems = 0;

    // very cool check detection idea 
    // basically at the top level (here) store the squres where if a friendly piece was moved to it would put the enemy king 
    // in check

    bool isWhiteTurn = (board_state::WhiteTurn & board.m_board_state) != 0;
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();
    uint64_t enemies = !isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t enemyPawns = board.pieceToBitboardConst<PieceType::Pawn>(!isWhiteTurn);
    uint64_t ourKing =    board.pieceToBitboardConst<PieceType::King>(isWhiteTurn);

    uint64_t friendlyInner = friendly & ~ourKing; // if we used friendly we would be able to move our 
                                                  // king backwards when checked by a sliding piece, this would 
                                                  // be illegal

    uint64_t enemy_attacks_mushed = 
        enemyAttackmaskLoop<PieceType::Pawn>  (friendlyInner, enemies, board, isWhiteTurn)   |
        enemyAttackmaskLoop<PieceType::Rook>  (friendlyInner, enemies, board, isWhiteTurn)   |
        enemyAttackmaskLoop<PieceType::Knight>(friendlyInner, enemies, board, isWhiteTurn)   |
        enemyAttackmaskLoop<PieceType::Bishop>(friendlyInner, enemies, board, isWhiteTurn)   |
        enemyAttackmaskLoop<PieceType::Queen> (friendlyInner, enemies, board, isWhiteTurn)   |
        enemyAttackmaskLoop<PieceType::King>  (friendlyInner, enemies, board, isWhiteTurn);


    FastStack<uint64_t, 13> pinLines {};
    uint64_t pinsMushed {chessMoves::calculatePinLineMasks(board, pinLines, enemies, friendly, isWhiteTurn)};

    FastStack<uint64_t, 2> checkingAttacks{};
    chessMoves::calculateCheckMasks(isWhiteTurn, board, enemies, friendly, checkingAttacks, enemyPawns);

    makeLegalMoves<PieceType::Pawn>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);
    makeLegalMoves<PieceType::Rook>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);
    makeLegalMoves<PieceType::Knight>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);
    makeLegalMoves<PieceType::Bishop>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);
    makeLegalMoves<PieceType::Queen>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);
    makeLegalMoves<PieceType::King>(enemies, friendly, enemyPawns, enemy_attacks_mushed, pinsMushed, pinLines, checkingAttacks, board, moveStack, isWhiteTurn);

    return {enemy_attacks_mushed};
};

// when a sequence of uci moves are entered, this function applies moves until it either reaches the end 
// or it reaches a move which is syntactically invalid or doesnt exist in the current board state 
// in the case of all ok it returns true, otherwise false
// if it can only apply some of the moves in the sequence the board will still be changed by the valid moves 
// in the sequence
bool makeMovesFromUciSequence(chessBoard& board, std::string_view uciSeq) {
    std::pair<std::string_view, std::optional<std::string_view>> splitResult = helpers::splitWord(uciSeq);
    auto makeAndApplyMove = [&](std::string_view moveWord){
        auto maybeMove = board.genPartialMove(moveWord).and_then(
            [&](auto mv){ 
                auto allMoves = makeAllMoves(board);
                return searchAndSelectMove(allMoves, mv);
        });

        if (!maybeMove.has_value()) {
            return false;
        }

        board.applyMoveImpure(maybeMove.value());
        return true;
    };

    while (splitResult.second.has_value()) {
        if (!makeAndApplyMove(splitResult.first))
            return false;

        splitResult = helpers::splitWord(splitResult.second.value());
    }

    if (!makeAndApplyMove(splitResult.first))
        return false;

    return true;
}
};

