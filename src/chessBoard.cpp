#include "chessBoard.h"
#include "helpers.hpp"

uint64_t chessBoard::whitePieces() const {
    return m_white_knights | m_white_king | m_white_queens | m_white_bishops | m_white_pawns | m_white_rooks;
}

uint64_t chessBoard::blackPieces() const {
    return m_black_knights | m_black_king | m_black_bishops | m_black_pawns | m_black_queens | m_black_rooks;
}

uint64_t* chessBoard::getPiecesByColor(bool isWhiteTurn) {
    // todo change representation to std::array such that we dont have Undefined behaviour 
    if (isWhiteTurn) {
        return &m_white_pawns;
    } else {
        return &m_black_pawns;
    }
}

const uint64_t* chessBoard::getPiecesByColorConst(bool isWhiteTurn) const {
    // todo change representation to std::array such that we dont have Undefined behaviour 
    if (isWhiteTurn) {
        return &m_white_pawns;
    } else {
        return &m_black_pawns;
    }
}

using annoying_return_type = std::vector<std::vector<std::pair<uint32_t, uint32_t>>>;

annoying_return_type chessBoard::piecePositions() const {
    annoying_return_type result = {};
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_king)));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_queens)));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_bishops)));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_knights)));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_rooks)));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_pawns)));

    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_king)));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_queens)));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_bishops)));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_knights)));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_rooks)));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_pawns)));
    return result;
}
