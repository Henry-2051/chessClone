#include "chessBoard.h"
#include "helpers.hpp"
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>


uint64_t chessBoard::whitePieces() const {
  return bitboards[PieceType::Rook + 6] | bitboards[PieceType::Pawn + 6] |
         bitboards[PieceType::Knight + 6] | bitboards[PieceType::Bishop + 6] |
         bitboards[PieceType::Queen + 6] | bitboards[PieceType::King + 6];
}

uint64_t chessBoard::blackPieces() const {
  return bitboards[PieceType::Bishop] | bitboards[PieceType::Pawn] |
         bitboards[PieceType::Rook] | bitboards[PieceType::Knight] |
         bitboards[PieceType::Queen] | bitboards[PieceType::King];
}

uint64_t* chessBoard::getPiecesByColor(bool isWhiteTurn) {
    if (isWhiteTurn) {
        return &bitboards[0] + 6;
    } else {
        return bitboards;
    }
}

const uint64_t* chessBoard::getPiecesByColorConst(bool isWhiteTurn) const {
    if (isWhiteTurn) {
        return &bitboards[0] + 6;
    } else {
        return bitboards;
    }
}

using annoying_return_type = std::vector<std::vector<std::pair<uint32_t, uint32_t>>>;

annoying_return_type chessBoard::piecePositions() const {
    annoying_return_type result = {};
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_king)));
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_queens)));
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_bishops)));
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_knights)));
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_rooks)));
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_white_pawns)));
    //
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_king)));
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_queens)));
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_bishops)));
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_knights)));
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_rooks)));
    // result.push_back(helpers::getChessCoordinates(helpers::getOnes(m_black_pawns)));

    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::King + 6])));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::Queen + 6])));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::Bishop + 6])));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::Knight + 6])));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::Rook + 6])));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::Pawn + 6])));

    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::King])));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::Queen])));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::Bishop])));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::Knight])));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::Rook])));
    result.push_back(helpers::getChessCoordinates(helpers::getOnes(bitboards[PieceType::Pawn])));
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
            } else if (term[0] == 'b') {
                this->m_board_state ^= this->m_board_state & board_state::WhiteTurn;
            } else {
                throw std::runtime_error("unhanded turn character in fen");
            }
            break;
        }
        case (2) : {
            assert(term.size() <= 4);
            for (char c : term) {
                switch (c) {
                    case ('K'):
                        m_board_state ^= board_state::WhiteLostCastlingRightsRight;
                        break;
                    case ('Q'):
                        m_board_state ^= board_state::WhiteLostCastlingRightsLeft;
                        break;
                    case ('k'):
                        m_board_state ^= board_state::BlacklostCastlingRightsRight;
                        break;
                    case ('q'):
                        m_board_state ^= board_state::BlackLostCastlingRightsLeft;
                        break;
                    case ('-'):
                        break;
                    default:
                        throw std::runtime_error("unhanded castling character in fen string");
                }
            }
            break;
        }
        case (3) : {
            std::map<char, int8_t> eppMap {
                {'a', 0},
                {'b', 1},
                {'c', 2},
                {'d', 3},
                {'e', 4},
                {'f', 5},
                {'g', 6},
                {'h', 7},
                {'8', 0},
                {'7', 8},
                {'6', 16},
                {'5', 24},
                {'4', 32},
                {'3', 40},
                {'2', 48},
                {'1', 56}
            };
            int8_t temp = 0;
            for (char c : term) {
                if (c == '-') {
                    return;
                }
                if (eppMap.find(c) == eppMap.end()) {
                    throw std::runtime_error("unhanded character in epp term");
                }
                temp += eppMap.find(c)->second;
            }
            this->enPassantState = temp;
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
    m_board_state = board_state::BlacklostCastlingRightsRight | board_state::WhiteLostCastlingRightsLeft | board_state::BlackLostCastlingRightsLeft | board_state::WhiteLostCastlingRightsRight;
    FastStack<size_t, 10> termSeperations {};
    FastStack<size_t, 10> slashPlaces {};

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
                // std::println("rank {} : {}", rankCounter, individualRankSubstring);
                changeRank(individualRankSubstring, rankCounter);
            } else {
                size_t previous_place = termSeperations.isEmpty() ? 0uz : termSeperations.peek();
                std::string_view termString {inputFen.substr(previous_place, counter - previous_place -1)};
                // std::println("term : {}", termString);
                updateBoardState(termString, termCounter);
            }

            termSeperations.push(counter);
            termCounter ++;

        } else if (c == '/' && termSeperations.isEmpty()) {
            size_t previous_place = slashPlaces.isEmpty() ? 0uz : slashPlaces.peek();
            auto individualRankSubstring = inputFen.substr(previous_place, counter - previous_place -1);
            // std::println("rank {} : {}", rankCounter, individualRankSubstring);
            changeRank(individualRankSubstring, rankCounter);
            slashPlaces.push(counter);
            rankCounter ++;
        } else if (c == '.') {
            size_t previous_place = termSeperations.peek();
            std::string_view termString {inputFen.substr(previous_place, counter - previous_place -1)};
            // std::println("term : {}", termString);
            updateBoardState(termString, termCounter);
            termSeperations.push(counter);
            break;
        }
        counter ++;
    }
}

