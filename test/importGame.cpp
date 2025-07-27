#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include "readTextFile.hpp"


int
main()
{
    // Here we use a filename relative to the executable directory.
    std::string chessGame = readFromFile("chessTestGame.chess");
    std::cout << "THE CHESS GAME : " << chessGame << std::endl;
    return 0;
}
