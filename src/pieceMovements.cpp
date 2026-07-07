#include "pieceMovements.hpp"
#include "helpers.hpp"
#include <cassert>
#include <cstdint>
#include <print>

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
std::pair<uint64_t, std::optional<uint64_t>> 
normalPawnMove(uint64_t pawn, uint64_t enemies, uint64_t friendly, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& enemyCheckingAttacks)
{
    uint64_t empty_space = ~(enemies | friendly);
    uint64_t white_front_pawn_row = static_cast<uint64_t>(0xff) << 48;
    uint64_t black_front_pawn_row = 0xff00;
    uint64_t front_pawn_row = isWhiteTurn ? white_front_pawn_row : black_front_pawn_row;
    uint64_t pawnMovedOne = (isWhiteTurn ? pawn >> 8 : pawn << 8) & empty_space;
    uint64_t pawnMovedTwo = (pawn & front_pawn_row ? (isWhiteTurn ? pawn >> 16: pawn << 16) : 0) & empty_space;
    uint64_t pawnCapture = isWhiteTurn ? generateSimpleWhitePawnCaptureNoTeleport(pawn, enemies) & (~friendly) : generateSimpleBlackPawnCaptureNoTeleport(pawn, enemies) & ~friendly;
    uint64_t attacked_squares = pawnMovedOne | pawnMovedTwo | pawnCapture;
    uint64_t back_row = static_cast<uint64_t>(0xff);

    attacked_squares = applyPinsToPiece(pawn, attacked_squares, pinLines);
    attacked_squares = applyChecksToPiece(pawn, attacked_squares, enemyCheckingAttacks);
    
    if (attacked_squares & back_row) {
        return {attacked_squares & (~back_row), attacked_squares & back_row};
    } else {
        return {attacked_squares, std::nullopt};
    }
}

std::optional<pieceMovement> eppMoveInner(uint64_t movingPawn, uint64_t shoulder, uint64_t capture_position, uint64_t eppCaptureSquare, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& enemyCheckingAttacks) {

    if (capture_position & eppCaptureSquare) {
        // we may move while pinned if the capture position is still within the pin if we are pinned multiple times this cant really happen
        capture_position = applyPinsToPiece(movingPawn, capture_position, pinLines);
        // when in check unless double checked we may capture the checking piece, we cant block a check when capturing en passant, its just not possible 
        shoulder = applyChecksToPiece(movingPawn, shoulder, enemyCheckingAttacks);

        if (capture_position == 0 || shoulder == 0) {
            return std::nullopt;
        }

        return pieceMovement{movingPawn| capture_position | shoulder, shoulder, PieceType::Pawn, PieceType::Pawn, true};
    }
    return std::nullopt;
}

