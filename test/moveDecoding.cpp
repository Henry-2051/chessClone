#include "../src/stackStack.hpp"
#include <chrono>
#include <array>
#include <cctype>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include "readTextFile.hpp"
#include "internalMoveRepresentation.hpp"

enum class AlgebraicChessInput : int {
    FilePosition          = 0,
    RankPosition          = 1,
    PromotableChessPiece  = 2,
    Capture               = 3,
    Check                 = 4,
    SpaceOrNewLine        = 5,
    Promotion             = 6,
    CastlingOh            = 7,
    King                  = 8,
    CheckMate             = 9,
    CastlingDash          = 10
};
// this part of the code relates to the state machine

// Files – the board columns, represented by the characters a–h.
enum class FilePositionSM : int {
    A = 0, B, C, D, E, F, G, H
};

// Ranks – the board rows, represented by digits 1–8.
enum class RankPositionSM : int {
    One = 0, Two, Three, Four, Five, Six, Seven, Eight
};

// Chess pieces – note that in standard algebraic notation a pawn is often omitted.
enum class ChessPieceSM : int {
    Queen,
    Rook,
    Bishop,
    Knight
};

enum class CheckOrMateTokenSM : int {
    Check,
    Checkmate,
};

enum class SpaceOrNewLineSM : int {
    Space,
    NewLine
};

enum class CastlingSM : int {
    Minus,
    Oh
};

// Now we fill a single unordered_map that maps from a string token to the corresponding variant.
std::map<char, AlgebraicChessInput> charToTokenType {
    // Files (columns)
    {'a', AlgebraicChessInput::FilePosition},
    {'b', AlgebraicChessInput::FilePosition},
    {'c', AlgebraicChessInput::FilePosition},
    {'d', AlgebraicChessInput::FilePosition},
    {'e', AlgebraicChessInput::FilePosition},
    {'f', AlgebraicChessInput::FilePosition},
    {'g', AlgebraicChessInput::FilePosition},
    {'h', AlgebraicChessInput::FilePosition},

    // Ranks (rows)
    {'1', AlgebraicChessInput::RankPosition},
    {'2', AlgebraicChessInput::RankPosition},
    {'3', AlgebraicChessInput::RankPosition},
    {'4', AlgebraicChessInput::RankPosition},
    {'5', AlgebraicChessInput::RankPosition},
    {'6', AlgebraicChessInput::RankPosition},
    {'7', AlgebraicChessInput::RankPosition},
    {'8', AlgebraicChessInput::RankPosition},

    // promotable Chess pieces
    {'Q', AlgebraicChessInput::PromotableChessPiece},
    {'R', AlgebraicChessInput::PromotableChessPiece},
    {'B', AlgebraicChessInput::PromotableChessPiece},
    {'N', AlgebraicChessInput::PromotableChessPiece},

    // the king
    {'K', AlgebraicChessInput::King},

    // capture 
    {'x', AlgebraicChessInput::Capture},

    // Special tokens
    {'+', AlgebraicChessInput::Check},

    {'#', AlgebraicChessInput::CheckMate},

    //  SpaceOrNewLine
    {' ', AlgebraicChessInput::SpaceOrNewLine},
    {'\n',AlgebraicChessInput::SpaceOrNewLine},

    // pawn promotion
    {'=', AlgebraicChessInput::Promotion},

    // Castling
    {'O', AlgebraicChessInput::CastlingOh},
    {'-', AlgebraicChessInput::CastlingDash}
};

enum class S : int {
    _=0,     // Invalid sequence entered Error                                
    S=1,     // start              
    F=2,     // start input file
    PR=3,    // named piece then rank
    FR=4,    // start input file, rank
    P =5,    // chess piece  inputted
    PC=6,    // named piece then capture
    PF=7,    // named piece then file 
    PFR=8,   // named piece then rank
    CH=9,    // turn ends in check
    CM=10,   // turn ends in checkmate
    PX=11,   // Pawn capture
    PXF=12,  // Pawn capture Inputted file
    PXFR=13, // Pawn capture inputted File then rank
    PP1=14,  // state 1 of pawn promotion
    PP2=15,  // state 2 of pawn promotion
    CS1=16,  // first castling symbol recieved
    CS2=17,  // second castling symbol recieved
    CS3=18,  // third castling symbol recieved, short
    CS4=19,  // fourth castling symbol recieved
    CS5=20,  // fith castling symbol recieved, long  
    PFD=21,  // named piece destination file
    PFRD=22  // named piece desintation rank
};


