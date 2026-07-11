#include "pieceMovements.hpp"
#include "chessBoard.h"
#include "helpers.hpp"
#include <bit>
#include <cassert>
#include <cstdint>
#include <optional>
#include <print>
#include <stdexcept>


struct normalPawnMoveReturn {
    uint64_t normalMovedTo;
    std::optional<uint64_t> maybePromotionSquare;
    std::optional<int8_t> maybeEnPassantState;
    
    void printThis() const {
        std::print("normalPawnMoveReturn{{ ");

        std::println("normalMovedTo:");
        helpers::printBitboard(normalMovedTo);
        std::print(", ");

        std::print("maybePromotionSquare:");
        if (maybePromotionSquare.has_value()) {
            std::println();
            helpers::printBitboard(*maybePromotionSquare);
        } else {
            std::print("std::nullopt");
        }
        std::print(", ");

        if (maybeEnPassantState.has_value()) {
            std::print(
                "maybeEnPassantState: {}",
                static_cast<int>(*maybeEnPassantState)
            );
        } else {
            std::print("maybeEnPassantState: std::nullopt");
        }

        std::print("}}");
    }
};


struct singlePawnMoveReturn {
    normalPawnMoveReturn normalRet;
    std::optional<pieceMovement> enPassantReturn;
    
