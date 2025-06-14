
#include <cstdint>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg/src/nanosvg.h"/ SVG parsing
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/src/nanosvgrast.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <memory>

// there is a chess board that conains all the chess pieces 
// there are also chess piece assets 
// we will need to access the chess piece sprites and the chess board at the same time 
// we can use int64 for each piece and xor with int64 as actions 
// we need to read user input 

struct chessBoard {
    uint64_t pawns = 0;
    uint64_t knights = 0;
    uint64_t rooks = 0;
    chessBoard(std::vector<int64_t> initialPosition) : 
}
int main (int argc, char *argv[]) {
    
    return 0;
}