constexpr int Num_states = 23;
constexpr int Num_token_types = 11;

// this is a beginning but a better way to structure this would be to define a map between states for every input token type and then use constexpr
// to automatically generate the transition matrix, this way you keep the performance of the transition matrix while having the flexability of a mapping
// this would make it easier to mkae revisions to the state machine because with this approach you dont need to specify each state transition just the 
// ones that have some special behavior, you also dont have to add new rows and collums for each new state / input token.

// all games are valid that dont end in the error state, if no other conditions the winner is the last player to make a move. 
// also a game can end in a draw if repetition, it is possible to decude the maximally specific game moves and if these show a repetition then 
// the game ends in a draw.


// FilePosition          = 0,
// RankPosition          = 1,
// PromotableChessPiece  = 2,
// Capture               = 3,
// Check                 = 4,
// SpaceOrNewLine        = 5,
// Promotion             = 6,
// CastlingOh            = 7,
// King                  = 8,
// CheckMate             = 9,
// CastlingDash          = 10


// row is current state, collumn is token type
constexpr std::array<std::array<S, Num_token_types>, Num_states> stateTransitionMatrix {{
//  FPos       RankPos   PromP     Capture   check     S/NL      Prom      ( O )     King      Checkmate ( - )
    {S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_  }, // ERR  0
    {S::F,     S::_,     S::P,     S::_,     S::_,     S::S,     S::_,     S::CS1,   S::P,     S::_,     S::_  }, // S    1
    {S::_,     S::FR,    S::_,     S::PX,    S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_  }, // F    2
    {S::PFD,   S::_,     S::_,     S::PC,    S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_  }, // PR   2
    {S::_ ,    S::_,     S::_,     S::_,     S::CH,    S::S,     S::PP1,   S::_,     S::_,     S::CM ,   S::_  }, // FR   4
    {S::PF,    S::PR,    S::_,     S::PC,    S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_  }, // P    5
    {S::PFD,   S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_  }, // PC   6
    {S::PFD,   S::PFR,   S::_,     S::PC,    S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_  }, // PF   7
    {S::PFD,   S::_,     S::_,     S::PC,    S::CH,    S::S,     S::_,     S::_,     S::_,     S::CM,    S::_  }, // PFR  8
    {S::_,     S::_,     S::_,     S::_,     S::_,     S::S  ,   S::_,     S::_,     S::_,     S::_,     S::_  }, // CH   9
    {S::_,     S::_,     S::_,     S::_,     S::_,     S::CM ,   S::_,     S::_,     S::_,     S::_,     S::_  }, // CM   10
    {S::PXF,   S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_  }, // PX   11
    {S::_,     S::PXFR,  S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_  }, // PXF  12
    {S::_,     S::_,     S::_,     S::_,     S::CH ,   S::S,     S::PP1,   S::_,     S::_,     S::CM ,   S::_  }, // PXFR 13
    {S::_,     S::_,     S::PP2,   S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_  }, // PP1  14
    {S::_,     S::_,     S::_,     S::_,     S::CH ,   S::S,     S::_,     S::_,     S::_,     S::CM ,   S::_  }, // PP2  15
    {S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::CS2}, // CS1  16
    {S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::CS3,   S::_,     S::_,     S::_  }, // CS2  17
    {S::_,     S::_,     S::_,     S::_,     S::_,     S::S  ,   S::_,     S::_,     S::_,     S::_,     S::CS4}, // CS3  18
    {S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::CS5,   S::_,     S::_,     S::_  }, // CS4  19
    {S::_,     S::_,     S::_,     S::_,     S::_,     S::S  ,   S::_,     S::_,     S::_,     S::_,     S::_  }, // CS5  20
    {S::_,     S::PFRD,  S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_,     S::_  }, // PFD  21
    {S::_,     S::_,     S::_,     S::_,     S::CH,    S::S,     S::_,     S::_,     S::_,     S::CM,    S::_  }  // PFRD 22
}};

