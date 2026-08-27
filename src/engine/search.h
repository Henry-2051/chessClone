#include "engineSharedDatatypes.hpp"
#include "transpositionTable.h"
#include <stop_token>

template <EvalFunction eval>
searchAnswer 
alphaBeta(int depth, chessBoard board, transpositionTableAccess tableAccess, searchTelemetry* tele= nullptr, std::optional<std::stop_token> stop_token = {});

namespace search {
template<EvalFunction eval>
int 
quiessenceSearch(chessBoard board, stackStack218& allMovesMemory, bool _madeNullMove = false);
}
