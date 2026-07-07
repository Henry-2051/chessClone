#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <bit>
#include <chrono>
#include <csignal>
#include <imgui.h>
#include "helpers.hpp"
#include "imgui-sfml/imgui-SFML.h"
#include <array>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "pieceMovements.hpp"
#include "seperateBitboard.hpp"
#include "stackStack.hpp"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
// #include "maybeResult.hpp"
#include "loadChessAssets.hpp"
#include "chessBoard.h"
#include "chessBoardMovegenSharedDatatypes.h"


enum PieceColor : bool {
    White = true,
    Black = false,
};

// there is a chess board that conains all the chess pieces 
// there are also chess piece assets 
// we will need to access the chess piece sprites and the chess board at the same time 
// we can use int64 for each piece and xor with int64 as actions 
// we need to read user input 


void printUint8_t(uint8_t uint) {
    for (uint8_t i = 0; i < 8; ++i) {
        uint8_t j = 7-i;
        if (uint & (1 << j)) {
            std::cout << '1';
        } else {
            std::cout << '0';
        }
    }
    std::cout << std::endl;
}

void printInt8_t(std::int8_t value) {
    std::uint8_t bits = static_cast<std::uint8_t>(value);

    for (int i = 7; i >= 0; --i) {
        std::cout << ((bits >> i) & 1);
    }

    std::cout << std::endl;
}


bool checkMoveLegal(pieceMovement move, const stackStack218& moveStack) {
    for (const pieceMovement& generated_move : moveStack) {
        // printCustomStruct(generated_move);
        if (generated_move == move) {
            // std::cout << std::format("verified move!!") << std::endl;
            // printCustomStruct(move);
            return true;
        }
    }
    std::cout << std::format("failed to find match among generated moves ({}), inputtedMove", moveStack.currentNumberItems) << std::endl;
    printCustomStruct(move);

    return false;
}



