#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

#pragma once

enum class ChessPieces : int
{
    Pawn = 0,
    Rook,
    Knight,
    Bishop,
    Queen,
    King
};

enum class CastlingStatus : int
{
    False = 0,
    Short,
    Long,
};

enum class CheckStatus : int
{
    False = 0,
    Check,
    Checkmate
};

enum class CaptureStatus : int
{
    False = 0,
    Capture
};

enum class PromotionStatus : int
{
    False = 0,
    Promotion
};

struct chessSquare
{
    std::optional<uint8_t> file {};
    std::optional<uint8_t> rank {};

};

struct filePos {
    uint8_t pos {};
};

struct rankPos {
    uint8_t pos {};
};

struct chessPieceVal {
    ChessPieces piece {};
};

struct pawnPromotion {
    ChessPieces piece {};
};

enum class EndChar : int {
    End
};

std::string
squareToUCI(const chessSquare& square)
{
    if (!square.file.has_value() || !square.rank.has_value()) {
        throw std::runtime_error(
            "Incomplete chessSquare (missing file or rank)");
    }
    // Map 0 -> 'a', 1 -> 'b', … and 0 -> '1', 1 -> '2', …
    char fileChar = 'a' + static_cast<char>(*square.file);
    char rankChar = '1' + static_cast<char>(*square.rank);
    return std::string() + fileChar + rankChar;
}

struct chessMove
{
    // a valid chess move must have all optional values filled in
    ChessPieces piece                   {ChessPieces::Pawn};
    std::optional<chessSquare> moveFrom {std::nullopt};
    std::optional<chessSquare> moveTo   {std::nullopt};
    CheckStatus checkStatus             {CheckStatus::False};
    ChessPieces pawnPromotion           {ChessPieces::Pawn}; // no promotion
    CastlingStatus castlingStatus       {CastlingStatus::False}; 
    CaptureStatus  captureStatus        {CaptureStatus::False};
    PromotionStatus promotionStatus     {PromotionStatus::False};

    enum lastPushedStatus : uint8_t {None, Rank, File};
    enum squareCurrentlyUpdating : uint8_t {MoveFrom=0, MoveTo=1, Finished};
    lastPushedStatus m_lastPushed {None};
    squareCurrentlyUpdating m_currentSquareAdd {MoveFrom};
    std::array<std::optional<chessSquare>*, 3> m_squaresPointers {&moveFrom, &moveTo};
    bool hasMovementCollapsed {false};

    // minimum precidence status, CastlingStatus::False = 0

    chessMove() = default;

    chessMove(const chessMove& move) : 
        piece(move.piece), moveFrom(move.moveFrom), moveTo(move.moveTo), checkStatus(move.checkStatus), 
        pawnPromotion(move.pawnPromotion), castlingStatus(move.castlingStatus), m_lastPushed(move.m_lastPushed),
        m_currentSquareAdd(move.m_currentSquareAdd) 
    {
        m_squaresPointers = {&moveFrom, &moveTo};
    }

    std::optional<chessMove> pushNewInformation(filePos file) const {
        if (m_currentSquareAdd == Finished) {
            return std::nullopt;
        };

        chessMove newMove = chessMove(*this);

        if (m_lastPushed != None) {
            if (newMove.m_currentSquareAdd == MoveTo) {
                return std::nullopt;
            }

            newMove.m_currentSquareAdd = squareCurrentlyUpdating(static_cast<uint8_t>(newMove.m_currentSquareAdd) + 1); // increment by one
        }

        newMove.ensure(*newMove.m_squaresPointers[newMove.m_currentSquareAdd]);
        (*newMove.m_squaresPointers[newMove.m_currentSquareAdd])->file.emplace(file.pos);
        newMove.m_lastPushed = File;

        return std::optional<chessMove>(newMove);
    }

    std::optional<chessMove> pushNewInformation(struct rankPos rank) const {
        chessMove newMove = chessMove(*this);

        newMove.m_lastPushed = Rank;

        newMove.ensure(*newMove.m_squaresPointers[newMove.m_currentSquareAdd]);

        (*newMove.m_squaresPointers[newMove.m_currentSquareAdd])->rank.emplace(rank.pos);

        return std::optional<chessMove>(newMove);
    }

