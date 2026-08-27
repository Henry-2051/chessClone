#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pieceMovements.hpp"
#include "perftreeRun.h"
#include "testFastStack.h"

TEST_CASE("Basic test of movegen function") {
    auto myBoard = chessBoard();
    CHECK(chessMoves::makeAllMoves(myBoard).numitems() == 20);
};

TEST_CASE("Movegen regression") {
    // TODO : write function to print out chess postion in the terminal (not difficult)
    size_t numMoves {0};
    auto myBoard = chessBoard("r6r/pp2kppp/2p5/8/3QN3/4Rq2/bPP3PP/R5K1 w - - 2 17");
    perftreeRun<true>(4, myBoard, numMoves);
    CHECK(numMoves == 2057526);

    myBoard = chessBoard("r7/1R2KP2/1N1P1NP1/3P3P/3p3p/3nnp2/1p1k2p1/8 w - - 0 1");
    numMoves = 0;
    perftreeRun<true>(5, myBoard, numMoves);
    CHECK(numMoves == 21666417);

    myBoard = chessBoard("r1r2nk1/p1q1bppp/b3p3/2p1P3/2Pp4/Pn3NNP/RPBBQPP1/4R1K1 b - - 9 20");
    numMoves = 0;
    perftreeRun<true>(4, myBoard, numMoves);
    CHECK(numMoves == 2055262);

    myBoard = chessBoard("rnbqkb1r/ppp3pp/8/3pNp2/3Pn3/5P2/PPP3PP/RNBQKB1R w KQkq - 0 6");
    numMoves = 0;
    perftreeRun<true>(5, myBoard, numMoves);
    CHECK(numMoves == 60969561);
}