    void printThis() const {
        std::print("singlePawnMoveReturn{{ ");

        std::print("normalRet:");
        normalRet.printThis();
        std::print(", ");

        std::print("enPassantReturn:");
        if (enPassantReturn.has_value()) {
            std::println();
            enPassantReturn->printThis();
        } else {
            std::print("std::nullopt");
        }

        std::print("}}");
    }
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
namespace innerMachinations {


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



uint64_t applyPinsToPiece(uint64_t piece, uint64_t attacked_squares, const FastStack<uint64_t, 13>& pins) {
    for (const auto& p: pins) {
        if (p & piece) {
            attacked_squares &= p;
        }
    }
    return attacked_squares;
}

uint64_t applyChecksToPiece(uint64_t piece, uint64_t attacked_squares, const FastStack<uint64_t, 2>& checkingAttacks) {
    for (const auto& c : checkingAttacks) {
        attacked_squares &= c;
    }
    return attacked_squares;
}


template <bool isWhiteTurn>
normalPawnMoveReturn
normalPawnMove(uint64_t pawn, uint64_t enemies, uint64_t friendly, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& enemyCheckingAttacks, uint64_t enemyPawns)
{
    int pawn_place = __builtin_ctzll(pawn);
    int pawn_file = pawn_place % 8;

    std::optional<int8_t> maybeEnPassantState = std::nullopt;

    bool can_have_right_bitshift_by_1 = pawn_file != 0;
    bool can_have_left_bitshift_by_1 = pawn_file != 7;

    uint64_t empty_space = ~(enemies | friendly);
    uint64_t white_front_pawn_row = 0xff000000000000;
    uint64_t black_front_pawn_row = 0xff00;

    uint64_t front_pawn_row = isWhiteTurn ? white_front_pawn_row : black_front_pawn_row;
    uint64_t pawnMovedOne = (isWhiteTurn ? pawn >> 8 : pawn << 8) & empty_space;

    // fixed pawn phasing bug
    uint64_t pawnMovedTwo = (pawn & front_pawn_row ? (isWhiteTurn ? pawnMovedOne >> 8 : pawnMovedOne << 8) : 0) & empty_space;

    uint64_t pawnMoveTwoShoulder = (can_have_left_bitshift_by_1 ? pawnMovedTwo << 1 : 0 ) | (can_have_right_bitshift_by_1 ? pawnMovedTwo >> 1 : 0);
    if (pawnMoveTwoShoulder & enemyPawns) {
        maybeEnPassantState = std::countr_zero(pawnMovedOne);
    }

    uint64_t pawnCapture = isWhiteTurn ? generateSimpleWhitePawnCaptureNoTeleport(pawn, enemies) : generateSimpleBlackPawnCaptureNoTeleport(pawn, enemies);

    uint64_t attacked_squares = pawnMovedOne | pawnMovedTwo | pawnCapture;
    uint64_t back_row = isWhiteTurn ? static_cast<uint64_t>(0xff) : static_cast<uint64_t>(0xff) << 56;

    attacked_squares = applyPinsToPiece(pawn, attacked_squares, pinLines);
    attacked_squares = applyChecksToPiece(pawn, attacked_squares, enemyCheckingAttacks);
    
    std::optional<uint64_t> maybePromotion = std::nullopt;

    if (attacked_squares & back_row) {
        maybePromotion = attacked_squares & back_row;
    }

    return {attacked_squares & (~back_row), maybePromotion, maybeEnPassantState};
}


template <bool isWhiteTurn>
std::optional<pieceMovement>
pawnMoveEPP(uint64_t pawn, uint64_t enemies, uint64_t friendly, uint64_t enemy_pawns, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& enemyCheckingAttacks, const chessBoard& board) {
    assert(board.enPassantState != -1);
    // std::println("epp state of board, {}", board.enPassantState);

    uint64_t eppCaptureSquare = 1ULL << board.enPassantState;

    // helpers::printBitboard(eppCaptureSquare);

    uint64_t locationMovedTo = isWhiteTurn ? generateSimpleWhitePawnCaptureNoTeleport(pawn, eppCaptureSquare) : generateSimpleBlackPawnCaptureNoTeleport(pawn, eppCaptureSquare);

    if (!locationMovedTo) {
        return std::nullopt;
    }

    uint64_t pawnWeCapture = isWhiteTurn ? locationMovedTo << 8 : locationMovedTo >> 8;
    // helpers::printBitboard(pawnWeCapture);

    locationMovedTo = applyPinsToPiece(pawn, locationMovedTo, pinLines);

    pawnWeCapture = applyChecksToPiece(pawn, pawnWeCapture, enemyCheckingAttacks);

    if (locationMovedTo == 0 || pawnWeCapture == 0) {
        return std::nullopt;
    }

    if (isWhiteTurn) {
        return pieceMovement{pawn | locationMovedTo, pawnWeCapture, PieceType::Pawn, PieceType::NotAPiece, PieceType::NotAPiece, PieceType::Pawn, board.enPassantState, board_state::WhiteTurn};
    } else {
        return pieceMovement{pawn | locationMovedTo, pawnWeCapture, PieceType::NotAPiece, PieceType::Pawn, PieceType::Pawn, PieceType::NotAPiece, board.enPassantState, board_state::WhiteTurn};
    }
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
        for (auto &mov : innerMachinations::how_the_bishop_moves) {
            pinned_squares |= scanPinRay(same_color_pieces, opposite_pieces, opposite_king, piece_rank, piece_file, mov[0], mov[1]);
        }
    } else if (pt == PieceType::Rook) {
        for (auto &mov : innerMachinations::how_the_rook_moves) {
            pinned_squares |= scanPinRay(same_color_pieces, opposite_pieces, opposite_king, piece_rank, piece_file, mov[0], mov[1]);
        }
    } else if (pt == PieceType::Queen) {
        for (auto &mov : innerMachinations::how_the_queen_moves) {
            pinned_squares |= scanPinRay(same_color_pieces, opposite_pieces, opposite_king, piece_rank, piece_file, mov[0], mov[1]);
        }
    } 

    return pinned_squares;
}
}

inline uint64_t generateSimpleWhitePawnCaptureNoTeleport(uint64_t white_pawn, uint64_t occupied) {
    int pawn_place = __builtin_ctzll(white_pawn);
    int pawn_file = pawn_place % 8;

    bool can_have_right_bitshift_by_1 = pawn_file != 0;
    bool can_have_left_bitshift_by_1 = pawn_file != 7;

    return (can_have_left_bitshift_by_1 ? ((white_pawn >> 7) & occupied) : 0ULL) | (can_have_right_bitshift_by_1 ? ((white_pawn >> 9) & occupied) : 0ULL);
}

