#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <csignal>
#include <imgui.h>
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
#include "stackStack.hpp"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include "loadChessAssets.hpp"
#include "chessBoard.h"
#include "engine.h"
#include "engineSharedDatatypes.hpp"


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


std::optional<pieceMovement> checkMoveLegal(pieceMovement move, const stackStack218& moveStack) {
    for (const pieceMovement& generated_move : moveStack) {
        // printCustomStruct(generated_move);
        if (generated_move.compareForSelection(move)) {
            // std::cout << std::format("verified move!!") << std::endl;
            // printCustomStruct(move);
            return generated_move;
        }
    }
    // std::cout << std::format("failed to find match among generated moves ({}), inputtedMove", moveStack.numitems()) << std::endl;
    // printCustomStruct(move);

    return std::nullopt;
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

    if (move_to == move_from) 
        return {std::nullopt, PieceMovementEnum::NothingSpecial};


    PieceType typeOfPieceMoving = board.figureOutTypeOfPieceOnSquare(move_from, isWhiteTurn);
    if (typeOfPieceMoving == PieceType::NotAPiece) {
        return {std::nullopt, PieceMovementEnum::NothingSpecial} ;
    }

    if (typeOfPieceMoving == PieceType::Pawn) {

        uint64_t enemy_back_rank = isWhiteTurn ? 0xff : static_cast<uint64_t>(0xff) << 56;

        if (move_to & enemy_back_rank) {
            PieceType pieceTypeOnMoveTo = board.figureOutTypeOfPieceOnSquare(move_to, !isWhiteTurn);
            auto promotionPlaceHolder = PieceType::NotAPiece;
            if (isWhiteTurn) {
                return {{{move_from, move_to, PieceType::Pawn, PieceType::NotAPiece, promotionPlaceHolder, pieceTypeOnMoveTo}}, PieceMovementEnum::PawnPromotionMenu};
            } else {

                return {{{move_from, move_to, PieceType::NotAPiece, PieceType::Pawn, pieceTypeOnMoveTo, promotionPlaceHolder}}, PieceMovementEnum::PawnPromotionMenu};
            }
        }


    }

    if (isWhiteTurn) {
        return {{{move, 0, typeOfPieceMoving, PieceType::NotAPiece}}, PieceMovementEnum::NothingSpecial};
    } else {
        return {{{move, 0, PieceType::NotAPiece, typeOfPieceMoving}}, PieceMovementEnum::NothingSpecial};
    }
}

struct infoCache {
    std::string qSearchString {""};
};

struct windowCtx {
    sf::RenderWindow window;
    sf::Clock clock;
    guiBoard boardWithExtraStuff;
    sf::Texture chessPieceTexture;
    std::tuple<sf::Color, sf::Color, sf::Color> boardColors;
    sf::RectangleShape chessBoardSquare;
    std::vector<sf::Sprite> chessPieceSprites;
    std::vector<sf::RectangleShape> boardBoarder;
    int w, h, ww, wh;
    int edge_padding;
    int board_square_size;
    int pieceHeight;
    std::vector<pieceMovement> gameHisory;
    chessEngine engineState {};
    bool displayedInfo {false};
    infoCache cachedInfo {};

    bool searchToSetDepth {true};
    int depthSearched {2};
    bool useAlphaBetaPruning {true};

    interfacePrinterState loggerThingy {};
    std::vector<std::string> logsFromLoggerThingy {};

    // basically how many times the user has clicked the undo move button
    size_t movesBackFromTopGameHistory {0};

    chessBoard& board = boardWithExtraStuff.board;

    void applyMove(const pieceMovement& mv) {
        board.applyMoveImpure(mv);
        displayedInfo = false;
        std::cout << board.stringBoard();
    }

    struct guiBoard& updateFen(std::string_view fen) {
        gameHisory.resize(0);
        return boardWithExtraStuff.updateFen(fen);
    }

    void addMoveToHistory(pieceMovement move) {
        if (movesBackFromTopGameHistory == 0)
            gameHisory.push_back(move);
        else {
            gameHisory.resize(gameHisory.size() - movesBackFromTopGameHistory);
            movesBackFromTopGameHistory = 0;

            gameHisory.push_back(move);
        }
    }

    // sometimes we cant undo a move, we want to express this as an expected part of the program rather than crashing
    bool undoMove() {
        if (!(gameHisory.size() - movesBackFromTopGameHistory > 0))
            return false;

        applyMove(gameHisory[gameHisory.size() -1 - movesBackFromTopGameHistory]);
        movesBackFromTopGameHistory ++;
        return true;
    }

