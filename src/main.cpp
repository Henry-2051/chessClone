
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>


#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include "maybeResult.hpp"
#include "loadChessAssets.hpp"
#include "pieceMovements.hpp"
#include "stackStack.hpp"
// there is a chess board that conains all the chess pieces 
// there are also chess piece assets 
// we will need to access the chess piece sprites and the chess board at the same time 
// we can use int64 for each piece and xor with int64 as actions 
// we need to read user input 


inline bool checkFlagsQualified(uint8_t state, uint8_t requiredFlags, uint8_t relevantBits) {
    state &= relevantBits;
    requiredFlags &= relevantBits;
    return (state & requiredFlags) == requiredFlags;
}

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
    uint64_t m_pawn_bitshift = 40;
    uint64_t m_piece_bitshift = 56;
    uint64_t m_black_pawns = 0xff00;
    uint64_t m_black_rooks = 0x81;
    uint64_t m_black_knights = 0x42;
    uint64_t m_black_bishops = 0x24;
    uint64_t m_black_queens = 0x8;
    uint64_t m_black_king = 0x10;

uint64_t m_white_pawns = m_black_pawns << m_pawn_bitshift;
    uint64_t m_white_rooks = m_black_rooks << m_piece_bitshift;
    uint64_t m_white_knights = m_black_knights << m_piece_bitshift;
    uint64_t m_white_bishops = m_black_bishops << m_piece_bitshift;
    uint64_t m_white_queens = m_black_queens << m_piece_bitshift;
    uint64_t m_white_king = m_black_king << m_piece_bitshift;

    uint64_t m_black_pieces = m_black_pawns | m_black_rooks | m_black_knights | m_black_bishops | m_black_queens | m_black_king;
    uint64_t m_white_pieces = m_white_pawns | m_white_rooks | m_white_knights | m_white_bishops | m_white_queens | m_white_king;
    
    enum State : uint8_t{
        WhiteTurn = 0b1,
        WhiteCastledRight = 0b10,
        WhiteCastledLeft = 0b100,
        BlackCastledRight = 0b1000,
        BlackCastledLeft = 0b10000,
        HasEnPassant = 0b100000
    };

    uint8_t m_board_state = 0b1;

    using annoying_return_type = std::vector<std::vector<std::pair<int, int>>>;
    
    // std::vector<uint64_t> m_white_queen_moves{std::vector<uint64_t> (8)};
    // std::vector<uint64_t> m_white_pawn_moves{std::vector<uint64_t>(8)};
    // std::vector<uint64_t> m_white_rook_moves{std::vector<uint64_t>(8)};
    // std::vector<uint64_t> m_white_knight_moves{std::vector<uint64_t>(8)};
    // std::vector<uint64_t> m_white_bishop_moves{std::vector<uint64_t>(8)};
    // uint64_t white_king_moves = 0;
    //
    //
    // std::vector<uint64_t> m_black_queen_moves{std::vector<uint64_t> (8)};
    // std::vector<uint64_t> m_black_pawn_moves{std::vector<uint64_t>(8)};
    // std::vector<uint64_t> m_black_rook_moves{std::vector<uint64_t>(8)};
    // std::vector<uint64_t> m_black_knight_moves{std::vector<uint64_t>(8)};
    // std::vector<uint64_t> m_black_bishop_moves{std::vector<uint64_t>(8)};
    // uint64_t black_king_moves = 0;

