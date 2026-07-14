#include "pieceMovements.hpp"
#include "chessBoard.h"
#include "helpers.hpp"
#include "timer.hpp"
#include <bit>
#include <cassert>
#include <cstdint>
#include <optional>
#include <print>
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


inline uint64_t simpleWhitePawnCapture(uint64_t white_pawn, uint64_t occupied) {
    int pawn_place = __builtin_ctzll(white_pawn);
    int pawn_file = pawn_place % 8;

    bool can_have_right_bitshift_by_1 = pawn_file != 0;
    bool can_have_left_bitshift_by_1 = pawn_file != 7;

    return (can_have_left_bitshift_by_1 ? ((white_pawn >> 7) & occupied) : 0ULL) | (can_have_right_bitshift_by_1 ? ((white_pawn >> 9) & occupied) : 0ULL);
}

inline uint64_t simpleBlackPawnCapture(uint64_t black_pawn, uint64_t occupied) {
    int pawn_place = __builtin_ctzll(black_pawn);
    int pawn_file = pawn_place % 8;

    bool can_have_right_bitshift_by_1 = pawn_file != 0;
    bool can_have_left_bitshift_by_1 = pawn_file != 7;
    
    return (can_have_right_bitshift_by_1 ? ((black_pawn  << 7) & occupied) : 0ULL) | (can_have_left_bitshift_by_1 ? ((black_pawn  << 9) & occupied) : 0ULL);
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

    returnVal.normalMovedTo |= isWhiteTurn ? simpleWhitePawnCapture(pawn, enemies) : simpleBlackPawnCapture(pawn, enemies);

    returnVal.normalMovedTo = applyPinsToPiece(pawn, returnVal.normalMovedTo, pinLines);
    returnVal.normalMovedTo = applyChecksToPiece(pawn, returnVal.normalMovedTo, enemyCheckingAttacks);

}


template <bool isWhiteTurn>
void pawnMoveEPP(uint64_t pawn, uint64_t enemies, uint64_t friendly, uint64_t enemy_pawns, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& enemyCheckingAttacks, uint8_t boardEPPState, pawnMoveReturn& returnStruct) {
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

    locationMovedTo = applyPinsToPiece(pawn, locationMovedTo, pinLines);

    pawnWeCapture = applyChecksToPiece(pawn, pawnWeCapture, enemyCheckingAttacks);

    if (locationMovedTo == 0 || pawnWeCapture == 0) {
        return;
    }

    returnStruct.enPassantMoveToSquare = locationMovedTo;
    returnStruct.enPassantPawnCapture = pawnWeCapture;
}

uint64_t scanPinRay(uint64_t pieces_of_the_same_color_as_the_attacker, uint64_t opposite_pieces, uint64_t opposite_king, int r, int f, int df, int dr) {
    uint64_t trial_attack = 1ULL << (f + 8 * r); // this line needs some explanation, the piece isnt actually attacking this square but it is the square that the pinning piece
                                                 // sits in, therefore when we apply the pin by anding the pinned pieces attack and the pin we must allow the pinned piece to capture the pinner 
    
    bool hit_enemy_piece = false; // we cant have a pin if there are 2 pieces inbetween our attacking piece and the enemy king
    r += dr;
    f += df;
    while (r >= 0 && r < 8 && f >= 0 && f < 8) {
        uint64_t looking_at = 1ULL << (f + 8 * r);
        if (looking_at & opposite_king) {
            // if its an open line to the king then this is a check not a pin
            if (!hit_enemy_piece) {
                return 0;
            }
            return trial_attack;
        } else if (looking_at & pieces_of_the_same_color_as_the_attacker) {
            return 0;
        } else if (looking_at & opposite_pieces) {
            if (hit_enemy_piece) {
                return 0;
            }
            trial_attack |= looking_at;
            hit_enemy_piece = true;
        } else {
            trial_attack |= looking_at;
        }
        r += dr;
        f += df;
    }
    return 0;
}