std::string stateToString(S state) {
    switch (state) {
        case S::_:     return "ERR (Invalid sequence entered)";
        case S::S:     return "S (start)";
        case S::F:     return "F (start input file)";
        case S::PR:    return "PR (named piece then rank)";
        case S::FR:    return "FR (start input file, rank)";
        case S::P:     return "P (chess piece inputted)";
        case S::PC:    return "PC (named piece then capture)";
        case S::PF:    return "FD (named piece file)";
        case S::PFR:   return "FRD (named piece then file, rank)";
        case S::CH:    return "CH (turn ends in check)";
        case S::CM:    return "CM (turn ends in checkmate)";
        case S::PX:    return "PX (pawn capture)";
        case S::PXF:   return "PXF (inputted file)";
        case S::PXFR:  return "PXFR (inputted file then rank)";
        case S::PP1:   return "PP1 (pawn promotion state 1)";
        case S::PP2:   return "PP2 (pawn promotion state 2)";
        case S::CS1:   return "CS1 (first castling symbol received)";
        case S::CS2:   return "CS2 (second castling symbol received)";
        case S::CS3:   return "CS3 (third castling symbol received, short)";
        case S::CS4:   return "CS4 (fourth castling symbol received)";
        case S::CS5:   return "CS5 (fifth castling symbol received, long)";
        case S::PFD:   return "PFD (named piece destination file)";
        case S::PFRD:  return "PFRD (named piece desintation rank, file)";
        default:       return "Unknown state";
    }
}

std::string algebraicInputToString(AlgebraicChessInput ai) {
    switch (ai) {
        case AlgebraicChessInput::FilePosition         : return "File Position a..h";
        case AlgebraicChessInput::RankPosition         : return "Rank Position 1..8";
        case AlgebraicChessInput::PromotableChessPiece : return "Rook | Knight | Bishop | Queen";
        case AlgebraicChessInput::Capture              : return "Capture symbol   x";
        case AlgebraicChessInput::Check                : return "Check symbol     +";
        case AlgebraicChessInput::SpaceOrNewLine       : return "S/NL";
        case AlgebraicChessInput::Promotion            : return "Promotion symbol =";
        case AlgebraicChessInput::CastlingOh           : return "Castling symbol  O";
        case AlgebraicChessInput::King                 : return "The King K";
        case AlgebraicChessInput::CheckMate            : return "Checkmate symbol #";
        case AlgebraicChessInput::CastlingDash         : return "Castling symbol  -";
        default                                        : return "Unknown AlgebraicChessInput";
    }
}

using outputChange = std::variant<chessPieceVal, filePos, rankPos, CheckStatus, pawnPromotion, CastlingStatus, CaptureStatus, PromotionStatus>;

outputChange filePosition(char fp) {
    uint8_t pos = static_cast<uint8_t>(std::toupper(fp) - 'A');
    return filePos(pos);
}

outputChange rankPosition(char rp) {
    uint8_t pos = static_cast<uint8_t>(rp - '1');
    return rankPos(pos);
}

outputChange makeChessPieceChange(char cp) {
    ChessPieces piece = std::map<char, ChessPieces>(
        {{'R', ChessPieces::Rook}, {'K', ChessPieces::Knight}, {'B', ChessPieces::Bishop}, 
         {'Q', ChessPieces::Queen}, {'K', ChessPieces::King}}) [cp];
    return chessPieceVal(piece);
}

using outputFunction = std::function<outputChange(char)>;

