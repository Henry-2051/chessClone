#include "../src/pieceMovements.hpp"
#include "./printBitboard.cpp"
#include <cstdint>
#include <iostream>
#include <string>

template <typename Function>
void printAttacks(uint64_t attacking_pieces, uint64_t enemy_pieces, Function operation, std::string name_of_attacking) {
    std::cout << name_of_attacking << " layout\n";
    printBitboard(attacking_pieces);
    std::cout << "\n targets \n";
    printBitboard(enemy_pieces);
    std::cout << "\n attacked squares \n";
    printBitboard(operation(attacking_pieces, enemy_pieces, attacking_pieces));
    std::cout << "\n";
}

template <typename Function>
void printPins(uint64_t attacking_pieces, uint64_t enemy_pieces, uint64_t enemy_king, Function operation, std::string name_of_attacking) {
    std::cout << name_of_attacking << " pin layout\n";
    printBitboard(attacking_pieces);
    std::cout << "\n enemy pieces \n";
    printBitboard(enemy_pieces | enemy_king);
    std::cout << "\n enemy king \n";
    printBitboard(enemy_king);
    std::cout << "\n";
    printBitboard(operation(attacking_pieces, enemy_pieces, attacking_pieces, enemy_king));
    std::cout << "\n";
}

int main (int argc, char *argv[]) {
    uint64_t sample_rookboard = 8796160147456;
    uint64_t sample_bishops   = 2305843026393571328;
    uint64_t sample_queens    = 17592186044928;
    uint64_t sample_pawns     = 847839232;

    printAttacks(sample_rookboard, 0, chessMoves::rookMove, "rooks");
    printAttacks(sample_rookboard, sample_pawns, chessMoves::rookMove, "rooks");
    printAttacks(sample_queens, 0, chessMoves::queenMove, "queens");

    // testing pin detection with rooks
    uint64_t test_pin_rook = 262144;
    uint64_t test_pin_generic_enemies = 2199561183232;
    uint64_t test_pin_enemy_king = 8388608;

    printPins(test_pin_rook, test_pin_generic_enemies, test_pin_enemy_king, chessMoves::rookPins, "the ROOKAH");
    return 0;
}
