
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
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "pieceMovements.hpp"
#include "seperateBitboard.hpp"
#include "stackStack.hpp"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include "maybeResult.hpp"
#include "loadChessAssets.hpp"
#include "boardState.hpp"


enum PieceColor : bool {
    White = true,
    Black = false,
};

// there is a chess board that conains all the chess pieces 
// there are also chess piece assets 
// we will need to access the chess piece sprites and the chess board at the same time 
// we can use int64 for each piece and xor with int64 as actions 
// we need to read user input 

// maybe we'll store the piece movements in a struct of stacks 
struct pieceMovements {
    singleColorChessMoveStack whiteMoves;
    singleColorChessMoveStack blackMoves;
};

struct moveToExecute {
    uint64_t moveBitboard;
    PieceType pieceType;
    PieceColor pieceColor;
};

struct validChessMoves {
    singleColorChessMoveStack moveStack;
    PieceColor colorToMove;
};

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

std::vector<std::pair<uint32_t, uint32_t>> getChessCoordinates(std::vector<int> ones) {
    std::vector<std::pair<uint32_t, uint32_t>>   result = {};
    for (auto p : ones) {
        int row = p / 8;
        int col = p % 8;
        result.push_back({col, row});
    }
    return result;
}
 
struct bitboardMove {
    using movetype = uint64_t;
    movetype b_pawn {0b0};
    movetype b_rook {0b0};
    movetype b_bishop {0b0};
    movetype b_knight {0b0};
    movetype b_queen {0b0};

    movetype w_pawn {0b0};
    movetype w_rook {0b0};
    movetype w_bishop {0b0};
    movetype w_knight {0b0};
    movetype w_queen {0b0};


    void applyMove(chessBoard& board) {
        board.m_black_pawns ^= b_pawn;
        board.m_black_rooks ^= b_rook;
        board.m_black_knights ^= b_rook;
        board.m_black_bishops ^= b_bishop;
        board.m_black_queens ^= b_queen;

        board.m_white_pawns ^= w_pawn;
        board.m_white_rooks ^= w_rook;
        board.m_white_knights ^= w_knight;
        board.m_white_bishops ^= w_bishop;
        board.m_white_queens ^= w_queen;
    }
};

struct chessBoard {
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

    std::array<std::unique_ptr<uint64_t>, 12> m_piece_bitboard_lookup_table = {
        std::make_unique<uint64_t>(m_black_pawns),
        std::make_unique<uint64_t>(m_black_rooks),
        std::make_unique<uint64_t>(m_black_knights),
        std::make_unique<uint64_t>(m_black_bishops),
        std::make_unique<uint64_t>(m_black_queens),
        std::make_unique<uint64_t>(m_black_king),

        std::make_unique<uint64_t>(m_white_pawns),
        std::make_unique<uint64_t>(m_white_rooks),
        std::make_unique<uint64_t>(m_white_knights),
        std::make_unique<uint64_t>(m_white_bishops),
        std::make_unique<uint64_t>(m_white_queens),
        std::make_unique<uint64_t>(m_white_king),
    }; 
    
    uint8_t m_board_state = board_state::WhiteTurn;
    uint32_t m_turn {};
    uint32_t m_last_generated_moves{};

    pieceMovements m_chess_moves;

    using annoying_return_type = std::vector<std::vector<std::pair<uint32_t, uint32_t>>>;
    

public:
    chessBoard () : m_chess_moves(singleColorChessMoveStack({}, 0), singleColorChessMoveStack({}, 0)) {}  

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

    singleColorChessMoveStack& boardKnightMoves(singleColorChessMoveStack& currentMoveStack, uint8_t boardState) {
        bool isWhiteTurn  = boardState & board_state::WhiteTurn;
        uint64_t knights  = isWhiteTurn ? m_white_knights: m_black_knights;
        uint64_t enemies  = isWhiteTurn ? m_black_pieces : m_white_pieces;
        uint64_t friendly = isWhiteTurn ? m_white_pieces : m_black_pieces;
        
        stackStack knightStack = seperateBitboardIntoStack<10>(knights); 

        while (!knightStack.isEmpty()) {
            uint64_t knight = knightStack.pop();
            uint64_t knightMove = chessMoves::knightMove(knight, enemies, friendly);
            currentMoveStack.pushMoves(knight, knightMove, PieceType::Knight);
        }
        return currentMoveStack;
    }