inline uint64_t generateSimpleBlackPawnCaptureNoTeleport(uint64_t black_pawn, uint64_t occupied) {
    int pawn_place = __builtin_ctzll(black_pawn);
    int pawn_file = pawn_place % 8;

    bool can_have_right_bitshift_by_1 = pawn_file != 0;
    bool can_have_left_bitshift_by_1 = pawn_file != 7;
    
    return (can_have_right_bitshift_by_1 ? ((black_pawn  << 7) & occupied) : 0ULL) | (can_have_left_bitshift_by_1 ? ((black_pawn  << 9) & occupied) : 0ULL);
}

singlePawnMoveReturn
singlePawnMove(uint64_t attacking_pawn, uint64_t enemies, uint64_t friendly,  uint64_t enemy_pawns, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks, const chessBoard& board) {
    assert(board.enPassantState<= 63);
    bool hasEnPassant = board.enPassantState >= 0;
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;

    singlePawnMoveReturn retval;

    retval.normalRet = isWhiteTurn ? 
          innerMachinations::normalPawnMove<true>(attacking_pawn, enemies, friendly, pinLines, checkingAttacks, enemy_pawns)
        : innerMachinations::normalPawnMove<false>(attacking_pawn, enemies, friendly, pinLines, checkingAttacks, enemy_pawns);

    retval.enPassantReturn = hasEnPassant ? 
            (isWhiteTurn ? 
              innerMachinations::pawnMoveEPP<true>(attacking_pawn, enemies, friendly,  enemy_pawns, pinLines, checkingAttacks, board)
            : innerMachinations::pawnMoveEPP<false>(attacking_pawn, enemies, friendly,  enemy_pawns, pinLines, checkingAttacks, board))
        : std::nullopt;

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

FastStack<uint64_t, 13> calculate_pin_lines(const chessBoard& board) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();

    // uint64_t friendly_king = isWhiteTurn ? board.m_white_king : board.m_black_king;
    //
    // uint64_t enemy_rooks = !isWhiteTurn ? board.m_white_rooks : board.m_black_rooks;
    // uint64_t enemy_bishops = !isWhiteTurn ? board.m_white_bishops : board.m_black_bishops;
    // uint64_t enemy_queens = !isWhiteTurn ? board.m_white_queens : board.m_black_queens;

    uint64_t friendly_king = isWhiteTurn ? board.bitboards[PieceType::King + 6] : board.bitboards[PieceType::King];

    uint64_t enemy_rooks = !isWhiteTurn ? board.bitboards[PieceType::Rook + 6] : board.bitboards[PieceType::Rook];
    uint64_t enemy_bishops = !isWhiteTurn ? board.bitboards[PieceType::Bishop + 6] : board.bitboards[PieceType::Bishop];
    uint64_t enemy_queens = !isWhiteTurn ? board.bitboards[PieceType::Queen + 6] : board.bitboards[PieceType::Queen];

    FastStack rookStack{seperateBitboardFastStackReturn<10>(enemy_rooks)};
    FastStack bishopStack {seperateBitboardFastStackReturn<10>(enemy_bishops)};
    FastStack queenStack {seperateBitboardFastStackReturn<9>(enemy_queens)};
    FastStack<uint64_t,13> pin_lines;

    for (uint64_t rook : rookStack) {
        // printBitboard(innerMachinations::output_pinned_squares_for_piece<PieceType::Rook>(rook, enemies, friendly, friendly_king));
        auto pin = innerMachinations::output_pinned_squares_for_piece<PieceType::Rook>(rook, enemies, friendly, friendly_king);
        if (pin != 0) {
            pin_lines.push(pin);
        }
    }

    for (uint64_t bishop : bishopStack) {
        auto pin = innerMachinations::output_pinned_squares_for_piece<PieceType::Bishop>(bishop, enemies, friendly, friendly_king);
        if (pin != 0) {
            pin_lines.push(pin);
        }
    }

    for (uint64_t queen : queenStack) {
        auto pin = innerMachinations::output_pinned_squares_for_piece<PieceType::Queen>(queen, enemies, friendly, friendly_king);
        if (pin != 0) {
            pin_lines.push(pin);
        }
    }
    return pin_lines;
}

