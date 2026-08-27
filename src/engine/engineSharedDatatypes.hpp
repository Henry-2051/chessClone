#include "chessBoard.h"
#include <cstdint>
#include <string>
#pragma once

#define INF 1000000000

using EvalFunction = int (*) (const chessBoard&, bool);

enum class SearchReturnState : char {
    Normal,
    BetaCutoff,
    SearchTerminated,
    CheckmateOrDraw,
};

// enum class EvalReturnState : char {
//     Normal,
//     WhiteCheckmate,
//     BlackCheckmate,
//     Draw,
// };

// struct evalReturn {
//     int score;
//     EvalReturnState specialState;
// };

struct searchAnswer {
    int eval;
    pieceMovement bestMove;
    SearchReturnState returnState;

    searchAnswer& negate() {
        eval = -eval;
        return *this;
    }
};

struct searchState {
    int alpha;
    int beta;
    const uint16_t rootNumPlys {0};
    const uint16_t rootSearchDepth {2};
    std::optional<pieceMovement> lastMove;

    inline searchState reflectPure() const {
        searchState st {*this};

        st.alpha = -beta;
        st.beta = -alpha;

        return st;
    }
};

struct searchTelemetry {
    size_t numEvals{0};
    size_t numMovegens {0};
    size_t nodes {0};
    std::vector<searchAnswer> principleVariation {};
    std::vector<chessBoard> boardStates {};
};
