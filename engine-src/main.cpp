#include "engine.hpp"
#include "chessBoard.h"
#include <chrono>
#include <print>

int main (int argc, char *argv[]) {
    if (false){
    chessBoard board {"rnbqkb1r/pppppppp/7n/8/4P3/2N5/PPPP1PPP/R1BQKBNR b KQkq - 2 2"} ;
    std::println("board score : {}", slowEval(board));
    std::println("board score : {}", pieceWiseEval(board));
    }

    if (false){
        chessBoard board {"rnb1k2r/p1p1bppp/3qpn2/1p1P4/8/2NB1N2/PPPP1PPP/R1BQ2KR w kq - 0 1"};
        std::println("board score white to move standard : {}", slowEval(board));
        std::println("board score white to move piecewise: {}", pieceWiseEval(board));
    }

    if(true) {

        int depth {6};
        chessBoard board {"r1bqk2r/p1p1nppp/1p1p1n2/b2P2B1/2B1P3/2N2N2/PP3PPP/R2Q1RK1 w kq - 0 11"} ;
        {
        auto start = std::chrono::steady_clock::now();
        searchTelemetry tele {};
        std::println("negamax (αβ unordered) ({}) score  : {}", depth, alphaBetaUnordered<pieceWiseEval>(depth, board, &tele).evalScore);
        auto stop = std::chrono::steady_clock::now();
        long numMilliseconds{std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()};
        std::println("negamax (αβ unordered) took {} ms, with {} eval calls", numMilliseconds, tele.numEvals);
        }
        {
        searchTelemetry tele {};
        auto start = std::chrono::steady_clock::now();
        std::println("negamax (αβ ordered) ({}) score  : {}", depth, alphaBeta<pieceWiseEval>(depth, board, &tele).evalScore);
        auto stop = std::chrono::steady_clock::now();
        long numMilseconds {std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()};
        std::println("negamax (αβ ordered) took {} ms, with {} eval calls", numMilseconds, tele.numEvals);
        }
    }

    chessBoard board2 {"r1bqk1nr/ppp1nppp/3p4/3P4/1b2P3/2N2N2/PP3PPP/R1BQKB1R w KQkq - 1 8"};
    if (true) {
        for (int i {3}; i <= 3; ++i) {
            auto result = alphaBeta<pieceWiseEval>(i, board2);
            std::println("alpha beta result for depth {} : ({}, {})", i, result.evalScore, board2.uciStringMove(result.move));

            // result.move.printThis();
            
        }
    }
    // std::println("negamax (5) score with make   : {}", negaMax(3, board));
    
    

    return 0;
}
