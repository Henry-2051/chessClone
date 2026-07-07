#include "chessBoard.h"
#include "helpers.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>

struct fenIntermediate {
    bool isWhiteTurn;
    uint8_t castlingRights = 0b01111000;
    uint8_t enPassantRights = 0b00000000;
    size_t halfMoveClock {0};
    size_t fullMoveCounter {0};
};

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

// starting fen string for reference
// "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
void chessBoard::changeRank(std::string_view fenRank, size_t rankNum) {
    assert(rankNum < 8);
    std::map<char, int> bitBoardIndexMap = {
        {'p', 0},
        {'r', 1},
        {'n', 2},
        {'b', 3},
        {'q', 4},
        {'k', 5},
        {'P', 6},
        {'R', 7},
        {'N', 8},
        {'B', 9},
        {'Q', 10},
        {'K', 11}
    };

    auto fileNum = 0uz;
    for (char c : fenRank) {
        if (c - '0' >= 1 && c - '0' <= 8) {
            fileNum += (size_t)(c - '0');
        } else {
            if (auto b_idx_iter {bitBoardIndexMap.find(c)}; b_idx_iter != bitBoardIndexMap.end()) {
                // the argument name for getPiecesByColor is 'isWhiteTurn' so we are indexing into the black pieces
                // which are contiguous with the white pieces
                this->getPiecesByColor(false)[b_idx_iter->second] |= 1ULL << (rankNum * 8 + fileNum);
                fileNum ++;
            } else {
                throw std::invalid_argument(std::format("error fen string invalid, {} doenst correspond to a piece", c));
            }
        }

        assert(fileNum <= 8);
        if (fileNum == 8) {
            return;
        }
    }
}

void chessBoard::updateBoardState(std::string_view term, size_t termNumber) {
    switch (termNumber) {
        case (1): {
            assert(term.size() == 1);
            if(term[0] == 'w') {
                this->m_board_state ^= ~(this->m_board_state & board_state::WhiteTurn) & board_state::WhiteTurn;
            } else {
                this->m_board_state ^= this->m_board_state & board_state::WhiteTurn;
            }
            break;
        }
        case (2) : {
            uint8_t castling_mask = board_state::WhiteLostCastlingRightsLeft | board_state::WhiteLostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft | board_state::BlacklostCastlingRightsRight;
            uint8_t castling = board_state::WhiteLostCastlingRightsLeft | board_state::WhiteLostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft | board_state::BlacklostCastlingRightsRight;
            assert(term.size() <= 4);
            for (char c : term) {
                switch (c) {
                    case ('K'):
                        castling ^= board_state::WhiteLostCastlingRightsRight;
                        break;
                    case ('Q'):
                        castling ^= board_state::WhiteLostCastlingRightsLeft;
                        break;
                    case ('k'):
                        castling ^= board_state::BlacklostCastlingRightsRight;
                        break;
                    case ('q'):
                        castling ^= board_state::BlackLostCastlingRightsLeft;
                        break;
                    default:
                        throw std::runtime_error("unhanded castling character in fen string");
                }
            }
            /// I love bit fields so much
            this->m_board_state ^= ~(this->m_board_state & castling) & castling_mask;
            break;
        }
        case (3) : {
            // oh no en passant is actually bugged
            // we are using 2 bits to to describe en passant we actually need 6
            break;
        }
        case (4) : {
            this->halfMoveClock = std::stoull(term.data());
            break;
        }
        case (5) : {
            this->moveNumber = std::stoull(term.data());
            break;
        }
    }
}

void chessBoard::readFenAndUpdate(std::string_view inputFen) {
    FastStack<size_t, 10> termSeperations {};
    FastStack<size_t, 10> slashPlaces {};

    fenIntermediate iState;

    auto termCounter {0uz};
    auto rankCounter {0uz};
    // 0 for piece positions
    // 1 for color to move
    // 2 for castling rights
    // 3 for en passant rights
    // 4 for the halfmove clock
    // 5 for the fullmove counter
    auto counter {1uz};
    for (char c : inputFen) {
        if (c == ' ') {
            if (termCounter == 0) {
                size_t previous_rank_place = slashPlaces.peek();
                auto individualRankSubstring = inputFen.substr(previous_rank_place, counter - previous_rank_place-1);
                std::println("rank {} : {}", rankCounter, individualRankSubstring);
                changeRank(individualRankSubstring, rankCounter);
            } else {
                size_t previous_place = termSeperations.isEmpty() ? 0uz : termSeperations.peek();
                std::string_view termString {inputFen.substr(previous_place, counter - previous_place -1)};
                std::println("term : {}", termString);
                updateBoardState(termString, termCounter);
            }

            termSeperations.push(counter);
            termCounter ++;

        } else if (c == '/' && termSeperations.isEmpty()) {
            size_t previous_place = slashPlaces.isEmpty() ? 0uz : slashPlaces.peek();
            auto individualRankSubstring = inputFen.substr(previous_place, counter - previous_place -1);
            std::println("rank {} : {}", rankCounter, individualRankSubstring);
            changeRank(individualRankSubstring, rankCounter);
            slashPlaces.push(counter);
            rankCounter ++;
        } else if (c == '.') {
            size_t previous_place = termSeperations.peek();
            std::string_view termString {inputFen.substr(previous_place, counter - previous_place -1)};
            std::println("term : {}", termString);
            updateBoardState(termString, termCounter);
            termSeperations.push(counter);
            break;
        }
        counter ++;
    }
}

chessBoard::chessBoard(std::string_view fenString) {
    readFenAndUpdate(fenString);
}