public:
    chessBoard() = default;

    annoying_return_type piecePositions() {
        annoying_return_type result = {};
        result.push_back(getChessCoordinates(getOnes(m_white_king)));
        result.push_back(getChessCoordinates(getOnes(m_white_queens)));
        result.push_back(getChessCoordinates(getOnes(m_white_bishops)));
        result.push_back(getChessCoordinates(getOnes(m_white_knights)));
        result.push_back(getChessCoordinates(getOnes(m_white_rooks)));
        result.push_back(getChessCoordinates(getOnes(m_white_pawns)));

        result.push_back(getChessCoordinates(getOnes(m_black_king)));
        result.push_back(getChessCoordinates(getOnes(m_black_queens)));
        result.push_back(getChessCoordinates(getOnes(m_black_bishops)));
        result.push_back(getChessCoordinates(getOnes(m_black_knights)));
        result.push_back(getChessCoordinates(getOnes(m_black_rooks)));
        result.push_back(getChessCoordinates(getOnes(m_black_pawns)));
        return result;
    }

    stackStack<uint64_t, 80> boardKnightMoves() {
        bool isWhiteTurn  = m_board_state & WhiteTurn;
        uint64_t knights  = isWhiteTurn ? m_white_knights: m_black_knights;
        uint64_t enemies  = isWhiteTurn ? m_black_pieces : m_white_pieces;
        uint64_t friendly = isWhiteTurn ? m_white_pieces : m_black_pieces;
        
        stackStack knightStack = chessMoves::seperateBitboardIntoStack<10>(knights); 
        stackStack<uint64_t, 80> knightMoveStack({}, 0);

        while (!knightStack.isEmpty()) {
            uint64_t knight = knightStack.pop();
            uint64_t knightMove = chessMoves::knightMove(knight, enemies, friendly);
            auto knightTransform = [knight](uint64_t knightInStack){ return knightInStack| knight; };
            knightMoveStack.pushItems(chessMoves::seperateBitboardIntoStack<8>(knightMove)).stackTransorm(knightTransform);
        }
        return knightMoveStack;
    }

    stackStack<uint64_t, 24> boardPawnMoves() {
        using PawnMoveFunc = uint64_t(*)(uint64_t, uint64_t, uint64_t);
        uint8_t pawnState = ((WhiteTurn & m_board_state) ? 0b10 : 0b00) | ((HasEnPassant & m_board_state) ? 0b01 : 0b00); 

        std::array<PawnMoveFunc, 4> functionLookup = {
            chessMoves::blackPawnMove,
            chessMoves::blackPawnMoveEPP,
            chessMoves::whitePawnMove,
            chessMoves::whitePawnMoveEPP
        };

        bool isWhiteTurn = m_board_state & WhiteTurn;

        uint64_t pawns = isWhiteTurn ? m_white_pawns : m_black_pawns;
        uint64_t enemies = isWhiteTurn ? m_black_pieces : m_white_pieces;
        uint64_t friendly = isWhiteTurn ? m_white_pieces : m_black_pieces;
        
        stackStack pawnStack = chessMoves::seperateBitboardIntoStack<8>(pawns);
        stackStack<uint64_t, 24> pawnMoveStack({}, 0);

        while (!pawnStack.isEmpty()) {
            uint64_t pawn = pawnStack.pop();
            uint64_t pawnMove = functionLookup[pawnState](pawn, enemies, friendly);
            auto makePawnMove = [pawn](uint64_t pawnMoved){ return pawnMoved | pawn; };
            pawnMoveStack.pushItems(chessMoves::seperateBitboardIntoStack<3>(pawnMove).stackTransorm(makePawnMove));
        }
        return pawnMoveStack;
    }

    void generateMoves(){
        auto pawnMoveStack = boardPawnMoves();
    }

    
};

sf::RectangleShape rectFromTopLeftAndBottomRight(sf::Vector2f topLeft, sf::Vector2f bottomRight) {
    sf::Vector2f diff = bottomRight - topLeft;
    sf::RectangleShape returnVal {diff}; // specifies the width and height 
    returnVal.setPosition(topLeft);
    return returnVal;

}

sf::Vector2f positionFromCoords(std::pair<int, int> coords, int edge_padding, int tile_size, int piece_offset) {
    if (coords.first >= 8 || coords.second >= 8 || coords.first < 0 || coords.second < 0) {
        throw std::runtime_error("ERROR TRYING TO PLACE PIECE OUT OF BOARD");
    } 
    return {static_cast<float>(coords.first * tile_size + piece_offset + edge_padding), static_cast<float>(coords.second * tile_size + piece_offset + edge_padding)};
}