template <PieceType pt>
requires SlidingPiece<pt>
uint64_t output_pinned_squares_for_piece(uint64_t pinning_piece, uint64_t same_color_pieces, uint64_t opposite_pieces, uint64_t opposite_king) {
    int piece_place = __builtin_ctzll(pinning_piece);
    int piece_rank = piece_place / 8;
    int piece_file = piece_place % 8;

    uint64_t pinned_squares = 0;

    if (pt == PieceType::Bishop) {
        for (auto &mov : how_the_bishop_moves) {
            pinned_squares |= scanPinRay(same_color_pieces, opposite_pieces, opposite_king, piece_rank, piece_file, mov[0], mov[1]);
        }
    } else if (pt == PieceType::Rook) {
        for (auto &mov : how_the_rook_moves) {
            pinned_squares |= scanPinRay(same_color_pieces, opposite_pieces, opposite_king, piece_rank, piece_file, mov[0], mov[1]);
        }
    } else if (pt == PieceType::Queen) {
        for (auto &mov : how_the_queen_moves) {
            pinned_squares |= scanPinRay(same_color_pieces, opposite_pieces, opposite_king, piece_rank, piece_file, mov[0], mov[1]);
        }
    } 

    return pinned_squares;
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

uint64_t attack_with_increment_sliding_piece(uint64_t enemies, uint64_t friendly, std::pair<int, int> starting_rf, std::pair<int, int> increment_rf) {
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

void calculatePinLineMasks(const chessBoard& board, FastStack<uint64_t, 13>& pin_lines, uint64_t enemies, uint64_t friendly, bool isWhiteTurn) {
    // Timer<Timers::ComputePinMasks> t{};

    // uint64_t friendly_king = isWhiteTurn ? board.m_white_king : board.m_black_king;
    //
    // uint64_t enemy_rooks = !isWhiteTurn ? board.m_white_rooks : board.m_black_rooks;
    // uint64_t enemy_bishops = !isWhiteTurn ? board.m_white_bishops : board.m_black_bishops;
    // uint64_t enemy_queens = !isWhiteTurn ? board.m_white_queens : board.m_black_queens;

    uint64_t friendly_king = isWhiteTurn ? board.bitboards[PieceType::King + 6] : board.bitboards[PieceType::King];

    uint64_t enemy_rooks = !isWhiteTurn ? board.bitboards[PieceType::Rook + 6] : board.bitboards[PieceType::Rook];
    uint64_t enemy_bishops = !isWhiteTurn ? board.bitboards[PieceType::Bishop + 6] : board.bitboards[PieceType::Bishop];
    uint64_t enemy_queens = !isWhiteTurn ? board.bitboards[PieceType::Queen + 6] : board.bitboards[PieceType::Queen];

    while (enemy_rooks) {
        uint64_t rook{1ULL << std::countr_zero(enemy_rooks)};
        enemy_rooks &= ~rook;
        auto pin = output_pinned_squares_for_piece<PieceType::Rook>(rook, enemies, friendly, friendly_king);
        if (pin != 0) {
            pin_lines.push(pin);
        }
    }

    while (enemy_bishops) {
        uint64_t bishop {1ULL << std::countr_zero(enemy_bishops)};
        enemy_bishops &= ~bishop;
        auto pin = output_pinned_squares_for_piece<PieceType::Bishop>(bishop, enemies, friendly, friendly_king);
        if (pin != 0) {
            pin_lines.push(pin);
        }
    }

    while (enemy_queens) {
        uint64_t queen {1ULL << std::countr_zero(enemy_queens)};
        enemy_queens &= ~queen;
        auto pin = output_pinned_squares_for_piece<PieceType::Queen>(queen, enemies, friendly, friendly_king);
        if (pin != 0) {
            pin_lines.push(pin);
        }
    }
}

inline uint64_t pseudoLegalRookMoves(uint64_t rook, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::AttackCreationRook> t{};
    int rook_place = __builtin_ctzll(rook);

    int rook_rank = rook_place / 8;
    int rook_file = rook_place % 8;

    uint64_t attacked_squares = 0;

    for (const auto &d : how_the_rook_moves) {
        attacked_squares |= attack_with_increment_sliding_piece(
            enemies, friendly,
            {rook_rank, rook_file},
            {d[0], d[1]}
        );
    }

    return attacked_squares;
}

inline uint64_t singleRookMove(uint64_t rook, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::SlidingAttackRook> t{};

    uint64_t attacked_squares = pseudoLegalRookMoves(rook, enemies, friendly);

    attacked_squares ^= (attacked_squares & friendly); // un attacks the friendly pieces
    attacked_squares = applyPinsToPiece(rook, attacked_squares, pinLines);
    attacked_squares = applyChecksToPiece(rook, attacked_squares, checkingAttacks);

    return attacked_squares;
}

inline uint64_t pseudoLegalBishopMoves(uint64_t bishop, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::AttackCreationBishop> t{};
    int bishop_place = __builtin_ctzll(bishop);

    int bishop_rank = bishop_place / 8;
    int bishop_file = bishop_place % 8;

    uint64_t attacked_squares = 0;

    for (auto &d : how_the_bishop_moves) {
        attacked_squares |= attack_with_increment_sliding_piece(
            enemies, friendly,
            {bishop_rank, bishop_file},
            {d[0], d[1]}
        );
    }
    return attacked_squares;
}

inline uint64_t singleBishopMove(uint64_t bishop, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::SlidingAttackBishop> t{};
    uint64_t attacked_squares = pseudoLegalBishopMoves(bishop, enemies, friendly);

    attacked_squares ^= (attacked_squares & friendly); // un attacks the friendly pieces
    attacked_squares = applyPinsToPiece(bishop, attacked_squares, pinLines);
    attacked_squares = applyChecksToPiece(bishop, attacked_squares, checkingAttacks);

    return attacked_squares;
}

inline uint64_t singleQueenMove(uint64_t queen, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::QueenAttack> t{};
    uint64_t attacked_squares = pseudoLegalRookMoves(queen, enemies, friendly) | pseudoLegalBishopMoves(queen, enemies, friendly);

    attacked_squares &= (~friendly); // un attacks the friendly pieces
    attacked_squares = applyPinsToPiece(queen, attacked_squares, pinLines);
    attacked_squares = applyChecksToPiece(queen, attacked_squares, checkingAttacks);

    return attacked_squares;
}

inline uint64_t pseudoLegalKnightMoves(uint64_t knight, uint64_t friendly) {
    int knight_place= __builtin_ctzll(knight);
    int knight_rank = (knight_place) / 8 + 1;
    int knight_file = (knight_place) % 8 + 1;

    uint64_t attacked_squares = 0ULL;

    for (const auto& d : how_the_knight_moves) {
        if (knight_rank+ d[0] <= 0 || knight_rank+ d[0] > 8 || knight_file+ d[1] <= 0 || knight_file+ d[1] > 8) {
            continue;
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
    uint64_t attacked_squares = pseudoLegalKnightMoves(knight, friendly);
    attacked_squares &= (~friendly);
    attacked_squares = applyPinsToPiece(knight, attacked_squares, pinLines);
    attacked_squares = applyChecksToPiece(knight, attacked_squares, checkingAttacks);

    return attacked_squares;
}

// we can pass in the sliding piece attacks since we now know this function will only be ran once per turn due to simply not generating moves that end with us in check
// passing in the sliding piece attacks saves on computation, limiting the number of sliding piece calculations we need to perform
inline void calculateCheckMasks(bool isWhiteTheColorBeingChecked, const chessBoard& board, uint64_t enemies, uint64_t friendly, FastStack<uint64_t, 2>& checks) {
    // Timer<Timers::ComputeCheckMasks> t{};
   
    uint64_t king_possibly_checked = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::King + 6] : board.bitboards[PieceType::King];

    uint64_t enemyPawns = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::Pawn] : board.bitboards[PieceType::Pawn + 6];

    uint64_t kingSeesLikePawn = isWhiteTheColorBeingChecked ? simpleWhitePawnCapture(king_possibly_checked, enemyPawns) : simpleBlackPawnCapture(king_possibly_checked, enemyPawns); 
    uint64_t kingScanRayBishopLike = pseudoLegalBishopMoves(king_possibly_checked, enemies, friendly);
    uint64_t kingScanRayRookLike = pseudoLegalRookMoves(king_possibly_checked, enemies, friendly);
    uint64_t kingSeesLikeKnight = pseudoLegalKnightMoves(king_possibly_checked, friendly);





    if (kingSeesLikePawn & enemyPawns) {
        checks.push(kingSeesLikePawn & enemyPawns);
    }

    uint64_t enemyKnights = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::Knight] : board.bitboards[PieceType::Knight + 6];
    if (kingSeesLikeKnight & enemyKnights) {
        // we cant be double checked by 2 knights since this would require a discovery and knights cant be blocked 
        checks.push(kingSeesLikeKnight & enemyKnights);
    }

    // its still check if the enemy piece is pinned, since checkmate is capturing the opponents king even through this capture is never played
    
    uint64_t enemyRooks = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::Rook] : board.bitboards[PieceType::Rook + 6];
    if (kingScanRayRookLike & enemyRooks) {
        assert(std::popcount(kingScanRayRookLike & enemyRooks) == 1);
        uint64_t enemyRookAttack = pseudoLegalRookMoves(kingScanRayRookLike & enemyRooks, friendly, enemies);
        if (enemyRookAttack & king_possibly_checked) {
            checks.push((enemyRookAttack & kingScanRayRookLike) | (kingScanRayRookLike & enemyRooks));
        }
    }

    uint64_t enemyBishops = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::Bishop] : board.bitboards[PieceType::Bishop + 6];
    // same as for the rooks, if we are on a diagonal with a king then we need two moves to check from another diagonal therefore we cant have 2 bishop checks
    if (kingScanRayBishopLike & enemyBishops) {
        assert(std::popcount(kingScanRayBishopLike & enemyBishops) == 1);
        uint64_t enemyBishopAttack = pseudoLegalBishopMoves(kingScanRayBishopLike & enemyBishops, friendly, enemies);
        if (enemyBishopAttack & king_possibly_checked) {
            checks.push((enemyBishopAttack & kingScanRayBishopLike) | (kingScanRayBishopLike & enemyBishops));
        }
    }

    uint64_t kingScanRayQueenLike = kingScanRayBishopLike | kingScanRayRookLike;
    uint64_t enemyQueens = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::Queen] : board.bitboards[PieceType::Queen + 6];
    if (kingScanRayQueenLike & enemyQueens) {
        uint64_t checkingQueens {kingScanRayQueenLike & enemyQueens};
        while (checkingQueens) {
            uint64_t chQueen {1ULL << std::countr_zero(checkingQueens)};
            checkingQueens &= ~chQueen;
            // it is important to deconstruct the attack like this or we may move a defending piece to any spot where the rays intersect instead of only being able to block and capture 
            uint64_t rookLikeAttack = pseudoLegalRookMoves(chQueen, friendly, enemies);
            uint64_t bishopLikeAttack = pseudoLegalBishopMoves(chQueen, friendly, enemies);

            // std::println("queen rooklike attack");
            // helpers::printBitboard(rookLikeAttack);
            // std::println("queen bihoplike attack");
            // helpers::printBitboard(bishopLikeAttack);
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
    int king_place = __builtin_ctzll(king);
    int king_rank = (king_place) / 8 + 1;
    int king_file = (king_place) % 8 + 1;

    uint64_t attacked_squares = 0;

    for (const auto &d : how_the_king_moves) {
        // prevent teleportation
        if (king_rank + d[0] <= 0 || king_rank + d[0] > 8 || king_file + d[1] <= 0 || king_file + d[1] > 8) {
            continue;
        }

        int shiftDist = d[0] * 8 + d[1];
        uint64_t attacked_square = shiftDist >= 0 ? king << shiftDist : king >> (-shiftDist);
        attacked_squares |= attacked_square; 
    }
    return attacked_squares;
};

inline kingMoveReturn singleKingMove(uint64_t king, const chessBoard& board, std::optional<uint64_t> enemy_attacks, uint64_t enemies, uint64_t friendly) {
    // Timer<Timers::KingMoveFunction> t{};
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    kingMoveReturn retVal;

    retVal.normalMoves = pseudoLegalKingMoves(king, friendly);
    retVal.normalMoves &= (~friendly);

    uint64_t all_pieces = enemies | friendly;

    if (enemy_attacks.has_value()) {

        retVal.normalMoves &= (~enemy_attacks.value());

        if (!(board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsLeft : board_state::BlackLostCastlingRightsLeft))) {
            uint64_t black_king_starting_square = 0x10;
            uint64_t black_left_rook_castling_square = 0x1;
            uint64_t black_square_to_check_not_attacked= 0xc;
            uint64_t black_squares_to_check_empty_long_castle = 0xe;
            // check all the pieces are in the right places, redundant by design with the above check
            if ((king & (isWhiteTurn ? (black_king_starting_square << 56) : black_king_starting_square)) && !(king & enemy_attacks.value()) &&
               ((isWhiteTurn ? board.bitboards[PieceType::Rook + 6] : board.bitboards[PieceType::Rook]) & (isWhiteTurn ? black_left_rook_castling_square << 56 : black_left_rook_castling_square)) &&
               !((isWhiteTurn ? black_squares_to_check_empty_long_castle << 56 : black_squares_to_check_empty_long_castle) & (all_pieces) ) && 
               !((isWhiteTurn ? black_square_to_check_not_attacked << 56 : black_square_to_check_not_attacked) & (enemy_attacks.value()) )
               )
            {

                uint8_t boardStateToXor = isWhiteTurn ? 
                        board_state::WhiteTurn | board_state::WhiteLostCastlingRightsRight | board_state::WhiteLostCastlingRightsLeft : 
                        board_state::WhiteTurn | board_state::BlacklostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft;

                boardStateToXor ^= board.m_board_state & boardStateToXor & board_state::allCastlingFields_const;

                retVal.castlingLeft = isWhiteTurn ? 
                    pieceMovement{
                        (black_king_starting_square >> 2 | black_king_starting_square) << 56, 
                        (black_left_rook_castling_square | black_left_rook_castling_square << 3) << 56, 
                        PieceType::King, PieceType::NotAPiece, 
                        PieceType::Rook, PieceType::NotAPiece, 
                        board.enPassantState, boardStateToXor
                    } : 
                    pieceMovement{
                        black_king_starting_square >> 2 | black_king_starting_square, 
                        black_left_rook_castling_square | black_left_rook_castling_square << 3, 
                        PieceType::NotAPiece, PieceType::King, 
                        PieceType::NotAPiece, PieceType::Rook, 
                        board.enPassantState, boardStateToXor
                    };
            }
        }

        // can castle right (board state)
        if (!(board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsRight : board_state::BlacklostCastlingRightsRight))) {
            uint64_t black_king_starting_square = 0x10;
            uint64_t black_squares_to_check_empty_short_castle = 0x60;
            uint64_t black_right_rook_castling_square = 0x80;

            // check the squares have what they should, redundant by design with the above check
            if((king & (isWhiteTurn ? (black_king_starting_square << 56) : black_king_starting_square)) && !(king & enemy_attacks.value()) &&
              ((isWhiteTurn ? board.bitboards[PieceType::Rook + 6] : board.bitboards[PieceType::Rook]) & (isWhiteTurn ? black_right_rook_castling_square << 56 : black_right_rook_castling_square)) &&
              !((isWhiteTurn ? black_squares_to_check_empty_short_castle << 56 : black_squares_to_check_empty_short_castle) & (all_pieces | enemy_attacks.value()) )) 
            {
                uint8_t boardStateToXor = isWhiteTurn ? 
                        board_state::WhiteTurn | board_state::WhiteLostCastlingRightsRight | board_state::WhiteLostCastlingRightsLeft : 
                        board_state::WhiteTurn | board_state::BlacklostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft;

                boardStateToXor ^= board.m_board_state & boardStateToXor & board_state::allCastlingFields_const;

                retVal.castlingRight = isWhiteTurn ?
                    pieceMovement{
                        (black_king_starting_square << 2 | black_king_starting_square) << 56, 
                        (black_right_rook_castling_square | black_right_rook_castling_square >> 2) << 56, 
                        PieceType::King, PieceType::NotAPiece,
                        PieceType::Rook, PieceType::NotAPiece, 
                        board.enPassantState, boardStateToXor
                    } :
                    pieceMovement{
                        black_king_starting_square << 2 | black_king_starting_square,
                        black_right_rook_castling_square | black_right_rook_castling_square >> 2,
                        PieceType::NotAPiece, PieceType::King, 
                        PieceType::NotAPiece, PieceType::Rook, 
                        board.enPassantState, boardStateToXor
                    };
            }
        }
    }

    return retVal;
}