    bool redoMove() {
        if (movesBackFromTopGameHistory == 0)
            return false;

        applyMove(gameHisory[gameHisory.size() - movesBackFromTopGameHistory]);
        movesBackFromTopGameHistory --;
        return true;
    }

    windowCtx(std::string_view fenString);
    ~windowCtx();
};

windowCtx::~windowCtx() {
    ImGui::SFML::Shutdown();
    window.close();
}

windowCtx::windowCtx(std::string_view fenString) : boardWithExtraStuff(fenString) {
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
        // if(__enteredMove.has_value()) {
        //     std::println("move created by chess gui :");
        //     __enteredMove.value().printThis();
        //     std::println("\n");
        // }
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

// 
std::pair<userInput, std::optional<pieceMovement>> consumeStagedMoveVerifyAndApply(userInput input, windowCtx& w_ctx) {
    if (input.stagedForApplicationMove.has_value()) {

        // auto start = std::chrono::steady_clock::now();
        stackStack218 allMoves = chessMoves::makeAllMoves(w_ctx.board);
        // auto stop = std::chrono::steady_clock::now();
        // std::println("make moves took {}ns", std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count());
        
        std::optional<pieceMovement> legalCheckResult = checkMoveLegal(input.stagedForApplicationMove.value(), allMoves);
        input.stagedForApplicationMove = std::nullopt;

        if (legalCheckResult.has_value()) {
            w_ctx.applyMove(legalCheckResult.value());
            return {input, legalCheckResult};
        }
    }

    return {input, std::nullopt};
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

userInput makeImguiPawnPromotionWindow(windowCtx& w_ctx, userInput input) {
    if (input.pawnPromotionState != PieceMovementEnum::PawnPromotionMenu) {
        return input;
    }

    // imgui sfml code

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

        if (w_ctx.board.m_board_state & board_state::WhiteTurn) {
            input.workingOnInputtedMove.value().movement2WhiteBB = PieceType(selection + 1);
        } else {
            input.workingOnInputtedMove.value().movement2BlackBB = PieceType(selection + 1);
        }
        std::println("after selecting promotion");
        input.workingOnInputtedMove.value().printThis();
        input.pawnPromting = true;
        selection = -1;

        input.stagedForApplicationMove = input.workingOnInputtedMove;
        input.pawnPromotionState = PieceMovementEnum::NothingSpecial;
        input.workingOnInputtedMove = std::nullopt;
    }

    ImGui::EndListBox();

    ImGui::End();

    return input;
}

userInput makeImguiInfoAndControlWindow(windowCtx& w_ctx, userInput input) {
    ImGui::Begin("Control and state");

    static bool firstRun = true;

    static const auto inptFenSize {256};
    static char inputFen[inptFenSize];
    if (firstRun) {
        firstRun = false;
        w_ctx.boardWithExtraStuff.inputFen.copy(inputFen, inptFenSize);
    }
    ImGui::InputText("Board fen", inputFen, inptFenSize);

    if (ImGui::Button("update Fen")) {
        w_ctx.boardWithExtraStuff.updateFen(std::string_view(inputFen));
    }


    if (ImGui::Button("undo move"))
        w_ctx.undoMove();
    if (ImGui::Button("Redo move"))
        w_ctx.redoMove();


    if (ImGui::TreeNode("board state information")) {
        ImGui::SeparatorText("Board state");

        uint8_t& bstate = w_ctx.board.m_board_state;
        ImGui::BulletText("%s", std::format("WhiteTurn : {}", static_cast<bool>(bstate & board_state::WhiteTurn)).c_str());
        
        bool blackLongCastle = !(bstate & board_state::BlackLostCastlingRightsLeft); // true if black can long castle
        bool blackShorCastle = !(bstate & board_state::BlacklostCastlingRightsRight); 
        bool whiteShorCastle = !(bstate & board_state::WhiteLostCastlingRightsRight); 
        bool whiteLongCastle = !(bstate & board_state::WhiteLostCastlingRightsLeft); 
        
        ImGui::BulletText("%s", std::format("BlackCastling (Long, Short) = ({}, {})", blackLongCastle, blackShorCastle).c_str());
        ImGui::BulletText("%s", std::format("WhiteCastling (Long, Short) = ({}, {})", whiteLongCastle, whiteShorCastle).c_str());
        ImGui::BulletText("%s", std::format("EnPassant state (square we capture into) = {}", w_ctx.board.enPassantState).c_str());

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("chess engine")) {
        swapLogBuffers(w_ctx.logsFromLoggerThingy, w_ctx.loggerThingy);
        for (const auto& l : w_ctx.logsFromLoggerThingy) {
            std::println("{}", l);
        }
        w_ctx.logsFromLoggerThingy.resize(0);
        
        if (ImGui::Button("Stop Engine")) {
            w_ctx.engineState.stopSearching();
        };

        if (ImGui::Button("Load Board")) {
            w_ctx.engineState.loadPosition(w_ctx.board);
        }

        if (ImGui::Button("Allocate transpositon table (16MB)")) {
            w_ctx.engineState.allocateTranspositionTable(16);
        }

        ImGui::Checkbox("search to finite depth", &w_ctx.searchToSetDepth);

        ImGui::InputInt("search depth", &w_ctx.depthSearched);

        if (ImGui::Button("Start searching")) {
            if (w_ctx.searchToSetDepth) {
                w_ctx.engineState.startSearch(&w_ctx.loggerThingy, w_ctx.depthSearched, w_ctx.useAlphaBetaPruning);
            } else {
                w_ctx.engineState.startSearch(&w_ctx.loggerThingy, std::nullopt, w_ctx.useAlphaBetaPruning);
            }
        }

        auto answer = w_ctx.engineState.getAnswer();
        if (answer.has_value()){
            ImGui::BulletText("%s", std::format("BestMove {}, Eval {} centipawns", w_ctx.board.uciStringMove(answer->bestMove), answer->eval).c_str());
            ImGui::BulletText("%s", std::format("thinking: {}, search depth : {}", w_ctx.engineState.m_isthinking, w_ctx.engineState.m_sharedAnswer.currentDepth).c_str());
        }
        else 
            ImGui::BulletText("Engine not loaded");

        if (!w_ctx.displayedInfo) {
            w_ctx.displayedInfo = true;
            w_ctx.cachedInfo.qSearchString = std::format("quiessence search eval {}", chessEngine::quiessenceSearch(w_ctx.board));
            ImGui::BulletText("%s", w_ctx.cachedInfo.qSearchString.c_str());
        } else {
            ImGui::BulletText("%s", w_ctx.cachedInfo.qSearchString.c_str());
        }

        if (ImGui::Button("Play best engine move")) {
            auto maybeAns = w_ctx.engineState.getAnswer();
            if (maybeAns.has_value()) {
                auto mv = maybeAns->bestMove;
                w_ctx.applyMove(mv);
                w_ctx.addMoveToHistory(mv);

                w_ctx.displayedInfo = false;
                w_ctx.engineState.stopSearching();
                w_ctx.engineState.loadPosition(w_ctx.board);
                
                w_ctx.engineState.startSearch();
            }
        }

        if (ImGui::Button("Clear engine state / reset")) {
            w_ctx.engineState.reset();
        }

        ImGui::Checkbox("alpha beta pruning", &w_ctx.useAlphaBetaPruning);

        ImGui::TreePop();
    }

    ImGui::End();

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

        {
            // we want to know the move that has been made so we can add it to the game history
            auto res = consumeStagedMoveVerifyAndApply(input_ctx, w_ctx);
            input_ctx = res.first;;
            if(res.second.has_value()) {
                // if (w_ctx.engineState.m_answer.has_value()) {
                //     w_ctx.engineState.loadPosition(w_ctx.board);
                //     w_ctx.engineState.startSearch();
                // }
                w_ctx.displayedInfo = false;
                w_ctx.addMoveToHistory(res.second.value());
            }
        }

        renderBoard(w_ctx);

        ImGui::SFML::Update(w_ctx.window, w_ctx.clock.restart());

        input_ctx = makeImguiPawnPromotionWindow(w_ctx, input_ctx);

        input_ctx = makeImguiInfoAndControlWindow(w_ctx, input_ctx);

        ImGui::SFML::Render(w_ctx.window);

        // draw over everything in the same frame if promotion occurs and display the new piece on the squre
        if (input_ctx.pawnPromting) {
            {
                auto res = consumeStagedMoveVerifyAndApply(input_ctx, w_ctx);
                input_ctx = res.first;;
                if(res.second.has_value())
                    w_ctx.addMoveToHistory(res.second.value());
            }

            input_ctx.pawnPromting = false;
            renderBoard(w_ctx);
        }

        // std::cout << std::format("square size: {}, piece height : {}, offset : {}\n", board_square_size, pieceHeight, (board_square_size - pieceHeight));
        
        w_ctx.window.display();
    }

    return 0;
}