std::vector<sf::RectangleShape> createScreenBoarderShapes(int lineWidth, int boardPadding, int edge_padding, int board_square_size, sf::Color boarderColor) {
    // compute the screen boarders

    sf::Vector2f topLeft{
        static_cast<float>(edge_padding - lineWidth - boardPadding), 
        static_cast<float>(edge_padding - lineWidth - boardPadding)
    };

    sf::Vector2f topRight{
        static_cast<float>(edge_padding + lineWidth + boardPadding + board_square_size * 8), 
        static_cast<float>(edge_padding - boardPadding)
    };

    sf::Vector2f bottomLeft{
        static_cast<float>(edge_padding - boardPadding),
        static_cast<float>(edge_padding  + lineWidth + boardPadding + board_square_size * 8)
    };

    sf::Vector2f bottomRightH{
        static_cast<float>(edge_padding + boardPadding + board_square_size * 8 + lineWidth),
        static_cast<float>(edge_padding + boardPadding + board_square_size * 8)
    };

    sf::Vector2f bottomRightV{
        static_cast<float>(edge_padding + boardPadding + board_square_size * 8),
        static_cast<float>(edge_padding + boardPadding + board_square_size * 8 + lineWidth)
    };

    sf::RectangleShape verticalLeft = rectFromTopLeftAndBottomRight(topLeft, bottomLeft);
    sf::RectangleShape verticalRight = rectFromTopLeftAndBottomRight(topRight, bottomRightV);

    sf::RectangleShape horizontalTop = rectFromTopLeftAndBottomRight(topLeft, topRight);
    sf::RectangleShape horizontalBottom = rectFromTopLeftAndBottomRight(bottomLeft, bottomRightH);

    verticalLeft.setFillColor(boarderColor);
    verticalRight.setFillColor(boarderColor);
    horizontalTop.setFillColor(boarderColor);
    horizontalBottom.setFillColor(boarderColor);

    return {verticalLeft, verticalRight, horizontalTop, horizontalBottom};
}

int main()
{
    float scale = 2.0f;

    sf::Color lightColor(204, 212, 224);  // light square color
    sf::Color darkColor(62, 126, 230);    // dark square color
    sf::Color boarderColor(18, 26, 22);
    //
    auto maybeData = loadChessPiecesTexture(scale);

    sf::Texture chessPieceTexture;
    int w,h, ww, wh;
    if (maybeData.exists()) {
        chessPieceTexture = maybeData.m_value.texture;
        w = maybeData.m_value.w;
        h = maybeData.m_value.h;
    } else { return -1; }

    int board_square_size = 50 * scale;
    int edge_padding = 100;
    ww =  8 * board_square_size + 2 * edge_padding;
    wh = ww;

    int pieceHeight = chessPieceTexture.getSize().y/ 2;

    std::cout << "x,y : (" <<  chessPieceTexture.getSize().x << ", " << chessPieceTexture.getSize().y << ")\n";
    
    sf::RectangleShape square{sf::Vector2f(board_square_size, board_square_size)};
    

    std::vector<sf::Sprite> chessPieceSprites = makeChessPieceSprites(chessPieceTexture, pieceHeight);
    std::vector<sf::RectangleShape> boardBoarder = createScreenBoarderShapes(10, 5, edge_padding, board_square_size,  boarderColor);
    chessBoard theChessBoard = chessBoard();

    // 7) Setup SFML window
    sf::RenderWindow window(sf::VideoMode(ww, wh), "NanoSVG + SFML");

    window.setFramerateLimit(60); // Limit to 60 frames per second

    while (window.isOpen())
    {

        sf::Vector2i pos = sf::Mouse::getPosition(window);

        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();

            // ////move back//////
            // if (e.type == sf::Event::KeyPressed)
            //     if (e.key.code == Keyboard::BackSpace) {
            //     }

            /////drag and drop///////
            if (e.type == sf::Event::MouseButtonPressed)
                if (e.mouseButton.button == sf::Mouse::Left) {

                }

            if (e.type == sf::Event::MouseButtonReleased)
                if (e.mouseButton.button == sf::Mouse::Left) {
                }
        }

        window.clear(lightColor);

        // draw the board 
        
        for (auto &b : boardBoarder) {
            window.draw(b);
        }

        int x_ = edge_padding;
        int y_ = edge_padding;
        int x = x_;
        int y = y_;
        for (int c = 0; c < 8; c++) {
            x = board_square_size * c + x_;
            for (int r = 0; r < 8; r++) {
                y = r* board_square_size + y_;
                square.setPosition(x,y);
                square.setFillColor((c + r) % 2 == 0 ? lightColor : darkColor);
                window.draw(square);
            }
        }
        // window.draw(chessPieceSprites[3]);

        std::vector<std::vector<std::pair<int,int>>> pieceCoords = theChessBoard.piecePositions();
        for (int i = 0; i < pieceCoords.size(); i ++) {
            for (std::pair<int, int>& pieceCoord : pieceCoords[i]) {
                sf::Vector2f piecePosition = positionFromCoords(pieceCoord, pieceHeight, board_square_size, (board_square_size - pieceHeight));
                chessPieceSprites[i].setPosition(piecePosition);
                window.draw(chessPieceSprites[i]);
            }
        }
        
        window.display();
    }

    return 0;
}