chessBoard applyChessMove(pieceMovement move, const chessBoard& board) {
    bool isWhiteTurn = board_state::WhiteTurn & board.m_board_state;
    chessBoard newBoard {board};
    // second and fourth rank
    uint64_t black_epp_check= 0xff00ff00;
    // seventh and fith rank
    uint64_t white_epp_check= 0xff00ff00000000;
    
    uint64_t* pawns_of_same_color = newBoard.getPiecesByColor(isWhiteTurn);
    uint64_t* pawns_of_opposite_color = newBoard.getPiecesByColor(!isWhiteTurn);

    size_t idx = std::to_underlying(move.pieceType);
    // fields in a struct are layed out sequentially in memory, therefore we can index into our struct as we would an array
    // this is dangerous, illegal, and causes Undefined Behavior
    //
    // todo change representation of pieces to std::array such that we dont have Undefined behaviour 

    bool hasEnPassant = false;

    uint64_t new_pawn_board = pawns_of_same_color[idx] ^ move.movement;

    if (move.pieceType == PieceType::Pawn) {
        uint64_t new_pawn_place = new_pawn_board & move.movement;

        // this means has the pawn advanced 2 places
        if (std::popcount(move.movement & (isWhiteTurn ? white_epp_check : black_epp_check)) == 2) {
            uint64_t pawnPassplaces = (new_pawn_place << 1) | (new_pawn_place >> 1);
            // is there an enemy pawn that can capture en passsant?
            hasEnPassant = pawns_of_opposite_color[idx] & pawnPassplaces;
        }
    } 
    
    if (!hasEnPassant) {
        newBoard.enPassantState = -1;
    } else {
        auto movementFilenum = (std::countr_zero(move.movement) % 8);
        newBoard.enPassantState = !isWhiteTurn ? static_cast<int8_t>(movementFilenum + 16) : static_cast<int8_t>(56 + movementFilenum - 16);
        std::println("new en passant state : {}", newBoard.enPassantState);
        printInt8_t(newBoard.enPassantState);
    }

    pawns_of_same_color[idx] = new_pawn_board;

    for (size_t i = 0; i <= 5; ++i) {
        pawns_of_opposite_color[i] ^= (move.movement & pawns_of_opposite_color[i]);
    }

    if (move.has_second_movement) {
        idx = std::to_underlying(move.secondPieceType);

        pawns_of_same_color[idx] ^= move.second_optional_movement;

        for (size_t i = 0; i <= 5; ++i) {
            pawns_of_opposite_color[i] ^= (move.second_optional_movement & pawns_of_opposite_color[i]);
        }
    }


    uint64_t whiteLeftCorner = 1ULL << 56; 
    uint64_t whiteRightCorner = 1ULL << 63;
    uint64_t blackLeftCorner = 1ULL;
    uint64_t blackRightCorner = 1ULL << 7;

    if (move.pieceType == PieceType::King) {
        newBoard.m_board_state |= (isWhiteTurn ? (board_state::WhiteLostCastlingRightsRight | board_state::WhiteLostCastlingRightsLeft) : (board_state::BlacklostCastlingRightsRight | board_state::BlackLostCastlingRightsLeft));
        if(isWhiteTurn) {
            std::println("white lost all castling rights");
        } else {
            std::println("black lost all castling rights");
        }
    } else if (move.pieceType == PieceType::Rook) {
        if (move.movement & whiteLeftCorner) {
            newBoard.m_board_state |= board_state::WhiteLostCastlingRightsLeft;
            std::println("lost white left corner castling rights");
        } 
        if (move.movement & whiteRightCorner) {
            newBoard.m_board_state |= board_state::WhiteLostCastlingRightsRight;
            std::println("lost white right corner castling rights");
        }  
        if (move.movement & blackLeftCorner) {
            newBoard.m_board_state |= board_state::BlackLostCastlingRightsLeft;
            std::println("lost black left corner castling rights");
        }  
        if (move.movement & blackRightCorner) {
            newBoard.m_board_state |= board_state::BlacklostCastlingRightsRight;
            std::println("lost black right corner castling rights");
        }
    }

    // we can tell whether we end the move with check by generating the next set of moves for the moved piece and when those next moves 

    newBoard.m_board_state ^= board_state::WhiteTurn;

    return newBoard;
}

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

std::optional<std::pair<int, int>> getSquarePosition(const sf::RenderWindow& window, sf::Vector2i pos) {
    pos *= 10;
    pos.x /= window.getSize().x;
    pos.y /= window.getSize().y;
    if (pos.x < 1 || pos.x > 8 || pos.y < 1 || pos.y > 8) {
        return std::nullopt;
    } 

    return {{pos.x-1, pos.y-1}};
}

// this is going to have to communicate with the open window for pawn promotion its going to have to open up an imgui window, or communicate with the main loop
enum class PieceMovementEnum {
    NothingSpecial,
    PawnPromotionMenu,
};