    std::optional<chessMove> pushNewInformation(struct chessPieceVal p) const {
        if (piece != ChessPieces::Pawn) {
            return std::nullopt;
        }
        chessMove newMove = chessMove(*this);

        newMove.piece = p.piece;

        return std::optional<chessMove>(newMove);
    }

    std::optional<chessMove> pushNewInformation(CastlingStatus status) const {
        if (castlingStatus > status) {
            return std::nullopt;
        }
        chessMove newMove = chessMove(*this);
        
        newMove.castlingStatus = status;

        return std::optional<chessMove>(newMove);
    }

    std::optional<chessMove> pushNewInformation(CheckStatus status) const {
        if (checkStatus != CheckStatus::False) {
            return std::nullopt;
        }
        chessMove newMove = chessMove(*this);

        newMove.checkStatus = status;

        return newMove;
    }

    std::optional<chessMove> pushNewInformation(struct pawnPromotion piece) const {
        if (pawnPromotion != ChessPieces::Pawn) {
            return std::nullopt;
        }
        chessMove newMove = chessMove(*this);

        newMove.pawnPromotion = piece.piece;

        return std::optional<chessMove>(newMove);
    }

    std::optional<chessMove> pushNewInformation(CaptureStatus status) const {
        if (captureStatus > status) {
            return std::nullopt;
        }
        chessMove newMove = chessMove(*this);

        newMove.captureStatus = status;

        return std::optional<chessMove>(newMove);
    }

    std::optional<chessMove> pushNewInformation(PromotionStatus status) const {
        if (promotionStatus > status) {
            return std::nullopt;
        }
        chessMove newMove = chessMove(*this);

        newMove.promotionStatus = status;

        return std::optional<chessMove>(newMove);
    }

    // as a quirk of algebraic chess notaiton we sometimes dont know whether a rank or file exists to 
    // disambiguate which square the piece moves from or whether it represents the desintation square
    // by convention all chess moves have a defined desintation square but can be missing information
    // regarding the square they are moving from, if by ommiting this information the move describes two
    // possible board transitions then the move must include the origin square of the move otherwise it 
    // may be ommited
    
    std::optional<chessMove> promoteMoveToMaximum() const {
        if ((!moveFrom && (!moveTo || (!moveTo->file || !moveTo->rank))) || hasMovementCollapsed) {
            return std::nullopt;
        };
        chessMove newMove = chessMove(*this);

        std::swap(newMove.moveTo, newMove.moveFrom);

        return std::optional<chessMove>(newMove);
    }

    // this takes an optional value and if its a nullopt constructs it with the default constructor 
    // of the type it encloses
    template<typename T>
    std::optional<T>& ensure(std::optional<T>& v) {
        if (!v) {
            v.emplace();
        }
        return v;
    }
};

// Returns a UCI–formatted move string for a given chessMove.
// Example: "e2e4" or, for a pawn promotion, "e7e8q".
std::string
toUCIMove(const chessMove& move)
{
    // For a valid move, both moveFrom and moveTo should be present.
    if (!move.moveFrom.has_value() || !move.moveTo.has_value()) {
        throw std::runtime_error(
            "Incomplete move: 'moveFrom' or 'moveTo' is missing");
    }

    std::string uciMove =
        squareToUCI(*move.moveFrom) + squareToUCI(*move.moveTo);

    // Handle pawn promotion:
    // In UCI promotion moves, you append the promoted piece's letter.
    // Here we assume that if isPawnPromotion is true, then move.piece contains
    // the promotion piece. (Typically, promotion piece letters are: 'q', 'r',
    // 'b', 'n' for queen, rook, bishop, knight.)
    if (move.promotionStatus == PromotionStatus::Promotion) {
        char promoChar = 'q'; // default promotion to queen
        switch (move.pawnPromotion) {
            case ChessPieces::Rook:
                promoChar = 'r';
                break;
            case ChessPieces::Knight:
                promoChar = 'n';
                break;
            case ChessPieces::Bishop:
                promoChar = 'b';
                break;
            case ChessPieces::Queen:
                promoChar = 'q';
                break;
            default:
                promoChar = 'q';
                break; // fallback
        }
        uciMove.push_back(promoChar);
    }

    return uciMove;
}