std::map<AlgebraicChessInput, std::optional<std::variant<outputFunction, EndChar>>> outputMap {
    {AlgebraicChessInput::FilePosition,         outputFunction(filePosition)    },
    {AlgebraicChessInput::RankPosition,         outputFunction(rankPosition)    },
    {AlgebraicChessInput::PromotableChessPiece, outputFunction(makeChessPieceChange)},
    {AlgebraicChessInput::Capture,              outputFunction([](char _){return CaptureStatus::Capture; }),},
    {AlgebraicChessInput::Check,                outputFunction([](char _){return CheckStatus::Check; })},
    {AlgebraicChessInput::SpaceOrNewLine,       EndChar::End},
    {AlgebraicChessInput::Promotion,            outputFunction([](char _){return PromotionStatus::Promotion; })},
    {AlgebraicChessInput::CastlingOh,           std::nullopt                    },
    {AlgebraicChessInput::King,                 outputFunction(makeChessPieceChange)},
    {AlgebraicChessInput::CheckMate,            outputFunction([](char _){return CheckStatus::Checkmate; })},
    {AlgebraicChessInput::CastlingDash,         std::nullopt                    }
};


std::array<std::optional<std::variant<outputFunction, EndChar>>, 11> outputArray{{
    outputFunction(filePosition),                                     // Index 0: FilePosition
    outputFunction(rankPosition),                                     // Index 1: RankPosition
    outputFunction(makeChessPieceChange),                             // Index 2: PromotableChessPiece
    outputFunction([](char _){ return CaptureStatus::Capture; }),     // Index 3: Capture
    outputFunction([](char _){ return CheckStatus::Check; }),         // Index 4: Check
    EndChar::End,                                                     // Index 5: SpaceOrNewLine
    outputFunction([](char _){ return PromotionStatus::Promotion; }), // Index 6: Promotion
    std::nullopt,                                                     // Index 7: CastlingOh
    outputFunction(makeChessPieceChange),                             // Index 8: King
    outputFunction([](char _){ return CheckStatus::Checkmate; }),     // Index 9: CheckMate
    std::nullopt,                                                     // Index 10: CastlingDash
}};

std::function<void(outputChange)> printOutputChange = [](outputChange oc) {
    std::visit([](auto&& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr(std::is_same_v<T, chessPieceVal>) {
            std::cout << "chessPieceVal: { piece: " << static_cast<int>(val.piece) << " }";
        } else if constexpr(std::is_same_v<T, filePos>) {
            std::cout << "filePos: { pos: " << static_cast<int>(val.pos) << " }";
        } else if constexpr(std::is_same_v<T, rankPos>) {
            std::cout << "rankPos: { pos: " << static_cast<int>(val.pos) << " }";
        } else if constexpr(std::is_same_v<T, CheckStatus>) {
            std::cout << "CheckStatus: { ";
            if(val == CheckStatus::False)
                std::cout << "False";
            else if(val == CheckStatus::Check)
                std::cout << "Check";
            else if(val == CheckStatus::Checkmate)
                std::cout << "Checkmate";
            std::cout << " }";
        } else if constexpr(std::is_same_v<T, pawnPromotion>) {
            std::cout << "pawnPromotion: { piece: " << static_cast<int>(val.piece) << " }";
        } else if constexpr(std::is_same_v<T, CastlingStatus>) {
            std::cout << "CastlingStatus: { ";
            if(val == CastlingStatus::False)
                std::cout << "False";
            else if(val == CastlingStatus::Short)
                std::cout << "Short";
            else if(val == CastlingStatus::Long)
                std::cout << "Long";
            std::cout << " }";
        } else if constexpr(std::is_same_v<T, CaptureStatus>) {
            std::cout << "CaptureStatus: { ";
            if(val == CaptureStatus::False)
                std::cout << "False";
            else if(val == CaptureStatus::Capture)
                std::cout << "Capture";
            std::cout << " }";
        } else if constexpr(std::is_same_v<T, PromotionStatus>) {
            std::cout << "PromotionStatus: { ";
            if(val == PromotionStatus::False)
                std::cout << "False";
            else if(val == PromotionStatus::Promotion)
                std::cout << "Promotion";
            std::cout << " }";
        }
    }, oc);
};

using outputFunctionFunctionType = std::function<std::optional<std::variant<outputFunction, EndChar>>(AlgebraicChessInput)>;

using stateTransitionFunction = std::function<S(std::pair<S, AlgebraicChessInput>)>;

