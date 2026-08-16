#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "boardState.hpp"
#include "stackStack.hpp"

#pragma once

struct chessBoard {
    uint64_t bitboards[12] {0};

    // uint64_t m_black_pawns = 0;
    // uint64_t m_black_rooks = 0;
    // uint64_t m_black_knights = 0;
    // uint64_t m_black_bishops = 0;
    // uint64_t m_black_queens = 0;
    // uint64_t m_black_king = 0;
    //
    // uint64_t m_white_pawns = 0;
    // uint64_t m_white_rooks = 0;
    // uint64_t m_white_knights = 0;
    // uint64_t m_white_bishops = 0;
    // uint64_t m_white_queens = 0;
    // uint64_t m_white_king = 0;
    
    // there are no guarantees as to whether these will reflect the state of bitboards

    uint8_t m_board_state = board_state::WhiteTurn;

    int8_t enPassantState = {-1}; 

    size_t halfMoveClock {0};
    size_t moveNumber {0};

    chessBoard();

    chessBoard(std::string_view fenString);

    using annoying_return_type = std::vector<std::vector<std::pair<uint32_t, uint32_t>>>;
    

    inline constexpr uint64_t whitePieces() const {
      return bitboards[PieceType::Rook + 6] | bitboards[PieceType::Pawn + 6] |
             bitboards[PieceType::Knight + 6] | bitboards[PieceType::Bishop + 6] |
             bitboards[PieceType::Queen + 6] | bitboards[PieceType::King + 6];
    }

    inline constexpr uint64_t blackPieces() const {
      return bitboards[PieceType::Bishop] | bitboards[PieceType::Pawn] |
             bitboards[PieceType::Rook] | bitboards[PieceType::Knight] |
             bitboards[PieceType::Queen] | bitboards[PieceType::King];
    }

    inline constexpr uint64_t* getPiecesByColor(bool isWhiteTurn){
        if (isWhiteTurn) {
            return &bitboards[0] + 6;
        } else {
            return bitboards;
        }
    }

    inline constexpr const uint64_t* getPiecesByColorConst(bool isWhiteTurn) const {
        if (isWhiteTurn) {
            return &bitboards[0] + 6;
        } else {
            return bitboards;
        }
    }

    annoying_return_type piecePositions() const;

    void readFenAndUpdate(std::string_view inputFen);

    template <PieceType pt>
    inline uint64_t pieceToBitboardConst(bool isWhiteTurn) const {
        if (isWhiteTurn) {
            return bitboards[std::to_underlying(pt)+6];
        } else {
            return bitboards[std::to_underlying(pt)];
        }
    }

    // this function is so slow because if we know what pt is at compile time when we 
    // call this function the cpu doenst branch
    // predict properly, we essentially increase the branching factor by a factor of 6 
    // this effect has been measured using linux perf to be signifficant
    //
    // but sometimes pt is dynamic like when we want to make a uci move so this needs to exist
    // we actually never use this in the move gen code, and it shouldnt be used in the hot path
    // if we can avoid it

    inline uint64_t slowPieceToBitboardConst(bool isWhiteTurn, PieceType pt) const {
        if (isWhiteTurn) {
            return bitboards[std::to_underlying(pt)+6];
        } else {
            return bitboards[std::to_underlying(pt)];
        }
    }

    template <bool isWhiteTurn>
    inline uint64_t slightlySlowPieceToBitboardConst(PieceType pt) const {
        if (isWhiteTurn) {
            return bitboards[std::to_underlying(pt)+6];
        } else {
            return bitboards[std::to_underlying(pt)];
        }
    }

    inline PieceType figureOutTypeOfPieceOnSquare(uint64_t square, bool checkWhiteColor) const {
        // there should only be 1 bitboard that satisfies the condition in the loop, this should be simd able 

        assert(std::popcount(square) == 1);
        // if (std::popcount(square) != 1) {
        //     std::println("failed with invalid argument, argument passed : ");
        //     helpers::printBitboard(square);
        //     throw std::runtime_error("invalid argument");
        // }

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

    // has the based property that if we apply the same move twice we get our original board back
    chessBoard applyMovePure(const pieceMovement& move) const;

    // board.applyMove(myMove).applyMove(myMove) == board should always be true
    inline chessBoard& applyMoveImpure(const pieceMovement& move) {
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

        enPassantState ^= move.enPassantState;

        return *this;
    }

    std::optional<pieceMovement> genPartialMove(std::string_view uciMove) const;

    std::string uciStringMove(pieceMovement move) const;

    private:
    void changeRank(std::string_view fenRank, size_t rankNum);
    void updateBoardState(std::string_view term, size_t termNumber);
    
};

struct guiBoard {
    chessBoard board;
    std::string inputFen;

    guiBoard(std::string_view inputFen) : board(inputFen), inputFen(inputFen) {};

    guiBoard& updateFen(std::string_view fen){
        board = chessBoard(fen);
        inputFen = fen;
        return *this;
    }
};

std::optional<pieceMovement> searchAndSelectMove(const stackStack218& generatedMoves, const pieceMovement& canidateIncompleteMove);
