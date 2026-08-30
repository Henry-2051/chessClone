#include "engine.h"
#include "chessBoard.h"
#include "engineSharedDatatypes.hpp"
#include "eval.hpp"
#include "pieceMovements.hpp"
#include <format>
#include <memory>
#include <optional>
#include <thread>

argumentValue argumentParser(int argc, char* argv[]) {
    argumentValue gs;
    // handle --uci 
    int lastLoadedParameter {argc};
    for (int i {argc}; i-- > 1;) {
        std::string uciString {"--uci"};
        if (std::strcmp(argv[i], uciString.c_str()) == 0) {
            // gs.uciMode = true;
            // Engine only handles uci
            lastLoadedParameter = i;
        }
    
        std::string fenString {"--fen"};
        if (std::strcmp(argv[i], fenString.c_str()) == 0) {
            gs.fen = "";
            for (int j {i+1}; j < lastLoadedParameter; j++) {
                if (!gs.fen.has_value())
                    gs.fen = argv[j];
                else 
                    *gs.fen += (std::string(" ") + argv[j]);
            }

            lastLoadedParameter = i;
        }
        std::string movesString {"--moves"};
        std::string moves {""};
        if (std::strcmp(argv[i], movesString.c_str()) == 0) {
            for (int j {i+1}; j < lastLoadedParameter; j++) {
                if (!gs.moves.has_value())
                    gs.moves = argv[j];
                else 
                    *gs.moves += (std::string(" ") + argv[j]);
            }

            lastLoadedParameter = i;
        }

    }
    return gs;
}

template <EvalFunction eval>
bool chessEngineTe<eval>::allocateTranspositionTable(int tableSizeMB) {
    m_tableSizeMB = tableSizeMB;

    try {
        m_TTAccess = TT::transpositionTableInterface(tableSizeMB);
        m_transpositionTable = std::make_unique<TT::tableEntry[]>(m_TTAccess.tableSize) ;
    } catch (std::bad_alloc) {
        return false;
    }
    return true;
}

template <EvalFunction eval>
void chessEngineTe<eval>::deallocateTranspositionTable() {
    m_transpositionTable.reset(nullptr);
}

template <EvalFunction eval>
void chessEngineTe<eval>::reset() {
    stopSearching();
    deallocateTranspositionTable();
    m_answer = std::nullopt;
    m_searchThread = std::jthread();
    delete m_printerState;
    m_printerState = nullptr;
    m_chessBoard = chessBoard();
    m_isthinking = false;
}

template <EvalFunction eval>
void 
chessEngineTe<eval>::iterativeSearch(std::stop_token st, chessBoard board, threaddedSearchAnswer& sharedAnswer, transpositionTableAccess tableAccess, interfacePrinterState* printerState, std::optional<int> testDepth, bool useAlphaBetaPruning) {
    int depth {2};
    
    while(!st.stop_requested()) {
        searchAnswer ans;
        if (testDepth.has_value())
            depth = *testDepth;

        // second heap allocation
        // we can actually use the formula (n/2) * (a_1 + a_n) to index into this 
        // and also instead of n^2 for the size, its best to overallocate and we'll cause a stack 
        // overflow before this becomes signifficant, we are storing a 5.2KB array on the heap for each 
        // depth searched, after depth 200 we will have used 1MB of stack memory, overflow territory
        
        auto pvMemory  = std::make_unique<searchAnswer[]>(depth * depth);

        searchTelemetry telemetry {depth, pvMemory.get()};
        
        // actually there will be a stack overflow after about depth 200, need more heap allocations
        auto start = std::chrono::steady_clock::now();
        ans = alphaBeta(depth, board, tableAccess, &telemetry, st, useAlphaBetaPruning);
        auto stop = std::chrono::steady_clock::now();
        if (ans.returnState != SearchReturnState::SearchTerminated) {
            {
                // prevent race conditions with a mutex lock
                std::lock_guard<std::mutex > lock{sharedAnswer.mu};
                sharedAnswer.answer = ans;
                sharedAnswer.currentDepth = depth;
            }

            if (printerState != nullptr)
            {
                long long searchTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

                double nodesPerSecond = (telemetry.nodes / static_cast<double>(std::max((int)searchTimeMs, 1))) / 1000.0;
                std::string info = std::format("info currmove {} depth {} searchtime_ms {} MNodes {} MNodes per second {}", board.uciStringMove(ans.bestMove), depth, searchTimeMs, telemetry.nodes / 1000000, nodesPerSecond);
                auto begin = telemetry.pvMemoryStart;
                auto end = begin + telemetry.searchDepth;

                chessBoard boardCopy {board};

                std::string variation {"principle variation:\n"};
                for (searchAnswer* it = begin; it != end; it ++) {
                    searchAnswer ans = *it;
                    stackStack218 allMoves;
                    chessMoves::makeAllMovesWithDataReturn(boardCopy, allMoves);
                    for (const auto& mv : allMoves) {
                        // ensure the moves in the principle variation are legal moves
                        if (mv.compareForSelection(ans.bestMove)) {
                            variation += std::format("move : {}, engineEval : {}, quiessenceEval: {}\n", boardCopy.uciStringMove(mv), ans.eval, chessEngine::quiessenceSearch(boardCopy.applyMovePure(mv)));
                            break;
                        }
                    }

                    boardCopy.applyMoveImpure(ans.bestMove);
                }

                {
                std::lock_guard<std::mutex> lock {printerState->stdoutBuffer.bufferMutex};
                printerState->stdoutBuffer.buffer.push_back(std::move(info));
                printerState->stdoutBuffer.buffer.push_back(std::move(variation));
                // for (const auto& b : telemetry.boardStates) {
                //     printerState->stdoutBuffer.buffer.push_back(std::format("{} {}\n", b.stringBoard(), chessEngine::quiessenceSearch(b)));
                // }
                }

                printerState->flushBuffer.notify_one();
            }
        }
        depth++;

        if (testDepth.has_value())
            break;
    }
}