chessBoard& chessBoard::applyMoveImpure(const pieceMovement& move) {
    uint64_t* whitePieces = getPiecesByColor(true);
    uint64_t* blackPieces = getPiecesByColor(false);

    if (move.movement1WhiteBB != PieceType::NotAPiece) {
        whitePieces[move.movement1WhiteBB] ^= move.movement;
    }
    if (move.movement1BlackBB != PieceType::NotAPiece) {
        blackPieces[move.movement1BlackBB] ^= move.movement;
    }
    if (move.movement2WhiteBB != PieceType::NotAPiece) {
        whitePieces[move.movement2WhiteBB] ^= move.secondMovement;
    }
    if (move.movement2BlackBB != PieceType::NotAPiece) {
        blackPieces[move.movement2BlackBB] ^= move.secondMovement;
    }

    m_board_state ^= move.boardStateChange;

    // move pawn in special place 2 spaces - > move with en passant value
    //
    // apply move (which has en passant state) to board without en passant state                 -> board has en passant state    state overrides null state
    // apply same move again to board with same en passant state as board                        -> board doesnt have en passant  if xor between states gets us to null state then keep the null state
    // apply differnt move (which doesnt have en passant) to board from position with en passant -> board doesnt have en passant  move with null state creates board with null state
    // apply different move (pawn moves 2 spaces in a specific spot, so has en passant) to board -> board has en passant          if xor doesnt get null state then overrite the boards state
    
    if (enPassantState == -1) {
        enPassantState = move.enPassantState;
    } else if ((enPassantState ^ move.enPassantState) == -1) {
        enPassantState = -1;
    } else if (enPassantState != -1 && move.enPassantState == -1) {
        enPassantState = -1;
    } else if (enPassantState != -1 && move.enPassantState != -1) {
        enPassantState = move.enPassantState;
    } else {
        throw std::logic_error("error in en passant logic, fallen through if block, re examine logic");
    }

    return *this;
}

chessBoard chessBoard::applyMovePure(const pieceMovement& move) const {
    chessBoard boardCopy = *this;
    boardCopy.applyMoveImpure(move);
    return boardCopy;
}

PieceType chessBoard::figureOutTypeOfPieceOnSquare(uint64_t square, bool checkWhiteColor) const {
    // there should only be 1 bitboard that satisfies the condition in the loop, this should be simd able 
    if (std::popcount(square) != 1) {
        std::println("failed with invalid argument, argument passed : ");
        helpers::printBitboard(square);
        throw std::runtime_error("invalid argument");
    }

    const uint64_t* bbPtr = this->getPiecesByColorConst(checkWhiteColor);

    uint8_t pieceType =   (bbPtr[PieceType::Pawn]   & square ? PieceType::Pawn+1   : 0)
                        | (bbPtr[PieceType::Bishop] & square ? PieceType::Bishop+1 : 0)
                        | (bbPtr[PieceType::Rook]   & square ? PieceType::Rook+1   : 0)
                        | (bbPtr[PieceType::Knight] & square ? PieceType::Knight+1 : 0)
                        | (bbPtr[PieceType::Queen]  & square ? PieceType::Queen+1  : 0)
                        | (bbPtr[PieceType::King]   & square ? PieceType::King+1   : 0);

    if (pieceType == 0) {
        return PieceType::NotAPiece;
    }
    pieceType --;

    return PieceType(pieceType);
}

chessBoard::chessBoard(std::string_view fenString) {
    readFenAndUpdate(fenString);
}
