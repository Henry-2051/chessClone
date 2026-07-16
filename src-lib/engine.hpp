#include "chessBoard.h" 
#include "pieceMovements.hpp"
#include "stackStack.hpp"
#include <cstdint>


struct searchResult {
    double evalScore;
    pieceMovement bestMove;
};

// negamax search

int eval(const chessBoard& board) {
    uint64_t   _50ptPawnBitboard {65280};
    uint64_t   _30ptPawnBitboard {1572864};
    uint64_t   _25ptPawnBitboard {402653184};
    uint64_t   _20ptPawnBitboard {103081574400};
    uint64_t   _10ptPawnBitboard {28710448241246208};
    uint64_t   __5ptPawnBitboard {36452112267214848};
    uint64_t  neg5PtPawnBitboard {72567767433216};
    uint64_t neg10PtPawnBitboard {39582418599936};
    uint64_t neg20PtPawnBitboard {6755399441055744};
    
};



int negaMax(chessBoard& board, int depth=0) {
    
}