    singleColorChessMoveStack& boardPawnMoves(singleColorChessMoveStack& currentMoveStack, uint8_t boardState) {
        uint8_t pawnState = board_state::mapBoardToPawnState(m_board_state); 

        bool isWhiteTurn = boardState & board_state::WhiteTurn;

        uint64_t pawns = isWhiteTurn ? m_white_pawns : m_black_pawns;
        uint64_t enemies = isWhiteTurn ? m_black_pieces : m_white_pieces;
        uint64_t friendly = isWhiteTurn ? m_white_pieces : m_black_pieces;
        
        stackStack pawnStack = seperateBitboardIntoStack<8>(pawns);

        while (!pawnStack.isEmpty()) {
            uint64_t pawn = pawnStack.pop();
            uint64_t pawnMoves = chessMoves::singlePawnMoveInterface(pawn, enemies, friendly, pawnState);
            currentMoveStack.pushMoves(pawn, pawnMoves, PieceType::Pawn);
        }
        return currentMoveStack;
    }

    // the code duplication is intentional, using templates and guaranteing perfomance is more annoying than just writing 3 functions
    singleColorChessMoveStack& boardRookMoves(singleColorChessMoveStack& currentMoveStack, uint8_t boardState) {
        bool isWhiteTurn = board_state::WhiteTurn & boardState;

        uint64_t rooks = isWhiteTurn ? m_white_rooks : m_black_rooks;
        uint64_t enemies = isWhiteTurn ? m_black_pieces : m_white_pieces;
        uint64_t friendly = isWhiteTurn ? m_white_pieces : m_black_pieces;

        stackStack rookStack = seperateBitboardIntoStack<10>(rooks);

        while (!rookStack.isEmpty()) {
            uint64_t rook = rookStack.pop();
            uint64_t rookMoves = chessMoves::singleRookMove(rook, enemies, friendly);
            currentMoveStack.pushMoves(rook, rookMoves, PieceType::Rook);
        }
        return currentMoveStack;
    }

    singleColorChessMoveStack& boardBishopMoves(singleColorChessMoveStack& currentMoveStack, uint8_t boardState) {
        bool isWhiteTurn = board_state::WhiteTurn & boardState;

        uint64_t bishops = isWhiteTurn ? m_white_bishops: m_black_bishops;
        uint64_t enemies = isWhiteTurn ? m_black_pieces : m_white_pieces;
        uint64_t friendly = isWhiteTurn ? m_white_pieces : m_black_pieces;

        stackStack bishopStack = seperateBitboardIntoStack<10>(bishops);

        while (!bishopStack.isEmpty()) {
            uint64_t bishop = bishopStack.pop();
            uint64_t bishopMoves = chessMoves::singleBishopMove(bishop, enemies, friendly);
            currentMoveStack.pushMoves(bishop, bishopMoves, PieceType::Bishop);
        }
        return currentMoveStack;
    }
    
    singleColorChessMoveStack& boardQueenMoves(singleColorChessMoveStack& currentMoveStack, uint8_t boardState) {
        bool isWhiteTurn = board_state::WhiteTurn & boardState;

        uint64_t queens = isWhiteTurn ? m_white_queens : m_black_queens;
        uint64_t enemies = isWhiteTurn ? m_black_pieces : m_white_pieces;
        uint64_t friendly = isWhiteTurn ? m_white_pieces : m_black_pieces;

        stackStack queenStack = seperateBitboardIntoStack<10>(queens);

        while (!queenStack.isEmpty()) {
            uint64_t queen = queenStack.pop();
            uint64_t queenMoves = chessMoves::singleQueenMove(queen, enemies, friendly);
            currentMoveStack.pushMoves(queen, queenMoves, PieceType::Queen);
        }
        return currentMoveStack;
    }

