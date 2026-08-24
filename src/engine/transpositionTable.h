#include "chessBoard.h"
#include <array>
#include <bit>
#include <cstdint>
#include <memory>
#include <random>
#include <utility>

namespace TT {
// we want to know whether it is more valuable to replace a table entry, our main heuristic is how much computational work 
// that table entry took and so how accurate is its information
//
// imagine we are on our 9th ply on a game of chess and both white and black have both made 4 plys, and we are searching the resulting tree 
// our first table entry could be for a position at depth 2 in the search so 11 plys deep on a depth 7 search so our current depth would be 11 
// and our depth searched would be 5, this would encode information that has been verified to a depth of 5 plys 
//
// now imagine we make a few more plys in our overall game and we are now searching from a position 12plys in and are doing a depth 5 search, 
// now in our search tree at depth 2 we insert a position into the tree and it collides with our earlier entry 
// our new entry only has a depth of 3 and its current depth is 15 and our game depth is at 13 plys, so our earlier entry is stale since we are 
// no longer evaluating positions which are at depth 11plys so we should replace it regardless of the depth searched discrepancy 
//
// replacement scheme order of importance 
// currentDepth > gameDepth  : 
// greater depth searched is preferred, we should never store entries less than 1
//
// then imagine we play the game and make a few moves say 3, and we have a collision between
struct tableEntry {
    // TODO : increase table space effiency by 50% by changing the eval score to a 16 bit integer 
    uint64_t zHash {0};
    int score {0};
    int currentDepth {0};
    int depthSearched {0};
    uint8_t bestMoveIdx {0};
    uint8_t numMoves {0};

    bool operator==(const tableEntry& entry2) const {
        return zHash == entry2.zHash && 
               score == entry2.score && 
               currentDepth == entry2.currentDepth && 
               depthSearched == entry2.depthSearched && 
               bestMoveIdx == entry2.bestMoveIdx && 
               numMoves == entry2.numMoves;
    }

    bool samePosition(const tableEntry& entry2) const {
        return zHash == entry2.zHash && numMoves == entry2.numMoves;
    }

    bool notEmpty() const {
        return *this != tableEntry{};
    }
};

inline tableEntry& tableSelectionFunction(tableEntry& entryInTable, tableEntry& candiate, int rootNumPlys) {
    // check for staleness
    if (entryInTable.currentDepth <= rootNumPlys)
        return candiate;

    // we want to keep results from earlier search iterations which are close to the root of the tree in our table
    // since they are very likely going to be usefull in move ordering and are going to save the most computational work 
    if (candiate.currentDepth <= entryInTable.currentDepth)
        return candiate;
    
    if (candiate.depthSearched > entryInTable.depthSearched)
        return candiate;

    return entryInTable;
} 

class transpositionTableInterface {
    // makes sense for the rng to be private
    std::array<std::array<uint64_t, 64>, 6> pieceSquareRNG;
    uint64_t whiteToMoveRNG;
    std::array<uint64_t, 16> castlingRightsRNG;
    std::array<uint64_t, 8> enPassantFileRNG;

    public:
    size_t tableSize;

    // some random seeds 
    //
    // 0x16da428f5647c95f
    // 0x21995ee10d73397
    // 0x7f99a54c1b3ee194
    // 0x44ab4e76369da72e
    // 0xd04107f553fe64f4

    transpositionTableInterface(int tableSizeMB = 16, uint64_t seed = 0x16da428f5647c95f);

    inline tableEntry& positionFromZHash(uint64_t zHash, tableEntry* table) {
        return table[zHash % tableSize];
    }

    uint64_t hashPosition(const chessBoard& board) const;
};
}