uint64_t singleRookMoveNoPinOrCheck_forLoop(uint64_t rook, uint64_t enemies, uint64_t friendly) {
    int rook_place = __builtin_ctzll(rook);

    int rook_rank = rook_place / 8;
    int rook_file = rook_place % 8;

    uint64_t attacked_squares = 0;

    for (const auto &d : innerMachinations::how_the_rook_moves) {
        attacked_squares |= attack_with_increment_sliding_piece(
            enemies, friendly,
            {rook_rank, rook_file},
            {d[0], d[1]}
        );
    }

    return attacked_squares;
}

uint64_t singleRookMove(uint64_t rook, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t attacked_squares = singleRookMoveNoPinOrCheck_forLoop(rook, enemies, friendly);

    attacked_squares ^= (attacked_squares & friendly); // un attacks the friendly pieces
    attacked_squares = innerMachinations::applyPinsToPiece(rook, attacked_squares, pinLines);
    attacked_squares = innerMachinations::applyChecksToPiece(rook, attacked_squares, checkingAttacks);

    return attacked_squares;
}

uint64_t singleBihopMoveNoPinOrCheck_forLoop(uint64_t bishop, uint64_t enemies, uint64_t friendly) {
    int bishop_place = __builtin_ctzll(bishop);

    int bishop_rank = bishop_place / 8;
    int bishop_file = bishop_place % 8;

    uint64_t attacked_squares = 0;

    for (auto &d : innerMachinations::how_the_bishop_moves) {
        attacked_squares |= attack_with_increment_sliding_piece(
            enemies, friendly,
            {bishop_rank, bishop_file},
            {d[0], d[1]}
        );
    }
    return attacked_squares;
}

uint64_t singleBishopMove(uint64_t bishop, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t attacked_squares = singleBihopMoveNoPinOrCheck_forLoop(bishop, enemies, friendly);

    attacked_squares ^= (attacked_squares & friendly); // un attacks the friendly pieces
    attacked_squares = innerMachinations::applyPinsToPiece(bishop, attacked_squares, pinLines);
    attacked_squares = innerMachinations::applyChecksToPiece(bishop, attacked_squares, checkingAttacks);

    return attacked_squares;
}

uint64_t singleQueenMove(uint64_t queen, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t attacked_squares = singleRookMoveNoPinOrCheck_forLoop(queen, enemies, friendly) | singleBihopMoveNoPinOrCheck_forLoop(queen, enemies, friendly);

    attacked_squares &= (~friendly); // un attacks the friendly pieces
    attacked_squares = innerMachinations::applyPinsToPiece(queen, attacked_squares, pinLines);
    attacked_squares = innerMachinations::applyChecksToPiece(queen, attacked_squares, checkingAttacks);

    return attacked_squares;
}

