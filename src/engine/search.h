#include "engineSharedDatatypes.hpp"
#include "transpositionTable.h"
#include <stop_token>

searchAnswer 
alphaBeta(int depth, chessBoard board, transpositionTableAccess tableAccess, searchTelemetry* tele= nullptr, std::optional<std::stop_token> stop_token = {}, bool alphaBetaPruning = true);

namespace search {
template<EvalFunction eval>
quiessenceSearchReturn
quiessenceSearch(chessBoard board, stackStack218& allMovesMemory, bool _madeNullMove = false);
}
