
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <string>
#include <utility>


#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include "maybeResult.hpp"
#include "loadChessAssets.hpp"
// there is a chess board that conains all the chess pieces 
// there are also chess piece assets 
// we will need to access the chess piece sprites and the chess board at the same time 
// we can use int64 for each piece and xor with int64 as actions 
// we need to read user input 


std::vector<int> getOnes(uint64_t b) {
    std::vector<int> ones = {};
    int count = 0;
    while (b > 0) {
        if (b % 2 == 1) {
            ones.push_back(count);
        } 
        b /=2;
        count ++;
    }
    return ones;
}

std::vector<std::pair<int, int>> getChessCoordinates(std::vector<int> ones) {
    std::vector<std::pair<int, int>>   result = {};
    for (auto p : ones) {
        int row = p / 8;
        int col = p % 8;
        result.push_back({col, row});
    }
    return result;
}

class chessBoard {
    uint64_t black_white_bitshift = 40; 
    uint64_t black_pawns = (1 + 2 + 4 + 8 + 16 + 32 + 64 + 128) << 8;
    uint64_t black_rooks = (1 + 128);
    uint64_t black_knights = (2+64);
    uint64_t black_bishops = (4+32);
    uint64_t black_queen = 8;
    uint64_t black_king = 16;

    uint64_t white_pawns = black_pawns << black_white_bitshift;
    uint64_t white_rooks = black_rooks << black_white_bitshift;
    uint64_t white_knights = black_knights << black_white_bitshift;
    uint64_t white_bishops = black_bishops << black_white_bitshift;
    uint64_t white_queen = black_queen << black_white_bitshift;
    uint64_t white_king = black_king << black_white_bitshift;

    using annoying_return_type = std::vector<std::vector<std::pair<int, int>>>;

    std::vector<std::string> black_names = {"black_pawns", "black_rooks", "black_knights", "black_bishops", "black_queen", "black_king"};
    std::vector<std::string> white_names = {"white_pawns", "white_rooks", "white_knights", "white_bishops", "white_queen", "white_king"};

    chessBoard() = default;

annoying_return_type piecePositions() {
        annoying_return_type result = {};
        result.push_back(getChessCoordinates(getOnes(black_pawns)));
        result.push_back(getChessCoordinates(getOnes(black_rooks)));
        result.push_back(getChessCoordinates(getOnes(black_knights)));
        result.push_back(getChessCoordinates(getOnes(black_bishops)));
        result.push_back(getChessCoordinates(getOnes(black_queen)));
        result.push_back(getChessCoordinates(getOnes(black_king)));

        result.push_back(getChessCoordinates(getOnes(white_pawns)));
        result.push_back(getChessCoordinates(getOnes(white_rooks)));
        result.push_back(getChessCoordinates(getOnes(white_knights)));
        result.push_back(getChessCoordinates(getOnes(white_bishops)));
        result.push_back(getChessCoordinates(getOnes(white_queen)));
        result.push_back(getChessCoordinates(getOnes(white_king)));
        return result;
    }
};

int main()
{
    float scale = 2.0f;

    sf::Color lightColor(181, 240, 212);  // light square color
    sf::Color darkColor(30, 43, 37);    // dark square color
    auto maybeData = loadChessPiecesTexture(scale);

    sf::Texture chessPieceTexture;
    int w,h, ww, wh;
    if (maybeData.exists()) {
        chessPieceTexture = maybeData.m_value.texture;
        w = maybeData.m_value.w;
        h = maybeData.m_value.h;
    } else { return -1; }

    int board_square_size = 60 * scale;
    int edge_padding = 100;
    ww =  8 * board_square_size + 2 * edge_padding;
    wh = ww;

    int pieceHeight = chessPieceTexture.getSize().y/ 2;

    std::cout << "x,y : (" <<  chessPieceTexture.getSize().x << ", " << chessPieceTexture.getSize().y << ")\n";
    
    sf::RectangleShape square{sf::Vector2f(board_square_size, board_square_size)};
    sf::Sprite sprite(chessPieceTexture);

    sf::Sprite king = sf::Sprite(chessPieceTexture); 
    king.setTextureRect(sf::IntRect({0,0}, {pieceHeight, pieceHeight}));
    // king.setPosition(ww / 2, wh / 2);


    // 7) Setup SFML window
    sf::RenderWindow window(sf::VideoMode(ww, wh), "NanoSVG + SFML");

    window.setFramerateLimit(60); // Limit to 60 frames per second

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed)
                window.close();

        window.clear(lightColor);
        int x_ = edge_padding;
        int y_ = edge_padding;
        int x = x_;
        int y = y_;
        for (int c = 0; c < 8; c++) {
            for (int r = 0; r < 8; r++) {
                
            }
        }
        // window.draw(sprite);
        window.draw(king);
        
        
        window.display();
    }

    return 0;
}
