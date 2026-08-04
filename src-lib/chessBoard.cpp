#include "chessBoard.h"
#include "helpers.hpp"
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <print>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>



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

std::string chessBoard::uciStringMove(pieceMovement move) const {
    bool whiteMoving = move.movement1WhiteBB != PieceType::NotAPiece;
    
    std::map<PieceType, char> promotablePieceMap{
        {PieceType::Rook,   'r'},
        {PieceType::Knight, 'n'},
        {PieceType::Bishop, 'b'},
        {PieceType::Queen,  'q'}
    };

    std::stringstream uciMoveOutputSS;

    auto addFileRankInfo = [](int originalSquare, int squareMovedTo, std::stringstream& ss) {
        std::map<int, char> fileMap {
            {0, 'a'},
            {1, 'b'},
            {2, 'c'},
            {3, 'd'},
            {4, 'e'},
            {5, 'f'},
            {6, 'g'},
            {7, 'h'}
        };

        std::map<int, char> rankMap {
            {0, '8'},
            {1, '7'},
            {2, '6'},
            {3, '5'},
            {4, '4'},
            {5, '3'},
            {6, '2'},
            {7, '1'}
        };
        int originalRank = originalSquare / 8;
        int originalFile = originalSquare % 8;

        int newRank = squareMovedTo / 8;
        int newFile = squareMovedTo % 8;

        if (!(fileMap.contains(originalFile) && fileMap.contains(newFile) && rankMap.contains(originalRank) && rankMap.contains(newRank))) {
            return;
        }

        ss << fileMap.find(originalFile)->second;
        ss << rankMap.find(originalRank)->second;

        ss << fileMap.find(newFile)->second;
        ss << rankMap.find(newRank)->second;
    };

    // pawn promotion
    if (std::popcount(move.movement) == 1) {
        int pawnPlace = std::countr_zero(move.movement);
        int promotionPlace = std::countr_zero(move.secondMovement);

        addFileRankInfo(pawnPlace, promotionPlace, uciMoveOutputSS);

        PieceType promotingPieceType {whiteMoving ? move.movement2WhiteBB : move.movement2BlackBB};

        if (!promotablePieceMap.contains(promotingPieceType))
            return "";

        uciMoveOutputSS << promotablePieceMap.find(promotingPieceType)->second;

        return uciMoveOutputSS.str();
    } 
    // not pawn promotion
    else {
        assert(std::popcount(move.movement) == 2);
        PieceType movingPT = whiteMoving ? move.movement1WhiteBB : move.movement1BlackBB;       

        uint64_t movingFromPieceTypeBB= slowPieceToBitboardConst(whiteMoving, movingPT);

        int pieceFromPlace = std::countr_zero(movingFromPieceTypeBB & move.movement);
        int pieceToPlace   = std::countr_zero(move.movement & ~(movingFromPieceTypeBB & move.movement));

        addFileRankInfo(pieceFromPlace, pieceToPlace, uciMoveOutputSS);

        return uciMoveOutputSS.str();
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

            termSeperations.pushVal(counter);
            termCounter ++;

        } else if (c == '/' && termSeperations.isEmpty()) {
            size_t previous_place = slashPlaces.isEmpty() ? 0uz : slashPlaces.peek();
            auto individualRankSubstring = inputFen.substr(previous_place, counter - previous_place -1);
            // std::println("rank {} : {}", rankCounter, individualRankSubstring);
            changeRank(individualRankSubstring, rankCounter);
            slashPlaces.pushVal(counter);
            rankCounter ++;
        } else if (c == '.') {
            size_t previous_place = termSeperations.peek();
            std::string_view termString {inputFen.substr(previous_place, counter - previous_place -1)};
            // std::println("term : {}", termString);
            updateBoardState(termString, termCounter);
            termSeperations.pushVal(counter);
            break;
        }
        counter ++;
    }
}


chessBoard chessBoard::applyMovePure(const pieceMovement& move) const {
    chessBoard boardCopy = *this;
    boardCopy.applyMoveImpure(move);
    return boardCopy;
}

chessBoard::chessBoard() {
    readFenAndUpdate("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

chessBoard::chessBoard(std::string_view fenString) {
    readFenAndUpdate(fenString);
}

std::optional<pieceMovement> chessBoard::genPartialMove(std::string_view uciMove) const {
    assert(uciMove.size() <= 5);

    bool isWhiteTurn = m_board_state & board_state::WhiteTurn;

    static const std::map<char, uint8_t> charToBoardPosition {
        {'a', 0},  {'b', 1},  {'c', 2},  {'d', 3},  {'e', 4},  {'f', 5},  {'g', 6}, {'h', 7},
        {'1', 56}, {'2', 48}, {'3', 40}, {'4', 32}, {'5', 24}, {'6', 16}, {'7', 8}, {'8', 0}
    };

    if (uciMove.size() < 4)
        return std::nullopt;

    uint64_t from_offset = charToBoardPosition.at(uciMove.data()[0]) + charToBoardPosition.at(uciMove.data()[1]);
    uint64_t to_offset   = charToBoardPosition.at(uciMove.data()[2]) + charToBoardPosition.at(uciMove.data()[3]);

    static const std::map<char, PieceType> charToPromotionPiece {
        {'r', PieceType::Rook}, {'n', PieceType::Knight}, {'b', PieceType::Bishop}, {'q', PieceType::Queen}
    };

    std::optional<PieceType> promotionValue = uciMove.size() == 5 
        ? std::optional<PieceType>{charToPromotionPiece.at(uciMove.data()[4])} 
        : std::nullopt;

    pieceMovement partialMovement;

    // in the case of a capture this is the piece type of the enemy piece which is captured

    // another thing, its alright to generate junk moves here since we are going to use this to search through the move generators outputs

    if (promotionValue.has_value()) {
        PieceType pieceTypeOnMoveTo = figureOutTypeOfPieceOnSquare(1ULL << to_offset, !isWhiteTurn);
        partialMovement = isWhiteTurn ? 
            pieceMovement{1ULL << from_offset, 1ULL << to_offset, PieceType::Pawn, PieceType::NotAPiece, promotionValue.value(), pieceTypeOnMoveTo} : 
            pieceMovement{1ULL << from_offset, 1ULL << to_offset, PieceType::NotAPiece, PieceType::Pawn, pieceTypeOnMoveTo, promotionValue.value()};
    } else {
        // the type of piece (friendly we are moving)
        partialMovement = pieceMovement{1ULL << from_offset | 1ULL << to_offset};
    }
    
    return partialMovement;
}

std::optional<pieceMovement> searchAndSelectMove(const stackStack218 &generatedMoves, const pieceMovement &canidateIncompleteMove) {
    for (const auto& mv : generatedMoves) {
        if (mv.compareForSelection(canidateIncompleteMove))
            return mv;
    }
    return std::nullopt;
}