uint64_t generateKnightMovesNoPinCheckTeleport(uint64_t knight, uint64_t friendly) {
    int knight_place= __builtin_ctzll(knight);
    int knight_rank = (knight_place) / 8 + 1;
    int knight_file = (knight_place) % 8 + 1;

    uint64_t attacked_squares = 0ULL;

    for (const auto& d : innerMachinations::how_the_knight_moves) {
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
uint64_t singleKnightMove(uint64_t knight, const chessBoard& board, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks) {
    // std::println("generating knight move, checks here : ");
    // for (auto ch : checkingAttacks) {
    //     helpers::printBitboard(ch);
    // }
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();
    uint64_t attacked_squares = generateKnightMovesNoPinCheckTeleport(knight, friendly);
    attacked_squares &= (~friendly);
    attacked_squares = innerMachinations::applyPinsToPiece(knight, attacked_squares, pinLines);
    attacked_squares = innerMachinations::applyChecksToPiece(knight, attacked_squares, checkingAttacks);

    return attacked_squares;
}

// we can pass in the sliding piece attacks since we now know this function will only be ran once per turn due to simply not generating moves that end with us in check
// passing in the sliding piece attacks saves on computation, limiting the number of sliding piece calculations we need to perform
FastStack<uint64_t, 2> calculateChecks(bool isWhiteTheColorBeingChecked, const chessBoard& board) {
   
    uint64_t king_possibly_checked = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::King + 6] : board.bitboards[PieceType::King];

    uint64_t enemies = isWhiteTheColorBeingChecked ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTheColorBeingChecked ? board.whitePieces() : board.blackPieces();

    uint64_t enemyPawns = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::Pawn] : board.bitboards[PieceType::Pawn + 6];

    uint64_t kingSeesLikePawn = isWhiteTheColorBeingChecked ? generateSimpleWhitePawnCaptureNoTeleport(king_possibly_checked, enemyPawns) : generateSimpleBlackPawnCaptureNoTeleport(king_possibly_checked, enemyPawns); 
    uint64_t kingScanRayBishopLike = singleBihopMoveNoPinOrCheck_forLoop(king_possibly_checked, enemies, friendly);
    uint64_t kingScanRayRookLike = singleRookMoveNoPinOrCheck_forLoop(king_possibly_checked, enemies, friendly);
    uint64_t kingSeesLikeKnight = generateKnightMovesNoPinCheckTeleport(king_possibly_checked, friendly);



    FastStack<uint64_t, 2> checks {};


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
        uint64_t enemyRookAttack = singleRookMoveNoPinOrCheck_forLoop(kingScanRayRookLike & enemyRooks, friendly, enemies);
        if (enemyRookAttack & king_possibly_checked) {
            checks.push((enemyRookAttack & kingScanRayRookLike) | (kingScanRayRookLike & enemyRooks));
        }
    }

    uint64_t enemyBishops = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::Bishop] : board.bitboards[PieceType::Bishop + 6];
    // same as for the rooks, if we are on a diagonal with a king then we need two moves to check from another diagonal therefore we cant have 2 bishop checks
    if (kingScanRayBishopLike & enemyBishops) {
        assert(std::popcount(kingScanRayBishopLike & enemyBishops) == 1);
        uint64_t enemyBishopAttack = singleBihopMoveNoPinOrCheck_forLoop(kingScanRayBishopLike & enemyBishops, friendly, enemies);
        if (enemyBishopAttack & king_possibly_checked) {
            checks.push((enemyBishopAttack & kingScanRayBishopLike) | (kingScanRayBishopLike & enemyBishops));
        }
    }

    uint64_t kingScanRayQueenLike = kingScanRayBishopLike | kingScanRayRookLike;
    uint64_t enemyQueens = isWhiteTheColorBeingChecked ? board.bitboards[PieceType::Queen] : board.bitboards[PieceType::Queen + 6];
    if (kingScanRayQueenLike & enemyQueens) {
        auto checkingQueens = seperateBitboardFastStackReturn<2>(kingScanRayQueenLike & enemyQueens);
        for (uint64_t queenPiece: checkingQueens) {
            // it is important to deconstruct the attack like this or we may move a defending piece to any spot where the rays intersect instead of only being able to block and capture 
            uint64_t rookLikeAttack = singleRookMoveNoPinOrCheck_forLoop(queenPiece, friendly, enemies);
            uint64_t bishopLikeAttack = singleBihopMoveNoPinOrCheck_forLoop(queenPiece, friendly, enemies);

            // std::println("queen rooklike attack");
            // helpers::printBitboard(rookLikeAttack);
            // std::println("queen bihoplike attack");
            // helpers::printBitboard(bishopLikeAttack);
            uint64_t rooklikeAttackIncPiece = rookLikeAttack | queenPiece;
            uint64_t bishoplikeAttackIncPiece = bishopLikeAttack | queenPiece;

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

    return checks;
}


uint64_t dummyKingMoveGenerationNoTeleportation(uint64_t king, uint64_t friendly) {
    int king_place = __builtin_ctzll(king);
    int king_rank = (king_place) / 8 + 1;
    int king_file = (king_place) % 8 + 1;

    uint64_t attacked_squares = 0;

    for (const auto &d : innerMachinations::how_the_king_moves) {
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

kingMoveReturn singleKingMove(uint64_t king, const chessBoard& board, std::optional<uint64_t> enemy_attacks) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();
    
    kingMoveReturn retVal;

    retVal.normalMoves = dummyKingMoveGenerationNoTeleportation(king, friendly);
    retVal.normalMoves &= (~friendly);

    uint64_t all_pieces = enemies | friendly;

    if (enemy_attacks.has_value()) {

        retVal.normalMoves &= (~enemy_attacks.value());

        if (!(board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsLeft : board_state::BlackLostCastlingRightsLeft))) {
            uint64_t black_king_starting_square = 0x10;
            uint64_t black_left_rook_castling_square = 0x1;
            uint64_t black_squares_to_check_empty_left = 0xe;
            // check all the pieces are in the right places, redundant by design with the above check
            if ((king & (isWhiteTurn ? (black_king_starting_square << 56) : black_king_starting_square)) && !(king & enemy_attacks.value()) &&
               ((isWhiteTurn ? board.bitboards[PieceType::Rook + 6] : board.bitboards[PieceType::Rook]) & (isWhiteTurn ? black_left_rook_castling_square << 56 : black_left_rook_castling_square)) &&
               !((isWhiteTurn ? black_squares_to_check_empty_left << 56 : black_squares_to_check_empty_left) & (all_pieces | enemy_attacks.value()) ))
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
            uint64_t black_squares_to_check_empty_right = 0x60;
            uint64_t black_right_rook_castling_square = 0x80;

            // check the squares have what they should, redundant by design with the above check
            if((king & (isWhiteTurn ? (black_king_starting_square << 56) : black_king_starting_square)) && !(king & enemy_attacks.value()) &&
              ((isWhiteTurn ? board.bitboards[PieceType::Rook + 6] : board.bitboards[PieceType::Rook]) & (isWhiteTurn ? black_right_rook_castling_square << 56 : black_right_rook_castling_square)) &&
              !((isWhiteTurn ? black_squares_to_check_empty_right << 56 : black_squares_to_check_empty_right) & (all_pieces | enemy_attacks.value()) )) 
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

stackStack218 makeAllMoves(const chessBoard& boardInput) {
    stackStack218 moveStack {};
    chessBoard board {boardInput};

    bool isWhiteTurn = (board_state::WhiteTurn & board.m_board_state) != 0;
    uint64_t friendly_pieces = isWhiteTurn ? board.whitePieces() : board.blackPieces();
    uint64_t enemy_pieces = !isWhiteTurn ? board.whitePieces() : board.blackPieces();

    // FastStack pawnStack {seperateBitboard<10>(isWhiteTurn ? board.m_white_pawns : board.m_black_pawns)};

    uint64_t enemy_pawns = !isWhiteTurn ? board.bitboards[PieceType::Pawn + 6] : board.bitboards[PieceType::Pawn];

    // we use an xor operation to change the board state to generate enemy attacks then xor it back, anything xored with itself is zero, anything xored with not itself becomes 1 and anything xored with 1 gets flipped
    
    uint8_t mask_for_bits_we_want_to_be_1 = board_state::WhiteLostCastlingRightsLeft | board_state::WhiteLostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft | board_state::BlacklostCastlingRightsRight;
    uint8_t bitmask_that_flips_bits_to_1 = (~(board.m_board_state & mask_for_bits_we_want_to_be_1)) & mask_for_bits_we_want_to_be_1;
    const uint8_t xor_reversible_transformation = bitmask_that_flips_bits_to_1 | (board.m_board_state & (~mask_for_bits_we_want_to_be_1)) | board_state::WhiteTurn;

    // TODO remove the xor transformation, it requires a copy so that we dont mutate the boardInput, the logic is also more complicated than an extra parameter to our functions

    uint64_t enemy_attacks_mushed {0ULL};
    FastStack<uint64_t, 2> checksOnOurKing{};

    FastStack<uint64_t, 13> pinsEmpty {};
    FastStack<uint64_t, 2> checksEmpty {};
    // TODO rename attack bitboard generator functions

    // this whole block of code is to generate an enemy attack bitboard 
    board.m_board_state ^= xor_reversible_transformation;
    for (uint8_t i = 0; i < 6; ++ i) {
        FastStack pieceStack {seperateBitboardFastStackReturn<10>(board.getPiecesByColorConst(!isWhiteTurn)[i])};
        for (uint64_t piece : pieceStack) {
            uint64_t attack;


            bool isWhiteTurnInner = board.m_board_state & board_state::WhiteTurn;
            uint64_t kingOfEnemyColor_inner = isWhiteTurn ? board.bitboards[PieceType::King + 6] : board.bitboards[PieceType::King];
            // this prevents a bug where the king can move backwards out of an attacked square and still be checked
            uint64_t enemies_inner = friendly_pieces & ~kingOfEnemyColor_inner;
            uint64_t friendly_inner= enemy_pieces;

            // fixed bug with pawn attack generation
            // uint64_t occupied_inner = friendly_inner | enemies_inner;
            switch (i) {
                case (0):
                    attack = isWhiteTurnInner ? chessMoves::generateSimpleWhitePawnCaptureNoTeleport(piece, ~0ULL) : chessMoves::generateSimpleBlackPawnCaptureNoTeleport(piece, ~0ULL);
                    break;
                case (1):
                    attack = chessMoves::singleRookMoveNoPinOrCheck_forLoop(piece, enemies_inner, friendly_inner);
                    break;
                case (2):
                    attack = chessMoves::generateKnightMovesNoPinCheckTeleport(piece, friendly_inner);
                    break;
                case (3):
                    attack = chessMoves::singleBihopMoveNoPinOrCheck_forLoop(piece, enemies_inner, friendly_inner);
                    break;
                case (4):
                    {
                    uint64_t attack_rooklike = chessMoves::singleRookMoveNoPinOrCheck_forLoop(piece, enemies_inner, friendly_inner);
                    uint64_t attack_bishoplike = chessMoves::singleBihopMoveNoPinOrCheck_forLoop(piece, enemies_inner, friendly_inner);
                    attack = attack_rooklike | attack_bishoplike;
                    break;
                    }
                case (5):
                    attack = chessMoves::dummyKingMoveGenerationNoTeleportation(piece, friendly_inner);
                    break;
            }
            // IMPORTANT ADDESS WHEN POSSIBLE
            // theres actually a bug here with the attack map generation, if the king is in the 
            // line of a sliding piece then the squares behind the attacked king arent actually attacked
            // so the king is allowed to move backwards, so is still attacked by the sliding piece
            //
            //
            // there is another bug with attack generating for king moves, the king can move to a square
            // which is attacked by a pawn
            //
            // TODO remove king from occupancy to avoid the sliding piece king blocking bug, the rook / bishop attacks will still 
            // be used for check generation since we xor with the kings own attack ray which will erase the attacked squares behind the king
            //
            // TODO change the behaviour of pawn attack generation to attack squares then in the pawn move generation 
            // function check for occupancy
            enemy_attacks_mushed |= (attack);
        }
    }
    board.m_board_state ^= xor_reversible_transformation;

    // std::println("############ pins and checks ############");
    // calculating the moves
    FastStack<uint64_t, 13> pinLines = chessMoves::calculate_pin_lines(board);
    // for (auto pin : pinLines) {
    //     std::println("pins for this move");
    //     helpers::printBitboard(pin);
    // }

    FastStack<uint64_t, 2> checkingAttacks = chessMoves::calculateChecks(isWhiteTurn, board);

    // for (auto check : checkingAttacks) {
    //     std::println("checks for this move");
    //     helpers::printBitboard(check);
    // }

    for (uint8_t i = 0; i < 6; ++ i) {
        // 0 pawns, 1 rooks, 2 knights, 3 bishops, 4 queens, 5 king
        FastStack pieceStack {seperateBitboardFastStackReturn<10>(board.getPiecesByColorConst(isWhiteTurn)[i])};
        for (uint64_t piece : pieceStack) {
            // std::tuple<uint64_t, std::optional<uint64_t>, std::optional<pieceMovement>> pawnRet;
            singlePawnMoveReturn pawnRet;
            // std::pair<uint64_t, std::pair<std::optional<pieceMovement>, std::optional<pieceMovement>>> kingRet;
            kingMoveReturn kingRet;
            uint64_t normalRet;
            switch (i) {
                case (0):
                    // should really be handling en passant in this function, so we dont need the pass board down 3 function calls, we're essentially 
                    // threading a pieceMovement all the way back up
                    pawnRet = chessMoves::singlePawnMove(piece, enemy_pieces, friendly_pieces,  enemy_pawns, pinLines, checkingAttacks, board);
                    break;
                case (1):
                    normalRet = chessMoves::singleRookMove(piece, board, pinLines, checkingAttacks);
                    break;
                case (2):
                    normalRet = chessMoves::singleKnightMove(piece, board, pinLines, checkingAttacks);
                    break;
                case (3):
                    normalRet = chessMoves::singleBishopMove(piece, board, pinLines, checkingAttacks);
                    break;
                case (4):
                    normalRet = chessMoves::singleQueenMove(piece, board, pinLines, checkingAttacks);
                    break;
                case (5):
                    kingRet = chessMoves::singleKingMove(piece, board, enemy_attacks_mushed);
                    break;
            }

            if (i == 0) {
                addAttacksToStack218<4>(piece, pawnRet.normalRet.normalMovedTo, PieceType::Pawn, moveStack, isWhiteTurn, board);

                if(pawnRet.normalRet.maybePromotionSquare.has_value()) {
                    // std::cout << "pawn promotion detected!!" << std::endl;
                    uint64_t pawnPromotionSquare = pawnRet.normalRet.maybePromotionSquare.value();
                    FastStack<uint64_t, 3> promotionSquares = seperateBitboardFastStackReturn<3>(pawnPromotionSquare);
                    
                    for (uint64_t pp : promotionSquares) {
                        PieceType enemyPieceTypeOnPromotionSquare = board.figureOutTypeOfPieceOnSquare(pp, !isWhiteTurn);
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
                if(pawnRet.enPassantReturn.has_value()) {
                    moveStack.push(pawnRet.enPassantReturn.value());
                }
            } else if (i > 0 && i < 5) {
                addAttacksToStack218<27>(piece, normalRet, PieceType{i}, moveStack, isWhiteTurn, board);
            } else {
                addAttacksToStack218<8>(piece, kingRet.normalMoves, PieceType{i}, moveStack, isWhiteTurn, board);

                if(kingRet.castlingRight.has_value()) {
                    moveStack.push(kingRet.castlingRight.value());
                }

                if(kingRet.castlingLeft.has_value()) {
                    moveStack.push(kingRet.castlingLeft.value());
                }
            }
        }
    }

    return moveStack;
}
}