std::pair<std::optional<pieceMovement>, PieceMovementEnum> makePieceMovementFromBoardPosition(const chessBoard& board, std::pair<int,int>from, std::pair<int,int> to) {
    uint64_t move_from = 1ULL << static_cast<size_t>(from.first + 8 * from.second);
    uint64_t move_to =  1ULL << static_cast<size_t>(to.first + 8 * to.second);
    uint64_t move = move_to | move_from;

    bool isWhiteTurn = (board_state::WhiteTurn & board.m_board_state) != 0;

    const uint64_t* pieces = board.getPiecesByColorConst(isWhiteTurn);
    const uint64_t* enemy_pieces = board.getPiecesByColorConst(!isWhiteTurn);

    if (move_to == move_from) 
        return {std::nullopt, PieceMovementEnum::NothingSpecial};

    // check that we arent trying to move into a friendly piece
    for (size_t i = 0; i < 6; ++ i) {
        if ((pieces[i] & move_to) != 0) {
            return {std::nullopt, PieceMovementEnum::NothingSpecial};
        }
    }

    auto returnNonSpecialMove= [&](size_t i)->pieceMovement{
        return {move, 0, PieceType(i), PieceType::King, false};
    };

    auto specialPawnMove = [&]->std::pair<std::optional<pieceMovement>, PieceMovementEnum>{
        uint64_t enemy_pawns = enemy_pieces[0];
        uint64_t triangle_corner = isWhiteTurn ? move_to << 8 : move_to >> 8;
        uint64_t enemy_piece_mask = isWhiteTurn ? board.blackPieces() : board.whitePieces();

        uint64_t enemy_back_rank = isWhiteTurn ? 0xff : static_cast<uint64_t>(0xff) << 56;
        // 4th rank for black and 5th rank for white 
        uint64_t enPassantingRank = isWhiteTurn ? static_cast<uint64_t>(0xff) << 24 : static_cast<uint64_t>(0xff) << 32;

        bool isDiagonalMovement = isWhiteTurn ? (move_from >> 7 & move_to) || (move_from >> 9 & move_to) : (move_from << 7 & move_to) || (move_from << 9 & move_to);

        // these extensive rules are necessary so that we dont create the wrong move, as the code works we check whether this move exists in the move generator
        // but if we generate an en passant move where we should have generated a capture the user will be unable to move the piece on the screen even though the move 
        // should work. This happens despite the move being generated by the move generator.
        // the pawn starts on the en passant rank 4th rank for black and 5th for white
        // the pawn moves diagonally
        // at the shoulder of the pawn there is an enemy pawn 
        // we are moving into empty space and not capturing another piece
        if ((move_from & enPassantingRank) && isDiagonalMovement && (triangle_corner & enemy_pawns) && !(move_to & enemy_piece_mask)) {
            return {{{move | triangle_corner, triangle_corner, PieceType::Pawn, PieceType::Pawn, true}}, PieceMovementEnum::NothingSpecial};
        }

        if (move_to & enemy_back_rank) {
            return {{{move_from, move_to, PieceType::Pawn, PieceType::Pawn, true}}, PieceMovementEnum::PawnPromotionMenu};
        }

        return {{{move, 0, PieceType::Pawn, PieceType::Pawn, false}}, PieceMovementEnum::NothingSpecial};
    };

    auto specialKingMove = [&]->pieceMovement{
        uint64_t startingKingSquare = isWhiteTurn ? static_cast<uint64_t>(0x10) << 56 : 0x10;
        uint64_t longCastlingKingDestination = startingKingSquare >> 2;
        uint64_t shortCastlingKingDesination = startingKingSquare << 2;

        uint64_t arook = isWhiteTurn ? static_cast<uint64_t>(1) << 56 : 1;
        uint64_t hrook = isWhiteTurn ? static_cast<uint64_t>(1) << 63 : 1 << 7;

        pieceMovement defaultReturn {move, 0, PieceType::King, PieceType::Pawn, false};

        if (!(move_from == startingKingSquare)) {
            return defaultReturn;
        }

        if (move_to == longCastlingKingDestination && (arook & pieces[std::to_underlying(PieceType::Rook)]) ) {
            return {move, arook | (startingKingSquare >> 1), PieceType::King, PieceType::Rook, true};
        } 

        if (move_to == shortCastlingKingDesination && (hrook & pieces[std::to_underlying(PieceType::Rook)])){
            return {move, hrook | (startingKingSquare << 1), PieceType::King, PieceType::Rook, true};
        }

        return defaultReturn;
    };

    // return the move if we're actually trying to move a piece, moving some empty space wouldnt make sense;
    for (size_t i = 0; i < 6; ++ i) {
        if ((pieces[i] & move_from) != 0) {
            if (PieceType(i) == PieceType::Pawn)
                return {specialPawnMove()};
            else if (PieceType(i) == PieceType::King)
                return {{specialKingMove()}, PieceMovementEnum::NothingSpecial};
            else
                return {{returnNonSpecialMove(i)}, PieceMovementEnum::NothingSpecial};
        }
    }

    return {std::nullopt, PieceMovementEnum::NothingSpecial};
}