using DFAMachineType = std::tuple<S, std::string, stateTransitionFunction, outputFunctionFunctionType>;

using DFAFunction = std::function<std::pair<std::vector<chessMove>, S>(DFAMachineType)>;

std::optional<std::variant<outputFunction, EndChar>> outputFunctionFunction(AlgebraicChessInput inpt) {
    return outputArray[static_cast<size_t>(inpt)];
}

S chessTransitionFunction(std::pair<S, AlgebraicChessInput> input){
    S currentState = input.first;
    AlgebraicChessInput tokenType = input.second;

    auto getEnumIndex = [](auto enumClassWithUnderlyingInt) { return static_cast<size_t>(std::to_underlying(enumClassWithUnderlyingInt)); };

    return stateTransitionMatrix[getEnumIndex(currentState)][getEnumIndex(tokenType)];
}

std::pair<std::vector<chessMove>, S> decodeChessGame(S initialState, std::string inputString, 
                                                 stateTransitionFunction chessStateTransitionFunction, 
                                                 outputFunctionFunctionType chessOutputFunctionMap) {
    S currentState{initialState};
    std::vector<chessMove> gameMoves{};

    chessMove workingOnChessMove{};

    for (char myC : inputString) {
        AlgebraicChessInput tokenType = charToTokenType.at(myC);
        // std::cout << std::to_underlying(tokenType) << "\n";

        S nextState = chessStateTransitionFunction({currentState, tokenType});
        // std::optional<std::variant<chessMove, endMove>> chessMoveShard = chessOutputFunction(myC);

        // if (currentState == S::_ && nextState == S::_) {
        //     std::cout << "E->";
        // } else {
        //     std::cout << stateToString(nextState) << "\n";
        // }
        //
        // if(nextState == S::_) {
        //     std::cout << "Erroneous symbol : " << algebraicInputToString(tokenType) << "\n";
        // }

        auto processOutputFunction = [&workingOnChessMove, myC](outputFunction outFunc)->chessMove{
            outputChange outChange = outFunc(myC);

            // printOutputChange(outChange);

            std::optional<chessMove> newMove = std::visit(
                [&workingOnChessMove](auto&& info)->std::optional<chessMove> {
                    return workingOnChessMove.pushNewInformation(info);
                },
                outChange);

            chessMove currentMove = newMove
                        .and_then([](chessMove& newMoveV) { return std::optional<chessMove>(newMoveV); })
                        .or_else([&workingOnChessMove]    { return std::optional<chessMove>(workingOnChessMove); })
                        .value();

            return currentMove;
        };

        std::optional<std::variant<outputFunction, EndChar>> outFunc = outputFunctionFunction(tokenType);

        // workingOnChessMove = processOutputFunction(std::get<outputFunction>(outFunc));   
        if (outFunc != std::nullopt && nextState != S::_) {
            auto outFuncOrEndVal = outFunc.value();
            if (std::holds_alternative<EndChar>(outFuncOrEndVal)) {
                gameMoves.push_back(workingOnChessMove); 
                workingOnChessMove = {};
            } else {
                workingOnChessMove = processOutputFunction(std::get<outputFunction>(outFuncOrEndVal));   
            } 
        }

        currentState = nextState;

        if (currentState == S::_) {
            break;
        }
    }

    return std::pair(gameMoves, currentState);
};


int main () {
    std::string chessGame = readFromFile("chessTestGame2.chess");
    std::cout <<"parsing chess game : " << chessGame << "\n";

    auto startTime = std::chrono::steady_clock::now();

    std::pair<std::vector<chessMove>, S> parseResult = decodeChessGame(S::S, chessGame, chessTransitionFunction, outputFunctionFunction );

    auto endTime = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> elapsedTime = endTime - startTime;
    std::cout << "decodeChessGame took " << elapsedTime.count() << " ms.\n";

    std::vector<chessMove> moves = parseResult.first;
    S                      finalState = parseResult.second;

    std::cout << "Final state : " << stateToString(finalState) << "\n";
    
    // for (const chessMove& move : moves) {
    //     std::cout << toUCIMove(move) << "\n";
    // }
    return 0;
}
