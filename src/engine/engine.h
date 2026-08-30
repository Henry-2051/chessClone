#include "chessBoard.h" 
#include "eval.hpp"
#include "stackStack.hpp"
#include <cassert>
#include <climits>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string_view>
#include <sys/types.h>
#include <thread>
#include "threaddedBuffer.h"
#include "transpositionTable.h"
#include "engineSharedDatatypes.hpp"
#include "search.h"

struct threaddedSearchAnswer {
    std::mutex mu;
    std::optional<searchAnswer> answer;
    int currentDepth;
};

struct argumentValue {
    std::optional<std::string> fen {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};
    std::optional<std::string> moves {};
};

argumentValue argumentParser(int argc, char* argv[]) ;


template <EvalFunction eval>
class chessEngineTe {
    std::jthread m_searchThread;
    int m_tableSizeMB {16};

    // storing the pointer here eliminates the need for us to seperately bookkeep whether the table has been initialised or not, we use nullptr
    TT::transpositionTableInterface m_TTAccess {};
    std::unique_ptr<TT::tableEntry[]> m_transpositionTable {nullptr};

    public:
    std::optional<searchAnswer> m_answer {std::nullopt};
    bool m_isthinking {false};
    threaddedSearchAnswer m_sharedAnswer;
    int m_currentDepth {0};
    chessBoard m_chessBoard;
    interfacePrinterState* m_printerState{nullptr};

    bool allocateTranspositionTable(int tableSizeMB);

    void deallocateTranspositionTable();

    void reset();

    static void iterativeSearch(std::stop_token st, chessBoard board, threaddedSearchAnswer& sharedAnswer, 
            transpositionTableAccess tableAccess, interfacePrinterState* printerState = nullptr, std::optional<int> testDepth = std::nullopt, bool useAlphaBetaPruning = true);

    // chessEngine() = default;

    // replaces the internal chess board with one of the supplied position and moves, if no arguments are provided uses the default start position
    // note will not return false if given an invalid fenString
    // you cannont return a bool from a constructor without passing an extra reference
    

    bool 
    loadPosition(std::optional<std::string_view> fenString = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", std::optional<std::string_view> moves = {}) ;

    bool 
    loadPosition(const chessBoard& board);

    // starts searching in a seperate thread
    bool startSearch(interfacePrinterState* loggerState = nullptr, std::optional<int> testDepth = std::nullopt, bool useAlphaBetaPruning = true) ;

    // updates the internal search answer state in a thread safe manner and returns the new state
    // if the engine isnt thinking then it just returns gets the most recent answer
    std::optional<searchAnswer> getAnswer();

    static int quiessenceSearch(chessBoard board);

    // stops all searching and updates the internal answer with the final threadded answer incase a new depth has been
    // completed since getAnswer was called
    bool stopSearching();
};

using chessEngine = chessEngineTe<pieceWiseEval>;