struct windowCtx {
    sf::RenderWindow window;
    sf::Clock clock;
    chessBoard board;
    sf::Texture chessPieceTexture;
    std::tuple<sf::Color, sf::Color, sf::Color> boardColors;
    sf::RectangleShape chessBoardSquare;
    std::vector<sf::Sprite> chessPieceSprites;
    std::vector<sf::RectangleShape> boardBoarder;
    int w, h, ww, wh;
    int edge_padding;
    int board_square_size;
    int pieceHeight;

    windowCtx(std::string_view fenString);
    ~windowCtx();
};

windowCtx::~windowCtx() {
    ImGui::SFML::Shutdown();
    window.close();
}

windowCtx::windowCtx(std::string_view fenString) : board(fenString) {
    float scale = 2.0f;

    sf::Color lightColor(227, 198, 168);  // light square color
    sf::Color darkColor(133, 91, 46);    // dark square color
    sf::Color boarderColor(31, 11, 7);
    boardColors = {lightColor, darkColor, boarderColor};
    //
    auto maybeData = loadChessPiecesTexture(scale);

    if (maybeData.exists()) {
        chessPieceTexture = maybeData.m_value.texture;
        w = maybeData.m_value.w;
        h = maybeData.m_value.h;
    } else { 
        throw std::runtime_error("failed to load chess textures");
    }

    board_square_size = static_cast<int>(50.0f * scale);
    edge_padding = 100;
    ww =  8 * board_square_size + 2 * edge_padding;
    wh = ww;

    pieceHeight = chessPieceTexture.getSize().y/ 2;

    // std::cout << "x,y : (" <<  chessPieceTexture.getSize().x << ", " << chessPieceTexture.getSize().y << ")\n";
    
    chessBoardSquare =  sf::RectangleShape{sf::Vector2f(static_cast<float>(board_square_size), static_cast<float>(board_square_size))};

    chessPieceSprites = makeChessPieceSprites(chessPieceTexture, pieceHeight);
    boardBoarder = createScreenBoarderShapes(10, 5, edge_padding, board_square_size,  boarderColor);

    window.create(sf::VideoMode(static_cast<uint32_t>(ww), static_cast<uint32_t>(wh)), "NanoSVG + SFML");
    window.setFramerateLimit(60); // Limit to 60 frames per second
    bool sucess = ImGui::SFML::Init(window);
    
    if (!sucess) {
        throw std::runtime_error("failed to initialise sfml window");
    }
}

struct userInput {
    bool pawnPromting{false};
    std::pair<std::optional<std::pair<int,int>>, std::optional<std::pair<int,int>>> movePositions{std::nullopt, std::nullopt};
    PieceMovementEnum pawnPromotionState {PieceMovementEnum::NothingSpecial};
    std::optional<pieceMovement> workingOnInputtedMove {std::nullopt};
    std::optional<pieceMovement> stagedForApplicationMove {std::nullopt};
};