template <bool isWhiteTurn>
std::optional<pieceMovement>
pawnMoveEPP(uint64_t white_pawn, uint64_t enemies, uint64_t friendly, int8_t eppState, uint64_t enemy_pawns, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& enemyCheckingAttacks) {
    int pawn_place = __builtin_ctzll(white_pawn);
    int pawn_file = pawn_place % 8;

    bool can_have_right_bitshift_by_1 = pawn_file != 0;
    bool can_have_left_bitshift_by_1 = pawn_file != 7;

    uint64_t eppCaptureSquare = 1ULL << eppState;

    if (can_have_right_bitshift_by_1) {

        uint64_t shoulder = white_pawn >> 1;
        uint64_t capture_position = isWhiteTurn ? shoulder >> 8 : shoulder << 8;
        auto retVal = eppMoveInner(white_pawn, shoulder, capture_position, eppCaptureSquare, pinLines, enemyCheckingAttacks);
        if (retVal.has_value()) {return retVal;}
    }   

    if (can_have_left_bitshift_by_1) {
        uint64_t shoulder = white_pawn << 1;
        uint64_t capture_position = isWhiteTurn ? shoulder >> 8 : shoulder << 8;

        auto retVal = eppMoveInner(white_pawn, shoulder, capture_position, eppCaptureSquare, pinLines, enemyCheckingAttacks);
        if (retVal.has_value()) {return retVal;}
    }

    return std::nullopt;
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

uint64_t generateSimpleWhitePawnCaptureNoTeleport(uint64_t white_pawn, uint64_t occupied) {
    int pawn_place = __builtin_ctzll(white_pawn);
    int pawn_file = pawn_place % 8;

    bool can_have_right_bitshift_by_1 = pawn_file != 0;
    bool can_have_left_bitshift_by_1 = pawn_file != 7;

    return (can_have_left_bitshift_by_1 ? ((white_pawn >> 7) & occupied) : 0ULL) | (can_have_right_bitshift_by_1 ? ((white_pawn >> 9) & occupied) : 0ULL);
}

uint64_t generateSimpleBlackPawnCaptureNoTeleport(uint64_t black_pawn, uint64_t occupied) {
    int pawn_place = __builtin_ctzll(black_pawn);
    int pawn_file = pawn_place % 8;

    bool can_have_right_bitshift_by_1 = pawn_file != 0;
    bool can_have_left_bitshift_by_1 = pawn_file != 7;
    
    return (can_have_right_bitshift_by_1 ? ((black_pawn  << 7) & occupied) : 0ULL) | (can_have_left_bitshift_by_1 ? ((black_pawn  << 9) & occupied) : 0ULL);
}

std::tuple<uint64_t, std::optional<uint64_t>, std::optional<pieceMovement>>
singlePawnMove(uint64_t attacking_pawn, uint64_t enemies, uint64_t friendly, board_state::PawnState pawnState, uint64_t enemy_pawns, const FastStack<uint64_t, 13>& pinLines, const FastStack<uint64_t, 2>& checkingAttacks) {
    assert(pawnState.eppState <= 63);
    bool hasEnPassant = pawnState.eppState>= 0;

    auto [normMove, maybePromotion] = pawnState.isWhiteTurn ? 
          innerMachinations::normalPawnMove<true>(attacking_pawn, enemies, friendly, pinLines, checkingAttacks)
        : innerMachinations::normalPawnMove<false>(attacking_pawn, enemies, friendly, pinLines, checkingAttacks);

    auto maybeEppMove = hasEnPassant ? 
            (pawnState.isWhiteTurn ? 
              innerMachinations::pawnMoveEPP<true>(attacking_pawn, enemies, friendly, pawnState.eppState, enemy_pawns, pinLines, checkingAttacks)
            : innerMachinations::pawnMoveEPP<false>(attacking_pawn, enemies, friendly, pawnState.eppState, enemy_pawns, pinLines, checkingAttacks))
        : std::nullopt;

    return {normMove, maybePromotion, maybeEppMove};
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

    uint64_t friendly_king = isWhiteTurn ? board.m_white_king : board.m_black_king;

    uint64_t enemy_rooks = !isWhiteTurn ? board.m_white_rooks : board.m_black_rooks;
    uint64_t enemy_bishops = !isWhiteTurn ? board.m_white_bishops : board.m_black_bishops;
    uint64_t enemy_queens = !isWhiteTurn ? board.m_white_queens : board.m_black_queens;

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
FastStack<uint64_t, 2> calculateChecks(bool isWhiteTheColorBeingChecked, const chessBoard& board, const FastStack<uint64_t, 10>& enemyRookAttacks, 
        const FastStack<uint64_t, 10>& enemyBishopAttacks, const FastStack<QueenAttackDeconstruction, 9>& enemyQueenAttacks) {
    uint64_t king_possibly_checked = isWhiteTheColorBeingChecked ? board.m_white_king : board.m_black_king;
    uint64_t enemies = isWhiteTheColorBeingChecked ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTheColorBeingChecked ? board.whitePieces() : board.blackPieces();

    uint64_t enemyPawns = isWhiteTheColorBeingChecked ? board.m_black_pawns : board.m_white_pawns;
    uint64_t kingSeesLikePawn = isWhiteTheColorBeingChecked ? generateSimpleWhitePawnCaptureNoTeleport(king_possibly_checked, enemyPawns) : generateSimpleBlackPawnCaptureNoTeleport(king_possibly_checked, enemyPawns); 
    uint64_t kingScanRayBishopLike = singleBihopMoveNoPinOrCheck_forLoop(king_possibly_checked, enemies, friendly);
    uint64_t kingScanRayRookLike = singleRookMoveNoPinOrCheck_forLoop(king_possibly_checked, enemies, friendly);
    uint64_t kingSeesLikeKnight = generateKnightMovesNoPinCheckTeleport(king_possibly_checked, friendly);

    FastStack<uint64_t, 2> checks {};


    if (kingSeesLikePawn & enemyPawns) {
        checks.push(kingSeesLikePawn & enemyPawns);
    }

    uint64_t enemyKnights = isWhiteTheColorBeingChecked ? board.m_black_knights : board.m_white_knights;
    if (kingSeesLikeKnight & enemyKnights) {
        // we cant be double checked by 2 knights since this would require a discovery and knights cant be blocked 
        checks.push(kingSeesLikeKnight & enemyKnights);
    }

    // its still check if the enemy piece is pinned, since checkmate is capturing the opponents king even through this capture is never played
    
    uint64_t enemyRooks = isWhiteTheColorBeingChecked ? board.m_black_rooks : board.m_white_rooks;
    if (kingScanRayRookLike & enemyRooks) {
        for (uint64_t attack : enemyRookAttacks) {
            if (attack & king_possibly_checked) {
                checks.push((attack & kingScanRayRookLike) | (kingScanRayRookLike & enemyRooks));
                break;
            }
        }
    }

    // same as for the rooks, if we are on a diagonal with a king then we need two moves to check from another diagonal therefore we cant have 2 bishop checks
    uint64_t enemyBishops = isWhiteTheColorBeingChecked ? board.m_black_bishops : board.m_white_bishops;
    if (kingScanRayBishopLike & enemyBishops) {
        for (uint64_t attack : enemyBishopAttacks) {
            if (attack & king_possibly_checked) {
                checks.push((attack & kingScanRayBishopLike) | (kingScanRayBishopLike & enemyBishops));
                break;
            }
        }
    }

    uint64_t kingScanRayQueenLike = kingScanRayBishopLike | kingScanRayRookLike;
    uint64_t enemyQueens = isWhiteTheColorBeingChecked ? board.m_black_queens : board.m_white_queens;
    if (kingScanRayQueenLike & enemyQueens) {
        for (const QueenAttackDeconstruction& attack : enemyQueenAttacks) {
            // it is important to deconstruct the attack like this or we may move a defending piece to any spot where the rays intersect instead of only being able to block and capture 
            uint64_t rookCheckLineWithCapture = (attack.rookLikeAttack | attack.queenPiece) & kingScanRayRookLike;
            uint64_t bishopCheckLineWithCapture = (attack.bishopLikeAttack | attack.queenPiece) & kingScanRayBishopLike;

            if (rookCheckLineWithCapture) 
            {
                checks.push(rookCheckLineWithCapture);
            } 
            else if (bishopCheckLineWithCapture) 
            {
                checks.push(bishopCheckLineWithCapture);
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
}

std::pair<uint64_t, std::pair<std::optional<pieceMovement>, std::optional<pieceMovement>>> 
singleKingMove(uint64_t king, const chessBoard& board, std::optional<uint64_t> enemy_attacks) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t attacked_squares = dummyKingMoveGenerationNoTeleportation(king, friendly);
    attacked_squares &= (~friendly);

    uint64_t all_pieces = enemies | friendly;

    std::optional<pieceMovement> castlingLeft {std::nullopt};
    std::optional<pieceMovement> castlingRight {std::nullopt} ;
    if (enemy_attacks.has_value()) {

        attacked_squares &= (~enemy_attacks.value());

        if (!(board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsLeft : board_state::BlackLostCastlingRightsLeft))) {
            uint64_t black_king_starting_square = 0x10;
            uint64_t black_left_rook_castling_square = 0x1;
            uint64_t black_squares_to_check_empty_left = 0xe;
            // check all the pieces are in the right places, redundant by design with the above check
            if ((king & (isWhiteTurn ? (black_king_starting_square << 56) : black_king_starting_square)) && !(king & enemy_attacks.value()) &&
               ((isWhiteTurn ? board.m_white_rooks : board.m_black_rooks) & (isWhiteTurn ? black_left_rook_castling_square << 56 : black_left_rook_castling_square)) &&
               !((isWhiteTurn ? black_squares_to_check_empty_left << 56 : black_squares_to_check_empty_left) & (all_pieces | enemy_attacks.value()) ))
            {
                castlingLeft = isWhiteTurn ? 
                    pieceMovement{(black_king_starting_square >> 2 | black_king_starting_square) << 56, (black_left_rook_castling_square | black_left_rook_castling_square << 3) << 56, PieceType::King, PieceType::Rook, true} : 
                    pieceMovement{black_king_starting_square >> 2 | black_king_starting_square, black_left_rook_castling_square | black_left_rook_castling_square << 3, PieceType::King, PieceType::Rook, true};

            }
        }

        // can castle right (board state)
        if (!(board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsRight : board_state::BlacklostCastlingRightsRight))) {
            uint64_t black_king_starting_square = 0x10;
            uint64_t black_squares_to_check_empty_right = 0x60;
            uint64_t black_right_rook_castling_square = 0x80;

            // check the squares have what they should, redundant by design with the above check
            if((king & (isWhiteTurn ? (black_king_starting_square << 56) : black_king_starting_square)) && !(king & enemy_attacks.value()) &&
              ((isWhiteTurn ? board.m_white_rooks : board.m_black_rooks) & (isWhiteTurn ? black_right_rook_castling_square << 56 : black_right_rook_castling_square)) &&
              !((isWhiteTurn ? black_squares_to_check_empty_right << 56 : black_squares_to_check_empty_right) & (all_pieces | enemy_attacks.value()) )) 
            {
                castlingRight = isWhiteTurn ?
                    pieceMovement{(black_king_starting_square << 2 | black_king_starting_square) << 56, (black_right_rook_castling_square | black_right_rook_castling_square >> 2) << 56, PieceType::King, PieceType::Rook, true} :
                    pieceMovement{black_king_starting_square << 2 | black_king_starting_square, black_right_rook_castling_square | black_right_rook_castling_square >> 2, PieceType::King, PieceType::Rook, true};

            }
        }
    }

    return {attacked_squares, {castlingLeft, castlingRight}};
}

stackStack218 makeAllMoves(const chessBoard& boardInput) {
    stackStack218 moveStack {};
    chessBoard board {boardInput};

    bool isWhiteTurn = (board_state::WhiteTurn & board.m_board_state) != 0;
    uint64_t friendly_pieces = isWhiteTurn ? board.whitePieces() : board.blackPieces();
    uint64_t enemy_pieces = !isWhiteTurn ? board.whitePieces() : board.blackPieces();

    // FastStack pawnStack {seperateBitboard<10>(isWhiteTurn ? board.m_white_pawns : board.m_black_pawns)};

    uint64_t enemy_pawns = !isWhiteTurn ? board.m_white_pawns : board.m_black_pawns;

    // we use an xor operation to change the board state to generate enemy attacks then xor it back, anything xored with itself is zero, anything xored with not itself becomes 1 and anything xored with 1 gets flipped
    
    uint8_t mask_for_bits_we_want_to_be_1 = board_state::WhiteLostCastlingRightsLeft | board_state::WhiteLostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft | board_state::BlacklostCastlingRightsRight;
    uint8_t bitmask_that_flips_bits_to_1 = (~(board.m_board_state & mask_for_bits_we_want_to_be_1)) & mask_for_bits_we_want_to_be_1;
    const uint8_t xor_reversible_transformation = bitmask_that_flips_bits_to_1 | (board.m_board_state & (~mask_for_bits_we_want_to_be_1)) | board_state::WhiteTurn;

    // TODO remove the xor transformation, it requires a copy so that we dont mutate the boardInput, the logic is also more complicated than an extra parameter to our functions

    uint64_t enemy_attacks_mushed {0ULL};
    FastStack<uint64_t, 2> checksOnOurKing{};

    FastStack<uint64_t, 13> pinsEmpty {};
    FastStack<uint64_t, 2> checksEmpty {};
    FastStack<uint64_t, 10> enemyRookAttacks_forCheckCalc {};
    FastStack<uint64_t, 10> enemyBishopAttacks_forCheckCalc {};
    FastStack<QueenAttackDeconstruction, 9> enemyQueenAttacks_forCheckCalc {};
    // TODO rename attack bitboard generator functions

    // this whole block of code is to generate an enemy attack bitboard 
    board.m_board_state ^= xor_reversible_transformation;
    for (uint8_t i = 0; i < 6; ++ i) {
        FastStack pieceStack {seperateBitboardFastStackReturn<10>(board.getPiecesByColorConst(!isWhiteTurn)[i])};
        for (uint64_t piece : pieceStack) {
            uint64_t attack;


            bool isWhiteTurnInner = board.m_board_state & board_state::WhiteTurn;
            uint64_t enemies_inner = friendly_pieces;
            uint64_t friendly_inner= enemy_pieces;
            uint64_t occupied_inner = friendly_inner | enemies_inner;
            switch (i) {
                case (0):
                    attack = isWhiteTurnInner ? chessMoves::generateSimpleWhitePawnCaptureNoTeleport(piece, occupied_inner) : chessMoves::generateSimpleBlackPawnCaptureNoTeleport(piece, occupied_inner);
                    break;
                case (1):
                    attack = chessMoves::singleRookMoveNoPinOrCheck_forLoop(piece, enemies_inner, friendly_inner);
                    if (attack) {
                        enemyRookAttacks_forCheckCalc.push(attack);
                    }
                    break;
                case (2):
                    attack = chessMoves::generateKnightMovesNoPinCheckTeleport(piece, friendly_inner);
                    break;
                case (3):
                    attack = chessMoves::singleBihopMoveNoPinOrCheck_forLoop(piece, enemies_inner, friendly_inner);
                    if (attack) {
                        enemyBishopAttacks_forCheckCalc.push(attack);
                    }
                    break;
                case (4):
                    {
                    uint64_t attack_rooklike = chessMoves::singleRookMoveNoPinOrCheck_forLoop(piece, enemies_inner, friendly_inner);
                    uint64_t attack_bishoplike = chessMoves::singleBihopMoveNoPinOrCheck_forLoop(piece, enemies_inner, friendly_inner);
                    attack = attack_rooklike | attack_bishoplike;
                    if (attack) {
                        enemyQueenAttacks_forCheckCalc.push(QueenAttackDeconstruction{attack_rooklike, attack_bishoplike, piece});
                    }
                    break;
                    }
                case (5):
                    attack = chessMoves::dummyKingMoveGenerationNoTeleportation(piece, friendly_inner);
                    break;
            }
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

    FastStack<uint64_t, 2> checkingAttacks = chessMoves::calculateChecks(isWhiteTurn, board, enemyRookAttacks_forCheckCalc, enemyBishopAttacks_forCheckCalc, enemyQueenAttacks_forCheckCalc);

    // for (auto check : checkingAttacks) {
    //     std::println("checks for this move");
    //     helpers::printBitboard(check);
    // }

    board_state::PawnState pawnState = board.mapBoardToPawnState();

    for (uint8_t i = 0; i < 6; ++ i) {
        // 0 pawns, 1 rooks, 2 knights, 3 bishops, 4 queens, 5 king
        FastStack pieceStack {seperateBitboardFastStackReturn<10>(board.getPiecesByColorConst(isWhiteTurn)[i])};
        for (uint64_t piece : pieceStack) {
            std::tuple<uint64_t, std::optional<uint64_t>, std::optional<pieceMovement>> pawnRet;
            std::pair<uint64_t, std::pair<std::optional<pieceMovement>, std::optional<pieceMovement>>> kingRet;
            uint64_t normalRet;
            switch (i) {
                case (0):
                    pawnRet = chessMoves::singlePawnMove(piece, enemy_pieces, friendly_pieces, pawnState, enemy_pawns, pinLines, checkingAttacks);
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
                addAttacksToStack218<4>(piece, std::get<uint64_t>(pawnRet), PieceType{i}, moveStack);
                if(std::get<std::optional<uint64_t>>(pawnRet).has_value()) {
                    // std::cout << "pawn promotion detected!!" << std::endl;

                    for (uint8_t j = 1; j < 5; ++j) {
                        auto promotionMove = pieceMovement{piece, std::get<std::optional<uint64_t>>(pawnRet).value(), PieceType::Pawn, PieceType{j}, true};
                        // promotionMove.printThis();
                        moveStack.push(promotionMove);
                    }
                }
                if(std::get<std::optional<pieceMovement>>(pawnRet)) {
                    moveStack.push(std::get<std::optional<pieceMovement>>(pawnRet).value());
                }
            } else if (i > 0 && i < 5) {
                addAttacksToStack218<27>(piece, normalRet, PieceType{i}, moveStack);
            } else {
                addAttacksToStack218<8>(piece, kingRet.first, PieceType{i}, moveStack);
                
                const auto& pair_opts = kingRet.second;
                if(pair_opts.first.has_value()) {
                    moveStack.push(pair_opts.first.value());
                }

                if(pair_opts.second.has_value()) {
                    moveStack.push(pair_opts.second.value());
                }
            }
        }
    }

    return moveStack;
}
}