const stackStack218& makeAllMoves(const chessBoard& boardInput) {
    // Timer<Timers::MakeAllMoves> t {};
    static stackStack218 moveStack {};
    moveStack.currentNumberItems = 0;

    chessBoard board {boardInput}; // interesting copying the board seems to increase the cache hit rate and increase performance


    bool isWhiteTurn = (board_state::WhiteTurn & board.m_board_state) != 0;
    uint64_t friendly_pieces = isWhiteTurn ? board.whitePieces() : board.blackPieces();
    uint64_t enemy_pieces = !isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t enemy_pawns = !isWhiteTurn ? board.bitboards[PieceType::Pawn + 6] : board.bitboards[PieceType::Pawn];

    uint64_t enemy_attacks_mushed {0ULL};

    FastStack<uint64_t, 13> pinsEmpty {};
    FastStack<uint64_t, 2> checksEmpty {};
    // TODO rename attack bitboard generator functions

    // this whole block of code is to generate an enemy attack bitboard 
    for (uint8_t i = 0; i < 6; ++ i) {
        bool isWhiteTurnInner = !isWhiteTurn;
        uint64_t pieces = board.getPiecesByColorConst(isWhiteTurnInner)[i];
        while (pieces) {
            uint64_t piece = 1ULL << std::countr_zero(pieces);
            pieces &= ~piece;
            uint64_t attack;

            uint64_t kingOfEnemyColor_inner = isWhiteTurn ? board.bitboards[PieceType::King + 6] : board.bitboards[PieceType::King];
            // this prevents a bug where the king can move backwards out of an attacked square and still be checked
            uint64_t enemies_inner = friendly_pieces & ~kingOfEnemyColor_inner;
            uint64_t friendly_inner= enemy_pieces;

            // fixed bug with pawn attack generation
            // uint64_t occupied_inner = friendly_inner | enemies_inner;
            switch (i) {
            case (0):
            attack = isWhiteTurnInner ? chessMoves::simpleWhitePawnCapture(piece, ~0ULL) : chessMoves::simpleBlackPawnCapture(piece, ~0ULL);
            break;
            case (1):
            attack = chessMoves::pseudoLegalRookMoves(piece, enemies_inner, friendly_inner);
            break;
            case (2):
            attack = chessMoves::pseudoLegalKnightMoves(piece, friendly_inner);
            break;
            case (3):
            attack = chessMoves::pseudoLegalBishopMoves(piece, enemies_inner, friendly_inner);
            break;
            case (4):
            {
            uint64_t attack_rooklike = chessMoves::pseudoLegalRookMoves(piece, enemies_inner, friendly_inner);
            uint64_t attack_bishoplike = chessMoves::pseudoLegalBishopMoves(piece, enemies_inner, friendly_inner);
            attack = attack_rooklike | attack_bishoplike;
            break;
            }
            case (5):
            attack = chessMoves::pseudoLegalKingMoves(piece, friendly_inner);
            break;
            }
            enemy_attacks_mushed |= (attack);
        }
    }

    // std::println("############ pins and checks ############");
    // calculating the moves
    FastStack<uint64_t, 13> pinLines {};
    chessMoves::calculatePinLineMasks(board, pinLines, enemy_pieces, friendly_pieces, isWhiteTurn);
    // for (auto pin : pinLines) {
    //     std::println("pins for this move");
    //     helpers::printBitboard(pin);
    // }

    FastStack<uint64_t, 2> checkingAttacks{};
    chessMoves::calculateCheckMasks(isWhiteTurn, board, enemy_pieces, friendly_pieces, checkingAttacks);

    // for (auto check : checkingAttacks) {
    //     std::println("checks for this move");
    //     helpers::printBitboard(check);
    // }

    for (uint8_t i = 0; i < 6; ++ i) {
        // 0 pawns, 1 rooks, 2 knights, 3 bishops, 4 queens, 5 king
        uint64_t pieces = board.getPiecesByColorConst(isWhiteTurn)[i];

        while (pieces) {
            uint64_t piece = 1ULL << std::countr_zero(pieces);
            pieces &= ~piece;
            switch (i) {
            case (0):
            {
            pawnMoveReturn pawnRet = chessMoves::singlePawnMove(piece, enemy_pieces, friendly_pieces,  enemy_pawns, pinLines, checkingAttacks, board);
            uint64_t back_row = isWhiteTurn ? static_cast<uint64_t>(0xff) : static_cast<uint64_t>(0xff) << 56;

            if (pawnRet.normalMovedTo & ~back_row) {
                addAttacksToStack218(piece, pawnRet.normalMovedTo, PieceType::Pawn, moveStack, isWhiteTurn, board, enemy_pieces);
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
                    if (pp & enemy_pieces) {
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
            }
            break;
            case (1):
            {
            uint64_t normalRet = chessMoves::singleRookMove(piece, board, pinLines, checkingAttacks, enemy_pieces, friendly_pieces);
            addAttacksToStack218(piece, normalRet, PieceType{i}, moveStack, isWhiteTurn, board, enemy_pieces);
            break;
            }
            case (2):
            {
            uint64_t normalRet = chessMoves::singleKnightMove(piece, board, pinLines, checkingAttacks, friendly_pieces);
            addAttacksToStack218(piece, normalRet, PieceType{i}, moveStack, isWhiteTurn, board, enemy_pieces);
            break;
            }
            case (3):
            {
            uint64_t normalRet = chessMoves::singleBishopMove(piece, board, pinLines, checkingAttacks, enemy_pieces, friendly_pieces);
            addAttacksToStack218(piece, normalRet, PieceType{i}, moveStack, isWhiteTurn, board, enemy_pieces);
            break;
            }
            case (4):
            {
            uint64_t normalRet = chessMoves::singleQueenMove(piece, board, pinLines, checkingAttacks, enemy_pieces, friendly_pieces);
            addAttacksToStack218(piece, normalRet, PieceType{i}, moveStack, isWhiteTurn, board, enemy_pieces);
            break;
            }
            case (5):
            {
            kingMoveReturn kingRet = chessMoves::singleKingMove(piece, board, enemy_attacks_mushed, enemy_pieces, friendly_pieces);
            addAttacksToStack218(piece, kingRet.normalMoves, PieceType{i}, moveStack, isWhiteTurn, board, enemy_pieces);

            if(kingRet.castlingRight.has_value()) {
                moveStack.push(kingRet.castlingRight.value());
            }

            if(kingRet.castlingLeft.has_value()) {
                moveStack.push(kingRet.castlingLeft.value());
            }

            break;
            }
            }
        }
    }

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