userInput collectUserInput(userInput input, windowCtx& w_ctx) {
    if (input.pawnPromotionState == PieceMovementEnum::PawnPromotionMenu) {
        return input;
    }

    sf::Vector2i pos = sf::Mouse::getPosition(w_ctx.window);
    sf::Event e;
    while (w_ctx.window.pollEvent(e)) {
        ImGui::SFML::ProcessEvent(w_ctx.window, e);
        if (e.type == sf::Event::Closed)
            w_ctx.window.close();

        // ////move back//////
        // if (e.type == sf::Event::KeyPressed)
        //     if (e.key.code == Keyboard::BackSpace) {
        //     }

        /////drag and drop///////


        if (e.type == sf::Event::MouseButtonPressed)
            if (e.mouseButton.button == sf::Mouse::Left) {
                // std::cout << std::format("mouse pressed at {}, {}\n", pos.x, pos.y);
                auto res = getSquarePosition(w_ctx.window, pos);
                if (res) {
                    // std::cout << std::format("square position : {}, {}\n", res.value().first, res.value().second);

                    if (input.movePositions.first == std::nullopt) {
                        input.movePositions.first = {res.value()};
                    }
                }

            }

        if (e.type == sf::Event::MouseButtonReleased)
            if (e.mouseButton.button == sf::Mouse::Left) {
                // std::cout << std::format("mouse released at {}, {}\n", pos.x, pos.y);
                auto res = getSquarePosition(w_ctx.window, pos);
                if (res) {
                    // std::cout << std::format("square position : {}, {}\n", res.value().first, res.value().second);
                    
                    if (input.movePositions.first != std::nullopt && input.movePositions.second == std::nullopt) {
                        input.movePositions.second = {res.value()};
                    }
                }
            }
    }

    return input;
}


userInput processUserInput(userInput input, const chessBoard& board) {
    // triggers if the user has clicked and dragged to move a piece, changes the input state
    if (input.movePositions.first.has_value() && input.movePositions.second.has_value()) {
        auto [__enteredMove, __moveCtx] = makePieceMovementFromBoardPosition(board, input.movePositions.first.value(), input.movePositions.second.value());
        input.workingOnInputtedMove= __enteredMove;
        input.pawnPromotionState = __moveCtx;
        input.movePositions.first  = std::nullopt;
        input.movePositions.second = std::nullopt;

    }

    // if theres a move in the buffer (workingOnInputtedMove) and we're not processing a pawn promotion this will decide whether
    // its legal 
    if (input.pawnPromotionState == PieceMovementEnum::NothingSpecial && input.workingOnInputtedMove.has_value()) {
        input.stagedForApplicationMove = input.workingOnInputtedMove;
        input.workingOnInputtedMove = std::nullopt;
    }
    
    return input;
}

userInput consumeStagedMoveVerifyAndApply(userInput input, chessBoard& board) {
    if (input.stagedForApplicationMove.has_value()) {
        auto start = std::chrono::high_resolution_clock::now();
        stackStack218 allMoves = chessMoves::makeAllMoves(board);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::println("time taken to generate moves : {} microseconds", duration.count());
        if (checkMoveLegal(input.stagedForApplicationMove.value(), allMoves)) {
            board = applyChessMove(input.stagedForApplicationMove.value(), board);
        }
        input.stagedForApplicationMove= std::nullopt;
    }
    return input;
}


void renderBoard(windowCtx& w_ctx) {
    w_ctx.window.getSize();

    w_ctx.window.clear(std::get<0>(w_ctx.boardColors));

    // draw the board 
    
    for (auto &b : w_ctx.boardBoarder) {
        w_ctx.window.draw(b);
    }

    int x_ = w_ctx.edge_padding;
    int y_ = w_ctx.edge_padding;
    int x = x_;
    int y = y_;
    for (int c = 0; c < 8; c++) {
        x = w_ctx.board_square_size * c + x_;
        for (int r = 0; r < 8; r++) {
            y = r* w_ctx.board_square_size + y_;
            w_ctx.chessBoardSquare.setPosition(static_cast<float>(x),static_cast<float>(y));
            w_ctx.chessBoardSquare.setFillColor((c + r) % 2 == 0 ? std::get<0>(w_ctx.boardColors) : std::get<1>(w_ctx.boardColors));
            w_ctx.window.draw(w_ctx.chessBoardSquare);
        }
    }
    // window.draw(chessPieceSprites[3]);

    std::vector<std::vector<std::pair<uint32_t,uint32_t>>> pieceCoords = w_ctx.board.piecePositions();
    for (uint32_t i = 0; i < pieceCoords.size(); i ++) {
        for (std::pair<uint32_t, uint32_t>& pieceCoord : pieceCoords[i]) {
            sf::Vector2f piecePosition = positionFromCoords(pieceCoord, w_ctx.pieceHeight, w_ctx.board_square_size, (w_ctx.board_square_size - w_ctx.pieceHeight));
            w_ctx.chessPieceSprites[i].setPosition(piecePosition);
            w_ctx.window.draw(w_ctx.chessPieceSprites[i]);
        }
    }
}