// replaces the internal chess board with one of the supplied position and moves, if no arguments are provided uses the default start position
// note will not return false if given an invalid fenString
// you cannont return a bool from a constructor without passing an extra reference

template <EvalFunction eval>
bool 
chessEngineTe<eval>::loadPosition(std::optional<std::string_view> fenString, std::optional<std::string_view> moves) 
{
    // TODO : add logging 
    // if (fenString.has_value())
    //     std::println("loading fen : {}", *fenString);
    // if (moves.has_value())
    //     std::println("loading moves : {}", *moves);
    //
    // if there is a value then load it 
    // otherwise we dont change the board state
    if (fenString == "startpos" || fenString == "") {
        m_chessBoard = chessBoard();
    } else {
        if (fenString.has_value()) {
            m_chessBoard = chessBoard(*fenString);
        }
    }

    bool sucess = moves.has_value() ? chessMoves::makeMovesFromUciSequence(m_chessBoard, *moves) : true;

    return sucess;
};

template <EvalFunction eval>
bool
chessEngineTe<eval>::loadPosition(const chessBoard& board) {
    m_chessBoard = board;
    stopSearching();
    allocateTranspositionTable(m_tableSizeMB);
    return true;
}

// starts searching in a seperate thread
template <EvalFunction eval>
bool chessEngineTe<eval>::startSearch(interfacePrinterState* loggerState, std::optional<int> testDepth, bool useAlphaBetaPruning) {
    if (m_isthinking)
        return false;
    transpositionTableAccess tableAccess {};
    if (m_transpositionTable == nullptr) {
        tableAccess = {&m_TTAccess, m_transpositionTable.get()};
    }
    
    if(loggerState == nullptr)
        m_searchThread = std::jthread(chessEngineTe<eval>::iterativeSearch,  m_chessBoard, std::ref(m_sharedAnswer), tableAccess, m_printerState, testDepth, useAlphaBetaPruning);
    else 
        m_searchThread = std::jthread(chessEngineTe<eval>::iterativeSearch,  m_chessBoard, std::ref(m_sharedAnswer), tableAccess, loggerState, testDepth, useAlphaBetaPruning);

    m_isthinking = true;
    bool sucess = true;
    return sucess;
};

// updates the internal search answer state in a thread safe manner and returns the new state
// if the engine isnt thinking then it just returns gets the most recent answer
template <EvalFunction eval>
std::optional<searchAnswer> chessEngineTe<eval>::getAnswer() {
    if (m_isthinking) {
        m_sharedAnswer.mu.lock();
        m_answer = m_sharedAnswer.answer;
        m_sharedAnswer.mu.unlock();
    }

    return m_answer;
};

template <EvalFunction eval>
int chessEngineTe<eval>::quiessenceSearch(chessBoard board) {
    // bool isWhiteTurn = board.m_board_state & board_state::WhiteTurn;
    stackStack218 allMoveMem;
    return search::quiessenceSearch<eval>(board, allMoveMem).score;
    // return isWhiteTurn ? search::quiessenceSearchMut<eval>(board, allMoveMem) : -search::quiessenceSearchMut<eval>(board, allMoveMem);
}

// stops all searching and updates the internal answer with the final threadded answer incase a new depth has been
// completed since getAnswer was called
// will return false if the engine wasnt actually searching
template <EvalFunction eval>
bool chessEngineTe<eval>::stopSearching() {
    if (!m_isthinking)
        return false;
    m_searchThread.request_stop();
    m_searchThread.join();
    m_isthinking = false;
    m_answer = m_sharedAnswer.answer;
    return true;
};

template 
class chessEngineTe<pieceWiseEval>;
