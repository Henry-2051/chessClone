#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <print>
#include <stdexcept>
#include <sys/types.h>
#include <utility>
#include "stackStack.hpp"
#include "boardState.hpp"
#include "chessBoard.h"

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

static constexpr uint64_t black_king_castling[2] = {
    0x50,
    0x14
};

static constexpr uint64_t black_king_squares_to_check_empty_before_castling[2] = {
    0x60,
    0xe
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

enum ScanState
{
    hitFriendlyPiece,
    hitEnemyKing,
    continueIteration
};


inline void printBitboard(uint64_t bitboard) {
    for (int rank = 7; rank >= 0; --rank) {     // ranks from 7 (top) down to 0 (bottom)
        for (int file = 0; file <= 7; ++file) { // files from 0 (left) to 7 (right)
            int squareIndex = rank * 8 + file; // bit index from 0 (LSB) to 63 (MSB)
            // Use mask to check bit; bit 0 at LSB
            uint64_t mask = 1ULL << squareIndex;
            std::cout << ((bitboard & mask) ? '#' : '.') << ' ';
        }
        std::cout << "\n";
    }
}


template<size_t N>
inline void addAttacksToStack218(uint64_t piece, uint64_t attacked_squares, PieceType typeofPiece, stackStack218& moveStack) {
    if (std::popcount(attacked_squares) > N) {
        std::println("popcount : {}", std::popcount(attacked_squares));
        std::println("type of piece: {}", getPieceTypeString(typeofPiece));
        printBitboard(attacked_squares);
        throw std::logic_error("trying to seperate a bitboard with more items than the array size");
    }
    auto [moves, num_moves] = seperateBitboard<N>(attacked_squares);
    for (size_t i = 0; i < num_moves; ++i) {
        moveStack.push({moves[i] | piece, 0, typeofPiece, PieceType::King, false});
    }
}



// we need to handle pawn promotion will being able to access attacked squares, we can do this by either moving the pawn promotion logic to the caller or returning an optional value of std::array<pieceMovement, 4> inside a pair with the attacked squares
inline void 
blackPawnMove(uint64_t black_pawn, uint64_t enemies, uint64_t friendly, uint8_t pawnState, stackStack218& moveStack, const FastStack<uint64_t, 13>& pinLines)
{
    uint64_t empty_space = ~(enemies | friendly);
    uint64_t front_pawn_row = 0xff00;

    int pawn_place = __builtin_ctzll(black_pawn);
    int pawn_file = pawn_place % 8;

    bool can_have_right_bitshift_by_1 = pawn_file != 0;
    bool can_have_left_bitshift_by_1 = pawn_file != 7;

    uint64_t attacked_squares =
       ((((black_pawn << 8) & empty_space) | 
        ((black_pawn  & front_pawn_row) << 16)) & empty_space) |
        (can_have_right_bitshift_by_1 ? ((black_pawn  << 7) & enemies) : 0ULL) |
        (can_have_left_bitshift_by_1 ? ((black_pawn  << 9) & enemies) : 0ULL);

    uint64_t back_row = static_cast<uint64_t>(0xff) << 56;

    for (uint64_t pin_line : pinLines) {
        if (black_pawn & pin_line) {
            attacked_squares &= pin_line;
        }
    }

    while(attacked_squares!= 0) {
        uint64_t destination_square = 1ULL << __builtin_ctzll(attacked_squares);
        attacked_squares ^= destination_square;
        if (destination_square & back_row) {
            for (int j = 1; j < 5; ++j) {
                moveStack.push({black_pawn, destination_square, PieceType::Pawn, PieceType(j), true});
            }
        } else {
            moveStack.push({black_pawn| destination_square, 0, PieceType::Pawn, PieceType::King, false});
        }
    }
}


inline void 
whitePawnMove(uint64_t white_pawn, uint64_t enemies, uint64_t friendly, uint8_t pawnState, stackStack218& moveStack, const FastStack<uint64_t, 13>& pinLines)
{
    uint64_t empty_space = ~(enemies | friendly);
    uint64_t front_pawn_row = static_cast<uint64_t>(0xff) << 48;

    int pawn_place = __builtin_ctzll(white_pawn);
    int pawn_file = pawn_place % 8;

    bool can_have_right_bitshift_by_1 = pawn_file != 0;
    bool can_have_left_bitshift_by_1 = pawn_file != 7;

    uint64_t white_pawn_move =
        (((white_pawn >> 8) & empty_space) |
        (((white_pawn & front_pawn_row) >> 16) & empty_space)) |
        (can_have_left_bitshift_by_1 ? ((white_pawn >> 7) & enemies) : 0ULL) |
        (can_have_right_bitshift_by_1 ? ((white_pawn >> 9) & enemies) : 0ULL);
    uint64_t back_row = static_cast<uint64_t>(0xff);

    for (uint64_t pin_line : pinLines) {
        if (white_pawn & pin_line) {
            white_pawn_move &= pin_line;
        }
    }
    
    // cant use seperate bitboard since we have to deal with pawn promotion, its easier to do it in one pass
    while(white_pawn_move != 0) {
        uint64_t destination_square = 1ULL << __builtin_ctzll(white_pawn_move);
        white_pawn_move ^= destination_square;
        if (destination_square & back_row) {
            for (int j = 1; j < 5; ++j) {
                moveStack.push({white_pawn, destination_square, PieceType::Pawn, PieceType(j), true});
            }
        } else {
            moveStack.push({white_pawn | destination_square, 0, PieceType::Pawn, PieceType::King, false});
        }
    }
}

inline void 
whitePawnMoveEPP(uint64_t white_pawn, uint64_t enemies, uint64_t friendly, uint8_t pawnState, uint64_t enemy_pawns, stackStack218& moveStack, const FastStack<uint64_t, 13>& pinLines) {
    whitePawnMove(white_pawn, enemies, friendly, pawnState, moveStack, pinLines);

    uint64_t shoulder = pawnState & board_state::PawnHasEnPassantRight ? white_pawn >> 1 : white_pawn << 1;
    uint64_t capture_position = shoulder >> 8;

    if (enemy_pawns & shoulder) {
        // we cant capture en passant if our pawn is pinned
        for (uint64_t pin_line : pinLines) {
            if (white_pawn & pin_line) {
                capture_position &= pin_line;
            }
        }
        pieceMovement eppMove = {white_pawn | capture_position | shoulder, shoulder, PieceType::Pawn, PieceType::Pawn, true};
        moveStack.push(eppMove);
    }
}

inline void 
blackPawnMoveEPP(uint64_t black_pawns, uint64_t enemies, uint64_t friendly, uint8_t pawnState, uint64_t enemy_pawns, stackStack218& moveStack, const FastStack<uint64_t, 13>& pinLines) {
    blackPawnMove(black_pawns, enemies, friendly, pawnState, moveStack, pinLines);

    uint64_t shoulder = pawnState & board_state::PawnHasEnPassantRight ? black_pawns >> 1 : black_pawns << 1;
    uint64_t capture_position = shoulder << 8;

    if (enemy_pawns & shoulder) {
        // we cant capture en passant if our pawn is pinned
        for (uint64_t pin_line : pinLines) {
            if (black_pawns & pin_line) {
                capture_position &= pin_line;
            }
        }
        moveStack.push({black_pawns | capture_position | shoulder, shoulder, PieceType::Pawn, PieceType::Pawn, true});
    }
}

inline uint64_t scanPinRay(uint64_t pieces_of_the_same_color_as_the_attacker, uint64_t opposite_pieces, uint64_t opposite_king, int r, int f, int df, int dr) {
    uint64_t trial_attack = 1ULL << (f + 8 * r); // this line needs some explanation, the piece isnt actually attacking this square but it is the square that the pinning piece
                                                 // sits in, therefore when we apply the pin by anding the pinned pieces attack and the pin we must allow the pinned piece to capture the pinner 
    
    bool hit_enemy_piece = false; // we cant have a pin if there are 2 pieces inbetween our attacking piece and the enemy king
    r += dr;
    f += df;
    while (r >= 0 && r < 8 && f >= 0 && f < 8) {
        uint64_t looking_at = 1ULL << (f + 8 * r);
        if (looking_at & opposite_king) {
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
inline uint64_t output_pinned_squares_for_piece(uint64_t pinning_piece, uint64_t same_color_pieces, uint64_t opposite_pieces, uint64_t opposite_king) {
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

// the bool represents whether the return contains an enpassant capture
inline void 
singlePawnMove(uint64_t attacking_pawn, uint64_t enemies, uint64_t friendly, uint8_t pawnState, uint64_t enemy_pawns, stackStack218& moveStack, FastStack<uint64_t, 13> pinLines) {
    uint8_t relevantPawnState = pawnState & (board_state::PawnWhiteTurn | board_state::PawnHasEnPassant);

    if (relevantPawnState == (board_state::PawnWhiteTurn | board_state::PawnHasEnPassant)) 
    {
        innerMachinations::whitePawnMoveEPP(attacking_pawn, enemies, friendly, pawnState, enemy_pawns, moveStack, pinLines);
    } 
    else if (relevantPawnState == board_state::PawnWhiteTurn) 
    {
        innerMachinations::whitePawnMove(attacking_pawn, enemies, friendly, pawnState, moveStack, pinLines);
    } 
    else if (relevantPawnState == board_state::PawnHasEnPassant) 
    {
        innerMachinations::blackPawnMoveEPP(attacking_pawn, enemies, friendly, pawnState, enemy_pawns, moveStack, pinLines);
    } 
    else 
    {
        innerMachinations::blackPawnMove(attacking_pawn, enemies, friendly, pawnState, moveStack, pinLines);
    };
}

inline uint64_t attack_with_increment_sliding_piece(uint64_t enemies, uint64_t friendly, std::pair<int, int> starting_rf, std::pair<int, int> increment_rf) {
    uint64_t attacked_squares = 0;

    auto [r, f] = starting_rf;
    auto [dr, df] = increment_rf;

    r += dr;
    f += df;

    while (r < 8 && r >= 0 && f < 8 && f >= 0) {
        uint64_t looking_at = 1ULL << (f + 8 * r);

        if (looking_at & enemies) {
            attacked_squares |= looking_at;
            return attacked_squares;
        } 
        else if (looking_at & friendly) {
            return attacked_squares;
        }

        attacked_squares |= looking_at;

        r += dr;
        f += df;
    };
    return attacked_squares;
};

inline FastStack<uint64_t, 13> calculate_pin_lines(const chessBoard& board) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();

    uint64_t friendly_king = isWhiteTurn ? board.m_white_king : board.m_black_king;

    uint64_t enemy_rooks = !isWhiteTurn ? board.m_white_rooks : board.m_black_rooks;
    uint64_t enemy_bishops = !isWhiteTurn ? board.m_white_bishops : board.m_black_bishops;
    uint64_t enemy_queens = !isWhiteTurn ? board.m_white_queens : board.m_black_queens;

    FastStack rookStack{seperateBitboard<10>(enemy_rooks)};
    FastStack bishopStack {seperateBitboard<10>(enemy_bishops)};
    FastStack queenStack {seperateBitboard<9>(enemy_queens)};
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

inline void singleRookMove(uint64_t rook, const chessBoard& board, stackStack218& moveStack, const FastStack<uint64_t, 13>& pinLines) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();


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

    for (uint64_t pin_line : pinLines) {
        if (rook & pin_line) {
            std::println("rook pinned!");
            attacked_squares &= pin_line;
        }
    }

    innerMachinations::addAttacksToStack218<14>(rook, attacked_squares, PieceType::Rook, moveStack);
}

inline void singleBishopMove(uint64_t bishop, const chessBoard& board, stackStack218& moveStack, const FastStack<uint64_t, 13>& pinLines) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();
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

    for (uint64_t pin_line : pinLines) {
        if (bishop & pin_line) {
            std::println("bishop pinned!");
            attacked_squares &= pin_line;
        }
    }

    innerMachinations::addAttacksToStack218<14>(bishop, attacked_squares, PieceType::Bishop, moveStack);
}

inline void singleQueenMove(uint64_t queen, const chessBoard& board, stackStack218& moveStack, const FastStack<uint64_t, 13>& pinLines) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();
    int queen_place = __builtin_ctzll(queen);

    int queen_rank = queen_place / 8;
    int queen_file = queen_place % 8;

    // if (std::popcount(queen) != 1) {
    //     throw std::logic_error(std::format("Error single queen has a pop count of {}", std::popcount(queen)));
    // }


    uint64_t attacked_squares = 0;

    for (const auto &d : innerMachinations::how_the_queen_moves) {
        attacked_squares |= attack_with_increment_sliding_piece(
            enemies, friendly,
            {queen_rank, queen_file},
            {d[0], d[1]}
        );
    }

    for (uint64_t pin_line : pinLines) {
        if (queen & pin_line) {
            std::println("Queen pinned!");
            attacked_squares &= pin_line;
        }
    }

    innerMachinations::addAttacksToStack218<28>(queen, attacked_squares, PieceType::Queen, moveStack);
}

// must also take into account the enemies attacked squares so we must either pass in an array of attack lines or a single bitboard of all the attacked squares, 
// passing the array seems like the better option since 
inline void singleKingMove(uint64_t king, const chessBoard& board, stackStack218& moveStack) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    uint64_t enemies = isWhiteTurn ? board.blackPieces() : board.whitePieces();
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();

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
        attacked_squares |= attacked_square & (~friendly); 
    }

    uint64_t all_pieces = enemies | friendly;

    // another chess rule, we cant castle through check, therefore we must also include the enemy attacks 
    // can castle left (board state)
    if (!(board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsLeft : board_state::BlackLostCastlingRightsLeft))) {
        uint64_t black_king_starting_square = 0x10;
        uint64_t black_left_rook_castling_square = 0x1;
        uint64_t black_squares_to_check_empty_left = 0xe;
        // check all the pieces are in the right places, redundant by design with the above check
        if ((king & (isWhiteTurn ? (black_king_starting_square << 56) : black_king_starting_square)) && 
           ((isWhiteTurn ? board.m_white_rooks : board.m_black_rooks) & (isWhiteTurn ? black_left_rook_castling_square << 56 : black_left_rook_castling_square)) &&
           !((isWhiteTurn ? black_squares_to_check_empty_left << 56 : black_squares_to_check_empty_left) & all_pieces))
        {
            pieceMovement castlingLeft = isWhiteTurn ? 
                pieceMovement{(black_king_starting_square >> 2 | black_king_starting_square) << 56, (black_left_rook_castling_square | black_left_rook_castling_square << 3) << 56, PieceType::King, PieceType::Rook, true} : 
                pieceMovement{black_king_starting_square >> 2 | black_king_starting_square, black_left_rook_castling_square | black_left_rook_castling_square << 3, PieceType::King, PieceType::Rook, true};

            moveStack.push(castlingLeft);
        }
    }

    // can castle right (board state)
    if (!(board.m_board_state & (isWhiteTurn ? board_state::WhiteLostCastlingRightsRight : board_state::BlacklostCastlingRightsRight))) {
        uint64_t black_king_starting_square = 0x10;
        uint64_t black_squares_to_check_empty_right = 0x60;
        uint64_t black_right_rook_castling_square = 0x80;

        // check the squares have what they should, redundant by design with the above check
        if((king & (isWhiteTurn ? (black_king_starting_square << 56) : black_king_starting_square)) &&
          ((isWhiteTurn ? board.m_white_rooks : board.m_black_rooks) & (isWhiteTurn ? black_right_rook_castling_square << 56 : black_right_rook_castling_square)) &&
          !((isWhiteTurn ? black_squares_to_check_empty_right << 56 : black_squares_to_check_empty_right) & all_pieces)) 
        {
            pieceMovement castlingRight = isWhiteTurn ?
                pieceMovement{(black_king_starting_square << 2 | black_king_starting_square) << 56, (black_right_rook_castling_square | black_right_rook_castling_square >> 2) << 56, PieceType::King, PieceType::Rook, true} :
                pieceMovement{black_king_starting_square << 2 | black_king_starting_square, black_right_rook_castling_square | black_right_rook_castling_square >> 2, PieceType::King, PieceType::Rook, true};

            moveStack.push(castlingRight);
        }
    }

    innerMachinations::addAttacksToStack218<10>(king, attacked_squares, PieceType::King, moveStack);
}


inline void singleKnightMove(uint64_t knight, const chessBoard& board, stackStack218& moveStack, const FastStack<uint64_t, 13>& pinLines) {
    bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    
    uint64_t friendly = isWhiteTurn ? board.whitePieces() : board.blackPieces();

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
        attacked_squares |= attacked_square & (~friendly); 
    }

    for (uint64_t pin_line : pinLines) {
        if (attacked_squares & pin_line) {
            attacked_squares &= pin_line;
        }
    }

    innerMachinations::addAttacksToStack218<8>(knight, attacked_squares, PieceType::Knight, moveStack);
}
    
}
