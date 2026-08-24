#include "engine.hpp"
#include "chessBoard.h"
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <print>
#include <ranges>
#include <sstream>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

void tests() {
    if constexpr (false){
    chessBoard board {"rnbqkb1r/pppppppp/7n/8/4P3/2N5/PPPP1PPP/R1BQKBNR b KQkq - 2 2"} ;
    std::println("board score : {}", slowEval(board));
    std::println("board score : {}", pieceWiseEval(board));
    }

    if constexpr (false){
        chessBoard board {"rnb1k2r/p1p1bppp/3qpn2/1p1P4/8/2NB1N2/PPPP1PPP/R1BQ2KR w kq - 0 1"};
        std::println("board score white to move standard : {}", slowEval(board));
        std::println("board score white to move piecewise: {}", pieceWiseEval(board));
    }

    if constexpr(false) {

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
        std::println("negamax (αβ ordered) ({}) score  : {}", depth, alphaBeta<pieceWiseEval>(depth, board, transpositionTableAccess{}, &tele).evalScore);
        auto stop = std::chrono::steady_clock::now();
        long numMilseconds {std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()};
        std::println("negamax (αβ ordered) took {} ms, with {} eval calls", numMilseconds, tele.numEvals);
        }
    }

    // std::println("negamax (5) score with make   : {}", negaMax(3, board));
}

void processPositionTokens(chessEngine& engineState, FastStack<std::string_view, 100>& tokens) {
    size_t lastConsumedArgument {0};

    std::optional<std::string> position {};
    std::optional<std::string> moves {};

    if (tokens[1] == "startpos") {
        position = "startpos";
        lastConsumedArgument = 1;
    } else if (tokens[1] == "fen") {
        bool first = true;
        for (auto i {2}; i < tokens.numitems(); i++) {
            if (tokens[i] == "moves") {
                lastConsumedArgument = i != 2 ? i - 1 : 0;
                i = tokens.numitems();
                break;
            } else if (first) {
                position = tokens[i];
                first = false;
            } else {
                *position += " ";
                *position += tokens[i];
            }
            lastConsumedArgument = i;
        }
    }

    {
        bool first {true};
        for (auto i {lastConsumedArgument + 1}; i < tokens.numitems(); i ++) {
            // std::println("token : {}", tokens[i]);
            if (first && tokens[i] != "moves") {
                i = tokens.numitems();
                break;
            } else if (first && tokens[i] == "moves") {
                first = false;
                continue;
            }
            else if (moves.has_value()) {
                *moves += " ";
                *moves += tokens[i];
            } else {
                moves = tokens[i];
            }
        }
    }

    if (!engineState.loadPosition(position, moves)) {
        std::cerr << "problem loading supplied moves from position";
        // TODO : decide what is going to happen if invalid moves are entered
        // std::println("invalid moves");
    }
    return;
}

void processLineTokens(chessEngine& engineState, FastStack<std::string_view, 100>& tokens) {
    if (tokens[0] == "position" && tokens.numitems() >= 2) {
        processPositionTokens(engineState, tokens);
        return;
    } else if (tokens[0] == "go") {
        if (tokens[1] == "infinite") {
            engineState.startSearch();
        }
        return;
    }
}

// processing a single input line over the standard in, gui / user commands
void processLine(chessEngine& engineState, std::string_view inputLine) {
    if (inputLine == "uci") {
        std::println("id name whatnotChess");
        std::println("id auther Henry");
        // std::println("option");
        std::println("uciok");
        return;
    }
    else if (inputLine == "isready") {
        engineState.allocateTranspositionTable(16);
        std::println("readyok");
        return;
    }
    else if (inputLine == "stop") {
        engineState.stopSearching();
        // assume we have an answer, if not we should crash, this will happen if we havent started seaching at all
        std::println("bestmove {}", engineState.m_chessBoard.uciStringMove(engineState.getAnswer().value().bestMove));
    }


    FastStack<std::string_view, 100> tokens {};
    for (auto token : std::ranges::split_view{inputLine, ' '}) {
        if (std::string_view(token) == " ")
            continue;
        tokens.pushVal(std::string_view(token));
    };

    processLineTokens(engineState, tokens);
    return;
}

void engineProcess(std::optional<argumentValue> gs, interfacePrinterState& printerState) {
    chessEngine engineState;
    engineState.m_printerState = &printerState;
    if (gs.has_value()) {
        engineState.loadPosition(gs->fen, gs->moves);
    } else {
        engineState.loadPosition();
    }

    std::string inputLine;
    while (std::getline(std::cin, inputLine)){
        if (inputLine == "")
            continue;

        processLine(engineState, inputLine);
    }
    
}

int main (int argc, char *argv[]) {
    // tests();

    argumentValue gs = argumentParser(argc, argv);
    // std::println("general state : (uciMode : {}, fen : {}, moves : {})", gs.uciMode, gs.fen, gs.moves);
    interfacePrinterState printerState {};
    constexpr size_t pollTimeMilliseconds = 10;
    std::jthread printerThread {printerProcess<pollTimeMilliseconds>, std::ref(printerState)};

    engineProcess(gs, printerState);

    return 0;
}
