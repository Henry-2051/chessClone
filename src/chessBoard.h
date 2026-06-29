#include <cstdint>
#include <vector>
#include "boardState.hpp"

#pragma once

struct chessBoard {
    // change this to an std::array<uint64_t, 12> and then the indexing wont be Undefined
    uint64_t m_pawn_bitshift = 40;
    uint64_t m_piece_bitshift = 56;
    uint64_t m_black_pawns = 0xff00;
    uint64_t m_black_rooks = 0x81;
    uint64_t m_black_knights = 0x42;
    uint64_t m_black_bishops = 0x24;
    uint64_t m_black_queens = 0x8;
    uint64_t m_black_king = 0x10;

    uint64_t m_white_pawns = m_black_pawns << m_pawn_bitshift;
    uint64_t m_white_rooks = m_black_rooks << m_piece_bitshift;
    uint64_t m_white_knights = m_black_knights << m_piece_bitshift;
    uint64_t m_white_bishops = m_black_bishops << m_piece_bitshift;
    uint64_t m_white_queens = m_black_queens << m_piece_bitshift;
    uint64_t m_white_king = m_black_king << m_piece_bitshift;
    
    uint8_t m_board_state = board_state::WhiteTurn;
    uint32_t m_turn {};
    uint32_t m_last_generated_moves{};

    using annoying_return_type = std::vector<std::vector<std::pair<uint32_t, uint32_t>>>;
    
    uint64_t whitePieces() const;

    uint64_t blackPieces() const;

    uint64_t* getPiecesByColor(bool isWhiteTurn);

    const uint64_t* getPiecesByColorConst(bool isWhiteTurn) const;

    annoying_return_type piecePositions() const;

};
