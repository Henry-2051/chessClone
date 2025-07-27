
#include <cstdint>
#include <iostream>
int getRookPlace(uint64_t theRook) {
    int place = __builtin_ctzll(theRook);
    return place;
}

int main (int argc, char *argv[]) {
    // square 12
    uint64_t rook1 = 4096;
    // 14
    uint64_t rook2 = 16384;
    // 60
    uint64_t rook3 = 1152921504606846976;

    std::cout << "rook 1 place " << getRookPlace(rook1) << "\n";

    std::cout << "rook 2 place " << getRookPlace(rook2) << "\n";

    std::cout << "rook 3 place " << getRookPlace(rook3) << "\n";
    return 0;
}
