#include "chessBoard.h"
#include <cstdint>
#pragma once

#define INF 1000000000

using EvalFunction = int (*) (const chessBoard&, bool);

enum class SearchReturnState : char {
    Normal,
    NotAssigned,
    SearchTerminated,
    EndOfGame
};

struct quiessenceSearchReturn {
    int score;
    SearchReturnState rState;
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
    const int searchDepth;
    searchAnswer* const pvMemoryStart;
    size_t numEvals{0};
    size_t numMovegens {0};
    size_t nodes {0};
};

// currentDepth = searchDepth - depthLeft
inline size_t pvMemoryOffset(int searchDepth, int depthLeft) {
    // I adapted / invented a formula ;w; 
    // if search depth = a_n === 4  and current depth = a_m === 10 then pvMemoryOffset(a_n, a_m) = pvMemoryOffset(4,10) = sum(10 + 9 + 8 + 7 + 6 + 5)
    // if a_n = 10, a_m = 10 then pvMemoryOffset(10,10) = 0, pvMemoryOffset(9,10) = 10, pvMemoryOffset(8, 10) = 19 ect
    // this is a special indexing system to make an array with non constant increment
    
    return ((((double)searchDepth - (double)depthLeft + 1.0) / 2.0) * ((double)searchDepth + (double)depthLeft)) - (double)depthLeft;
}
