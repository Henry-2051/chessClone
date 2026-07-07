#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "boardState.hpp"
#include "stackStack.hpp"

#pragma once

struct chessBoard {
    // change this to an std::array<uint64_t, 12> and then the indexing wont be Undefined
    uint64_t m_black_pawns = 0;
    uint64_t m_black_rooks = 0;
    uint64_t m_black_knights = 0;
    uint64_t m_black_bishops = 0;
    uint64_t m_black_queens = 0;
    uint64_t m_black_king = 0;

    uint64_t m_white_pawns = 0;
    uint64_t m_white_rooks = 0;
    uint64_t m_white_knights = 0;
    uint64_t m_white_bishops = 0;
    uint64_t m_white_queens = 0;
    uint64_t m_white_king = 0;
    
    uint8_t m_board_state = board_state::WhiteTurn;

    int8_t enPassantState = -1; 

    size_t halfMoveClock {0};
    size_t moveNumber {0};

    chessBoard(std::string_view fenString);

    using annoying_return_type = std::vector<std::vector<std::pair<uint32_t, uint32_t>>>;
    
    uint64_t whitePieces() const;

    uint64_t blackPieces() const;

    uint64_t* getPiecesByColor(bool isWhiteTurn);

    const uint64_t* getPiecesByColorConst(bool isWhiteTurn) const;

    annoying_return_type piecePositions() const;

    void readFenAndUpdate(std::string_view inputFen);

    board_state::PawnState mapBoardToPawnState();
    
    private:
    void changeRank(std::string_view fenRank, size_t rankNum);
    void updateBoardState(std::string_view term, size_t termNumber);
    
};