    singleColorChessMoveStack& boardKingMoves(singleColorChessMoveStack& currentMoveStack, uint64_t boardState) {
        bool isWhiteTurn = board_state::WhiteTurn & boardState;

        uint64_t king = isWhiteTurn ? m_white_king : m_black_king;
        uint64_t enemies = isWhiteTurn ? m_black_pieces : m_white_pieces;
        uint64_t friendly = isWhiteTurn ? m_white_pieces : m_black_pieces;

        uint64_t kingMoves = chessMoves::naieveKingMove(king, enemies, friendly);

        currentMoveStack.pushMoves(king, kingMoves, PieceType::King);

        return currentMoveStack;
    }
    

    void generateMoves(){
        m_chess_moves = {singleColorChessMoveStack({}, 0), singleColorChessMoveStack({}, 0)};
        bool isWhiteTurn = m_board_state & board_state::WhiteTurn;
        singleColorChessMoveStack& attackingMoves = isWhiteTurn ? m_chess_moves.whiteMoves : m_chess_moves.blackMoves;

        attackingMoves = boardPawnMoves(attackingMoves, m_board_state);
        attackingMoves = boardKnightMoves(attackingMoves, m_board_state);
        attackingMoves = boardRookMoves(attackingMoves, m_board_state);
        attackingMoves = boardBishopMoves(attackingMoves, m_board_state);
        attackingMoves = boardQueenMoves(attackingMoves, m_board_state);
        attackingMoves = boardKingMoves(attackingMoves, m_board_state);
    };

    validChessMoves getMoves() {
        if (m_turn != m_last_generated_moves) {
            generateMoves();
            m_last_generated_moves ++;
        };     
        bool isWhiteTurn = board_state::WhiteTurn & m_board_state;
        return isWhiteTurn ? validChessMoves(m_chess_moves.whiteMoves, PieceColor::White) : validChessMoves(m_chess_moves.blackMoves, PieceColor::Black);
    }

    chessBoard& makeMove(moveToExecute theMove) {
        size_t pieceIndex = (theMove.pieceColor ? 6 : 0) + theMove.pieceType;
        *m_piece_bitboard_lookup_table[pieceIndex] ^= theMove.moveBitboard;
        m_turn ++;
        m_board_state ^= board_state::WhiteTurn;
        
        // we will need to to calculate the newly attacked squares
        return *this;
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

    int board_square_size = static_cast<int>(50.0f * scale);
    int edge_padding = 100;
    ww =  8 * board_square_size + 2 * edge_padding;
    wh = ww;

    int pieceHeight = chessPieceTexture.getSize().y/ 2;

    std::cout << "x,y : (" <<  chessPieceTexture.getSize().x << ", " << chessPieceTexture.getSize().y << ")\n";
    
    sf::RectangleShape square{sf::Vector2f(static_cast<float>(board_square_size), static_cast<float>(board_square_size))};
    

    std::vector<sf::Sprite> chessPieceSprites = makeChessPieceSprites(chessPieceTexture, pieceHeight);
    std::vector<sf::RectangleShape> boardBoarder = createScreenBoarderShapes(10, 5, edge_padding, board_square_size,  boarderColor);
    chessBoard theChessBoard = chessBoard();

    // 7) Setup SFML window
    sf::RenderWindow window(sf::VideoMode(static_cast<uint32_t>(ww), static_cast<uint32_t>(wh)), "NanoSVG + SFML");

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
                square.setPosition(static_cast<float>(x),static_cast<float>(y));
                square.setFillColor((c + r) % 2 == 0 ? lightColor : darkColor);
                window.draw(square);
            }
        }
        // window.draw(chessPieceSprites[3]);

        std::vector<std::vector<std::pair<uint32_t,uint32_t>>> pieceCoords = theChessBoard.piecePositions();
        for (uint32_t i = 0; i < pieceCoords.size(); i ++) {
            for (std::pair<uint32_t, uint32_t>& pieceCoord : pieceCoords[i]) {
                sf::Vector2f piecePosition = positionFromCoords(pieceCoord, pieceHeight, board_square_size, (board_square_size - pieceHeight));
                chessPieceSprites[i].setPosition(piecePosition);
                window.draw(chessPieceSprites[i]);
            }
        }
        
        window.display();
    }

    return 0;
}
