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
#include <stdexcept>


struct pawnMoveReturn {
    uint64_t normalMovedTo{0};
    uint64_t enPassantPawnCapture {0};
    uint64_t enPassantMoveToSquare {0};
};



struct kingMoveReturn {
    uint64_t normalMoves;
    std::optional<pieceMovement> castlingRight;
    std::optional<pieceMovement> castlingLeft;
    
    void printThis() const {
        std::println("kingMoveReturn{{");

        std::println("normalMoves:");
        helpers::printBitboard(normalMoves);
        std::println(",");

        std::println("castlingRight:");
        if (castlingRight.has_value()) {
            castlingRight->printThis();
        } else {
            std::println("std::nullopt");
        }
        std::println(",");

        std::println("castlingLeft:");
        if (castlingLeft.has_value()) {
            castlingLeft->printThis();
        } else {
            std::println("std::nullopt");
        }

        std::println("}}");
    }
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

static constexpr int how_the_king_moves[8][2] = {
    { 1,  1},
    { 1, -1},
    {-1,  1},
    {-1, -1},
    { 1,  0},
    {-1,  0},
    { 0,  1},
    { 0, -1}
};

static constexpr int how_the_queen_moves[8][2] = {
    { 1,  1},
    { 1, -1},
    {-1,  1},
    {-1, -1},
    { 1,  0},
    {-1,  0},
    { 0,  1},
    { 0, -1}
};


// how does the knight move? 
static constexpr int how_the_knight_moves[8][2] = {
    {2, 1},
    {2, -1},
    {-2, 1},
    {-2, -1},
    {1, 2},
    {1, -2},
    {-1, 2},
    {-1, -2}
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


template <bool teleportCheck>
inline uint64_t simpleWhitePawnCapture(uint64_t white_pawn, uint64_t occupied) {
    if (!teleportCheck)
        return ((white_pawn >> 7) & occupied) | ((white_pawn >> 9) & occupied);

    uint64_t haveRbs {0x1010101010100};
    uint64_t haveLbs {0x80808080808000};
    haveRbs = ~haveRbs;
    haveLbs = ~haveLbs;

    return (((white_pawn & haveLbs) >> 7) & occupied) |  (((white_pawn & haveRbs) >> 9) & occupied);
}

template <bool teleportCheck>
inline uint64_t simpleBlackPawnCapture(uint64_t black_pawn, uint64_t occupied) {
    if (!teleportCheck)
        return ((black_pawn << 7) & occupied) | ((black_pawn << 9) & occupied);

    uint64_t haveRbs {0x1010101010100};
    uint64_t haveLbs {0x80808080808000};
    haveRbs = ~haveRbs;
    haveLbs = ~haveLbs;
    
    return (((black_pawn & haveRbs)  << 7) & occupied) | (((black_pawn & haveLbs)  << 9) & occupied);
}

template <bool isWhiteTurn>
void
normalPawnMove(uint64_t pawn, uint64_t enemies, uint64_t friendly, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& enemyCheckingAttacks, uint64_t enemyPawns, pawnMoveReturn& returnVal)
{
    uint64_t empty_space = ~(enemies | friendly);
    uint64_t white_front_pawn_row = 0xff000000000000;
    uint64_t black_front_pawn_row = 0xff00;

    uint64_t front_pawn_row = isWhiteTurn ? white_front_pawn_row : black_front_pawn_row;

    returnVal.normalMovedTo = (isWhiteTurn ? pawn >> 8 : pawn << 8) & empty_space;

    returnVal.normalMovedTo |= (pawn & front_pawn_row ? (isWhiteTurn ? returnVal.normalMovedTo >> 8 : returnVal.normalMovedTo << 8) : 0) & empty_space;

returnVal.normalMovedTo |= isWhiteTurn ? simpleWhitePawnCapture<true>(pawn, enemies) : simpleBlackPawnCapture<true>(pawn, enemies);

    if (pinLines.notEmpty())
        returnVal.normalMovedTo = applyPinsToPiece(pawn, returnVal.normalMovedTo, pinLines);

    if (enemyCheckingAttacks.notEmpty())
        returnVal.normalMovedTo = applyChecksToPiece(pawn, returnVal.normalMovedTo, enemyCheckingAttacks);
}


template <bool isWhiteTurn>
void pawnMoveEPP(uint64_t pawn, uint64_t enemies, uint64_t friendly, uint64_t enemy_pawns, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& enemyCheckingAttacks, uint8_t boardEPPState, pawnMoveReturn& returnStruct) {
    assert(boardEPPState != -1);
    // std::println("epp state of board, {}", board.enPassantState);

    uint64_t eppCaptureSquare = 1ULL << boardEPPState;

    // helpers::printBitboard(eppCaptureSquare);

    uint64_t locationMovedTo = isWhiteTurn ? simpleWhitePawnCapture<true>(pawn, eppCaptureSquare) : simpleBlackPawnCapture<true>(pawn, eppCaptureSquare);

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



pawnMoveReturn
inline singlePawnMove(uint64_t attacking_pawn, uint64_t enemies, uint64_t friendly,  uint64_t enemy_pawns, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, const chessBoard& board) {
    assert(board.enPassantState<= 63);
    bool hasEnPassant = board.enPassantState >= 0;
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;

    pawnMoveReturn retval;

    isWhiteTurn ? 
          normalPawnMove<true>(attacking_pawn, enemies, friendly, pinLines, checkingAttacks, enemy_pawns, retval)
        : normalPawnMove<false>(attacking_pawn, enemies, friendly, pinLines, checkingAttacks, enemy_pawns, retval);

    if (hasEnPassant) 
    {
        (isWhiteTurn ? 
          pawnMoveEPP<true>(attacking_pawn, enemies, friendly,  enemy_pawns, pinLines, checkingAttacks, board.enPassantState, retval)
        : pawnMoveEPP<false>(attacking_pawn, enemies, friendly,  enemy_pawns, pinLines, checkingAttacks, board.enPassantState, retval));
    }

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

inline uint64_t pseudoLegalRookMoves(uint64_t rook, uint64_t enemies, uint64_t friendly) {
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

inline uint64_t pseudoLegalBishopMoves(uint64_t bishop, uint64_t enemies, uint64_t friendly) {
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

inline uint64_t calculatePinLineMasks(const chessBoard& board, FastStack<uint64_t, 13>& pin_lines, uint64_t enemies, uint64_t friendly, bool isWhiteTurn) {

    uint64_t pinsMushed {0};
    uint64_t friendly_king = isWhiteTurn ? board.bitboards[PieceType::King + 6] : board.bitboards[PieceType::King];


    uint64_t kingScanRayRookLike = pseudoLegalRookMoves(friendly_king, enemies, 0);
    uint64_t enemyRooks  {board.pieceToBitboardConst<PieceType::Rook>(!isWhiteTurn)};
    uint64_t enemyQueens {board.pieceToBitboardConst<PieceType::Queen>(!isWhiteTurn)};
    
    if (kingScanRayRookLike & enemyRooks) {
        uint64_t seenRooks {kingScanRayRookLike & enemyRooks};
        while (seenRooks) {
            uint64_t piece = seenRooks & -seenRooks;
            seenRooks &= seenRooks -1;

            uint64_t rookRayIncRook {pseudoLegalRookMoves(piece, friendly_king, enemies) | piece};
            uint64_t candiatePin {kingScanRayRookLike & rookRayIncRook};

            if (std::popcount(candiatePin & friendly) == 1) {
                pin_lines.push(candiatePin);
                pinsMushed |= candiatePin;
            }
        }
    }

    if (kingScanRayRookLike & enemyQueens) {
        uint64_t seenQueens{kingScanRayRookLike & enemyQueens};
        while (seenQueens) {
            uint64_t piece = seenQueens & -seenQueens;
            seenQueens &= seenQueens-1;

            uint64_t queenRayRookLikeIncQueen {pseudoLegalRookMoves(piece, friendly_king, enemies) | piece};
            uint64_t candiatePin {kingScanRayRookLike & queenRayRookLikeIncQueen};

            if (std::popcount(candiatePin & friendly) == 1) {
                pin_lines.push(candiatePin);
                pinsMushed |= candiatePin;
            }
        }
    }

    uint64_t kingScanRayBishopLike = pseudoLegalBishopMoves(friendly_king, enemies, 0);
    uint64_t enemyBishops {board.pieceToBitboardConst<PieceType::Bishop>(!isWhiteTurn)};

    if (kingScanRayBishopLike & enemyBishops) {
        uint64_t seenBishops {kingScanRayBishopLike & enemyBishops};
        while (seenBishops) {
            uint64_t piece = seenBishops & -seenBishops;
            seenBishops &= seenBishops -1;

            uint64_t bishopRayIncBishop = pseudoLegalBishopMoves(piece, friendly_king, enemies) | piece;
            uint64_t candiatePin = kingScanRayBishopLike & bishopRayIncBishop;

            if (std::popcount(candiatePin & friendly) == 1) {
                pin_lines.push(candiatePin);
                pinsMushed |= candiatePin;
            }
        }
    }

    if (kingScanRayBishopLike & enemyQueens) {
        uint64_t seenQueens {kingScanRayBishopLike & enemyQueens};
        while (seenQueens) {
            uint64_t piece = seenQueens & -seenQueens;
            seenQueens &= seenQueens-1;

            uint64_t queenRayBishopLikeIncQueen = pseudoLegalBishopMoves(piece, friendly_king, enemies) | piece;
            uint64_t candiatePin = kingScanRayBishopLike & queenRayBishopLikeIncQueen;

            if (std::popcount(candiatePin & friendly) == 1) {
                pin_lines.push(candiatePin);
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

template<bool teleportCheck>
inline uint64_t pseudoLegalKnightMoves(uint64_t knight, uint64_t friendly) {
    int knight_place;
    int knight_rank;
    int knight_file;

    if (teleportCheck) {

        knight_place= __builtin_ctzll(knight);
        knight_rank = (knight_place) / 8 + 1;
        knight_file = (knight_place) % 8 + 1;
    }

    uint64_t attacked_squares = 0ULL;

    for (const auto& d : how_the_knight_moves) {
        if (teleportCheck) {
            if (knight_rank+ d[0] <= 0 || knight_rank+ d[0] > 8 || knight_file+ d[1] <= 0 || knight_file+ d[1] > 8) {
                continue;
            }
        }
        int shiftDist = d[0] * 8 + d[1];
        uint64_t attacked_square = shiftDist >= 0 ? knight << shiftDist : knight >> (-shiftDist);
        attacked_squares |= attacked_square;
    }

    return attacked_squares;
}

// evil and intimidating horse
inline uint64_t singleKnightMove(uint64_t knight, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, uint64_t friendly) {
    // std::println("generating knight move, checks here : ");
    // for (auto ch : checkingAttacks) {
    //     helpers::printBitboard(ch);
    // }
    uint64_t attacked_squares;
    uint64_t center = 0x3c3c3c3c0000;
    if (knight & center)
        attacked_squares = pseudoLegalKnightMoves<false>(knight, friendly);
    else
        attacked_squares = pseudoLegalKnightMoves<true>(knight, friendly);

    attacked_squares &= (~friendly);
    if (pinLines.notEmpty())
        attacked_squares = applyPinsToPiece(knight, attacked_squares, pinLines);
    if (checkingAttacks.notEmpty())
        attacked_squares = applyChecksToPiece(knight, attacked_squares, checkingAttacks);

    return attacked_squares;
}

// we can pass in the sliding piece attacks since we now know this function will only be ran once per turn due to simply not generating moves that end with us in check
// passing in the sliding piece attacks saves on computation, limiting the number of sliding piece calculations we need to perform
inline void calculateCheckMasks(bool isWhiteTheColorBeingChecked, const chessBoard& board, uint64_t enemies, uint64_t friendly, FastStack<uint64_t, 2>& checks, uint64_t enemyPawns) {
    // Timer<Timers::ComputeCheckMasks> t{};
   
    uint64_t king_possibly_checked = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::King + 6] : board.bitboards[PieceType::King];

    uint64_t kingSeesLikePawn = isWhiteTheColorBeingChecked ? simpleWhitePawnCapture<true>(king_possibly_checked, enemyPawns) : simpleBlackPawnCapture<true>(king_possibly_checked, enemyPawns); 
    uint64_t kingScanRayBishopLike = pseudoLegalBishopMoves(king_possibly_checked, enemies, friendly);
    uint64_t kingScanRayRookLike = pseudoLegalRookMoves(king_possibly_checked, enemies, friendly);

    // uint64_t center = 0x3c3c3c3c0000;
    uint64_t kingSeesLikeKnight = pseudoLegalKnightMoves<true>(king_possibly_checked, friendly);
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

template <bool teleportationCheck>
inline uint64_t pseudoLegalKingMoves(uint64_t king, uint64_t friendly) {
    int king_place;
    int king_rank ;
    int king_file ;

    if (teleportationCheck) {
        king_place = __builtin_ctzll(king);
        king_rank = (king_place) / 8 + 1;
        king_file = (king_place) % 8 + 1;
    }

    uint64_t attacked_squares = 0;

    for (const auto &d : how_the_king_moves) {
        // prevent teleportation
        if (teleportationCheck && (king_rank + d[0] <= 0 || king_rank + d[0] > 8 || king_file + d[1] <= 0 || king_file + d[1] > 8)) {
            continue;
        }

        int shiftDist = d[0] * 8 + d[1];
        uint64_t attacked_square = shiftDist >= 0 ? king << shiftDist : king >> (-shiftDist);
        attacked_squares |= attacked_square; 
    }
    return attacked_squares;
};


inline kingMoveReturn singleKingMove(uint64_t king, const chessBoard& board, uint64_t attackMask, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::KingMoveFunction> t{};
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    kingMoveReturn retVal;

    uint64_t center = 0x7e7e7e7e7e7e00;
    if (king & center)
        retVal.normalMoves = pseudoLegalKingMoves<false>(king, friendly);
    else
        retVal.normalMoves = pseudoLegalKingMoves<true>(king, friendly);

    // helpers::printBitboard(retVal.normalMoves);
    retVal.normalMoves &= (~friendly) & (~attackMask);

    uint64_t all_pieces = enemies | friendly;

    if (!(board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsLeft : board_state::BlackLostCastlingRightsLeft))) {

        uint64_t bKingStart = 0x10, bLongRookStart = 0x1, bSquareNotAttacked= 0xc, bSquaresEmpty = 0xe;

        if ( !((isWhiteTurn ? bSquaresEmpty << 56 : bSquaresEmpty) & (all_pieces)) && 
             !((isWhiteTurn ? bSquareNotAttacked << 56 : bSquareNotAttacked) & (attackMask)))
        {

            uint8_t boardStateToXor = isWhiteTurn ? 
                    board_state::WhiteTurn | board_state::WhiteLostCastlingRightsRight | board_state::WhiteLostCastlingRightsLeft : 
                    board_state::WhiteTurn | board_state::BlacklostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft;

            boardStateToXor ^= board.m_board_state & boardStateToXor & board_state::allCastlingFields_const;

            retVal.castlingLeft = isWhiteTurn ? 
                pieceMovement{
                    (bKingStart >> 2 | bKingStart) << 56, 
                    (bLongRookStart | bLongRookStart << 3) << 56, 
                    PieceType::King, PieceType::NotAPiece, 
                    PieceType::Rook, PieceType::NotAPiece, 
                    board.enPassantState, boardStateToXor
                } : 
                pieceMovement{
                    bKingStart >> 2 | bKingStart, 
                    bLongRookStart | bLongRookStart << 3, 
                    PieceType::NotAPiece, PieceType::King, 
                    PieceType::NotAPiece, PieceType::Rook, 
                    board.enPassantState, boardStateToXor
                };
        }
    }

    // can castle right (board state)
    if (!(board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsRight : board_state::BlacklostCastlingRightsRight))) {
        uint64_t bKingStart = 0x10, bSquaresEmptyAndNotAttacked = 0x60, bShortRookStart = 0x80;

        // check the squares have what they should, redundant by design with the above check
        if(!((isWhiteTurn ? bSquaresEmptyAndNotAttacked << 56 : bSquaresEmptyAndNotAttacked) & (all_pieces | attackMask))) 
        {
            uint8_t boardStateToXor = isWhiteTurn ? 
                    board_state::WhiteTurn | board_state::WhiteLostCastlingRightsRight | board_state::WhiteLostCastlingRightsLeft : 
                    board_state::WhiteTurn | board_state::BlacklostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft;

            boardStateToXor ^= board.m_board_state & boardStateToXor & board_state::allCastlingFields_const;

            retVal.castlingRight = isWhiteTurn ?
                pieceMovement{
                    (bKingStart << 2 | bKingStart) << 56, 
                    (bShortRookStart | bShortRookStart >> 2) << 56, 
                    PieceType::King, PieceType::NotAPiece,
                    PieceType::Rook, PieceType::NotAPiece, 
                    board.enPassantState, boardStateToXor
                } :
                pieceMovement{
                    bKingStart << 2 | bKingStart,
                    bShortRookStart | bShortRookStart >> 2,
                    PieceType::NotAPiece, PieceType::King, 
                    PieceType::NotAPiece, PieceType::Rook, 
                    board.enPassantState, boardStateToXor
                };
        }
    }
    

    return retVal;
}

template <PieceType pt>
uint64_t enemyAttackmaskLoop(uint64_t friendly, uint64_t enemies, const chessBoard& board, bool isWhiteTurn) {
    uint64_t pieces = board.pieceToBitboardConst<pt>(!isWhiteTurn);

    uint64_t attacks_mushed {0};

    if (pt == PieceType::Pawn) {
        uint64_t sideOfBoard = 0x81818181818100;
        attacks_mushed |= !isWhiteTurn ? chessMoves::simpleWhitePawnCapture<false>(pieces & ~sideOfBoard, ~0ULL) : chessMoves::simpleBlackPawnCapture<false>(pieces & ~sideOfBoard, ~0ULL);
        pieces &= sideOfBoard;
    } else if (pt == PieceType::Knight) {
        uint64_t edgeOfBoard = 0xffffc3c3c3c3ffff;
        attacks_mushed |= chessMoves::pseudoLegalKnightMoves<false>(pieces & ~edgeOfBoard, enemies);;
        pieces &= edgeOfBoard;
    } else if (pt ==PieceType::King) {
        uint64_t boardErrr = 0xff818181818181ff;
        attacks_mushed |= chessMoves::pseudoLegalKingMoves<false>(pieces & ~boardErrr, enemies);
        pieces &= boardErrr;
    }


    while (pieces) {
        uint64_t piece = pieces & -pieces;
        pieces &= pieces - 1;
        if (pt == PieceType::Pawn) {
            attacks_mushed |= !isWhiteTurn ? chessMoves::simpleWhitePawnCapture<true>(piece, ~0ULL) : chessMoves::simpleBlackPawnCapture<true>(piece, ~0ULL);
        } else if (pt ==PieceType::Rook) {
            attacks_mushed |= chessMoves::pseudoLegalRookMoves(piece, friendly, enemies);
        } else if (pt == PieceType::Bishop) {
            attacks_mushed |= chessMoves::pseudoLegalBishopMoves(piece, friendly, enemies);
        } else if (pt == PieceType::Queen) {
            attacks_mushed |= chessMoves::pseudoLegalBishopMoves(piece, friendly, enemies);
            attacks_mushed |= chessMoves::pseudoLegalRookMoves(piece, friendly, enemies);
        } else if (pt == PieceType::Knight) {

            attacks_mushed |= chessMoves::pseudoLegalKnightMoves<true>(piece, enemies);;
        } else if (pt == PieceType::King) {
            attacks_mushed |= chessMoves::pseudoLegalKingMoves<true>(piece, enemies);
        }
    }

    return attacks_mushed;
}

template <PieceType pt>
void makeLegalMoves(uint64_t enemies, uint64_t friendly, uint64_t enemy_pawns, uint64_t enemy_attacks_mushed, uint64_t pinsMushed, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, const chessBoard& board, stackStack218& moveStack, bool isWhiteTurn) {
    uint64_t pieces = board.pieceToBitboardConst<pt>(isWhiteTurn);

    // pieces &= doPinnedPieces ? pinsMushed : ~pinsMushed;

    while (pieces) {
        uint64_t piece = 1ULL << __builtin_ctzll(pieces);
        pieces &= pieces - 1;
        if (pt == PieceType::Pawn) {
            // pawnMoveReturn pawnRet = chessMoves::singlePawnMove(piece, enemies, friendly,  enemy_pawns, pinLines, checkingAttacks, board, doPinnedPieces);
            pawnMoveReturn pawnRet = chessMoves::singlePawnMove(piece, enemies, friendly,  enemy_pawns, pinLines, checkingAttacks, board);
            uint64_t back_row = isWhiteTurn ? static_cast<uint64_t>(0xff) : static_cast<uint64_t>(0xff) << 56;

            if (pawnRet.normalMovedTo & ~back_row) {
                addAttacksToStack218(piece, pawnRet.normalMovedTo, PieceType::Pawn, moveStack, isWhiteTurn, board, enemies);
            }

            if (pawnRet.enPassantPawnCapture) {
                if (isWhiteTurn){
                    moveStack.push({piece | pawnRet.enPassantMoveToSquare, pawnRet.enPassantPawnCapture, PieceType::Pawn, PieceType::NotAPiece, PieceType::NotAPiece, PieceType::Pawn, board.enPassantState});
                } else {
                    moveStack.push({piece | pawnRet.enPassantMoveToSquare, pawnRet.enPassantPawnCapture, PieceType::NotAPiece, PieceType::Pawn, PieceType::Pawn, PieceType::NotAPiece, board.enPassantState});
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
                                static_cast<PieceType>(j), enemyPieceTypeOnPromotionSquare, board.enPassantState
                            };
                        } else {
                            promotionMove = pieceMovement{
                                piece, pp,
                                PieceType::NotAPiece, PieceType::Pawn,
                                enemyPieceTypeOnPromotionSquare, static_cast<PieceType>(j), board.enPassantState
                            };
                        }
                        // std::println("generated promotion move:");
                        // promotionMove.printThis();
                        moveStack.push(promotionMove);
                    }
                }
            }
        } else if (pt ==PieceType::Rook) {
            // uint64_t normalRet = chessMoves::singleRookMove(piece, board, pinLines, checkingAttacks, enemies, friendly, doPinnedPieces);
            uint64_t normalRet = chessMoves::singleRookMove(piece, board, pinLines, checkingAttacks, enemies, friendly);
            addAttacksToStack218(piece, normalRet, pt, moveStack, isWhiteTurn, board, enemies);
        } else if (pt == PieceType::Knight) {
            uint64_t normalRet = chessMoves::singleKnightMove(piece, board, pinLines, checkingAttacks, friendly);
            addAttacksToStack218(piece, normalRet, pt, moveStack, isWhiteTurn, board, enemies);
        } else if (pt == PieceType::Bishop) {
            uint64_t normalRet = chessMoves::singleBishopMove(piece, board, pinLines, checkingAttacks, enemies, friendly);
            addAttacksToStack218(piece, normalRet, pt, moveStack, isWhiteTurn, board, enemies);
        } else if (pt == PieceType::Queen) {
            uint64_t normalRet = chessMoves::singleQueenMove(piece, board, pinLines, checkingAttacks, enemies, friendly);
            addAttacksToStack218(piece, normalRet, pt, moveStack, isWhiteTurn, board, enemies);

        } else if (pt == PieceType::King) {
            kingMoveReturn kingRet = chessMoves::singleKingMove(piece, board, enemy_attacks_mushed, enemies, friendly);
            addAttacksToStack218(piece, kingRet.normalMoves, pt, moveStack, isWhiteTurn, board, enemies);

            if(kingRet.castlingRight.has_value()) {
                moveStack.push(kingRet.castlingRight.value());
            }

            if(kingRet.castlingLeft.has_value()) {
                moveStack.push(kingRet.castlingLeft.value());
            }
        }
    }
}

const stackStack218& makeAllMoves(const chessBoard& boardInput) {
    // Timer<Timers::MakeAllMoves> t {};
    static stackStack218 moveStack {};
    moveStack.currentNumberItems = 0;

    chessBoard board {boardInput}; // interesting copying the board seems to increase the cache hit rate and increase performance


    bool isWhiteTurn = (board_state::WhiteTurn & board.m_board_state) != 0;
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();
    uint64_t enemies = !isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t enemyPawns = board.pieceToBitboardConst<PieceType::Pawn>(!isWhiteTurn);
    uint64_t ourKing =    board.pieceToBitboardConst<PieceType::King>(isWhiteTurn);

    uint64_t friendlyInner = friendly & ~ourKing;

    ;

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
}