userInput makeImguiWindow(windowCtx& w_ctx, userInput input) {
    if (input.pawnPromotionState != PieceMovementEnum::PawnPromotionMenu) {
        return input;
    }

    // imgui sfml code
    ImGui::SFML::Update(w_ctx.window, w_ctx.clock.restart());

    ImGui::Begin("Pawn promotion piece selection");

    static int selection = -1;
    const std::array<std::string, 6> items = {"Rook", "Knight", "Bishop", "Queen"};
    bool selected = false;
     
    ImGui::BeginListBox("");
    for (int i {0}; i < 4; ++i) {
        const bool is_selected = i == selection;
        if (ImGui::Selectable(items[static_cast<size_t>(i)].c_str(), is_selected)) {
            selection = i;
        }

        if (is_selected) {
            ImGui::SetItemDefaultFocus();
            selected = true;
        }
    }

    if (selected) {
        if (!input.workingOnInputtedMove.has_value())
            throw std::logic_error("Invlaid program state reached, pawn promotion menu and no move we are working on ");

        input.workingOnInputtedMove.value().secondPieceType = PieceType(selection + 1);
        input.pawnPromting = true;
        selection = -1;

        input.stagedForApplicationMove = input.workingOnInputtedMove;
        input.pawnPromotionState = PieceMovementEnum::NothingSpecial;
        input.workingOnInputtedMove = std::nullopt;
    }

    ImGui::EndListBox();

    ImGui::End();

    ImGui::SFML::Render(w_ctx.window);

    return input;
}

int main(int argc, char *argv[])
{
    char myargstring[256];
    auto counter {0uz};
    for (auto arr : std::span(argv, argc)) {
        if (counter == 0) {
            counter ++;
            continue;
        }

        for (auto c : std::string_view(arr)) {
            assert(counter < 256);
            myargstring[counter-1] = c;
            counter ++;
        }   
        assert(counter < 256);
        myargstring[counter-1] = ' ';
        counter ++;
    }
    myargstring[counter-1] = '\0';
    std::string_view fenArgument = argc > 1 ? std::string_view(myargstring) : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    windowCtx w_ctx(fenArgument);

    userInput input_ctx;

    // std::pair<std::optional<std::pair<int,int>>, std::optional<std::pair<int,int>>> movePositions{std::nullopt, std::nullopt};
    // std::pair<std::pair<int, int>, std::pair<int,int>> movePos_definite; 


    while (w_ctx.window.isOpen())
    {
        input_ctx = collectUserInput(input_ctx, w_ctx); 

        input_ctx = processUserInput(input_ctx, w_ctx.board);

        input_ctx = consumeStagedMoveVerifyAndApply(input_ctx, w_ctx.board);

        renderBoard(w_ctx);

        input_ctx = makeImguiWindow(w_ctx, input_ctx);

        if (input_ctx.pawnPromting) {
            input_ctx = consumeStagedMoveVerifyAndApply(input_ctx, w_ctx.board);
            input_ctx.pawnPromting = false;
            renderBoard(w_ctx);
        }

        // std::cout << std::format("square size: {}, piece height : {}, offset : {}\n", board_square_size, pieceHeight, (board_square_size - pieceHeight));
        
        w_ctx.window.display();
    }


    return 0;
}
