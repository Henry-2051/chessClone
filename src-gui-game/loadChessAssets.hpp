#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg/src/nanosvg.h"/// SVG parsing
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/src/nanosvgrast.h"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include "maybeResult.hpp"

struct sfTexandWidthAndHeight {
    sf::Texture texture;
    int w;
    int h;
};

inline std::vector<sf::Sprite> makeChessPieceSprites(sf::Texture& chessPieceTexture, int pieceHeight) {

    sf::Vector2i pieceSize{pieceHeight, pieceHeight};
    // White pieces - top row (y = 0)
    sf::Sprite white_king(chessPieceTexture);
    white_king.setTextureRect({{0 * pieceHeight, 0}, pieceSize});

    sf::Sprite white_queen(chessPieceTexture);
    white_queen.setTextureRect({{1 * pieceHeight, 0}, pieceSize});

    sf::Sprite white_bishop(chessPieceTexture);
    white_bishop.setTextureRect({{2 * pieceHeight, 0}, pieceSize});

    sf::Sprite white_knight(chessPieceTexture);
    white_knight.setTextureRect({{3 * pieceHeight, 0}, pieceSize});

    sf::Sprite white_rook(chessPieceTexture);
    white_rook.setTextureRect({{4 * pieceHeight, 0}, pieceSize});

    sf::Sprite white_pawn(chessPieceTexture);
    white_pawn.setTextureRect({{5 * pieceHeight, 0}, pieceSize});

    // Black pieces - bottom row (y = pieceHeight)
    sf::Sprite black_king(chessPieceTexture);
    black_king.setTextureRect({{0 * pieceHeight, pieceHeight}, pieceSize});

    sf::Sprite black_queen(chessPieceTexture);
    black_queen.setTextureRect({{1 * pieceHeight, pieceHeight}, pieceSize});

    sf::Sprite black_bishop(chessPieceTexture);
    black_bishop.setTextureRect({{2 * pieceHeight, pieceHeight}, pieceSize});

    sf::Sprite black_knight(chessPieceTexture);
    black_knight.setTextureRect({{3 * pieceHeight, pieceHeight}, pieceSize});

    sf::Sprite black_rook(chessPieceTexture);
    black_rook.setTextureRect({{4 * pieceHeight, pieceHeight}, pieceSize});

    sf::Sprite black_pawn(chessPieceTexture);
    black_pawn.setTextureRect({{5 * pieceHeight, pieceHeight}, pieceSize});

    return {white_king, white_queen, white_bishop, white_knight, white_rook, white_pawn, black_king, black_queen, black_bishop, black_knight, black_rook, black_pawn};
}
inline maybeResult<sfTexandWidthAndHeight> loadChessPiecesTexture(float scale = 4.0f) {

    const char* filename = "../chess_assets.svg";
    const float dpi = static_cast<float>(96 * 4);

    // 1) Parse SVG
    NSVGimage* image = nsvgParseFromFile(filename, "px", dpi);
    if (!image) {
        std::cerr << "Could not open SVG image: " << filename << std::endl;
        return maybeResult<sfTexandWidthAndHeight>();
    }

    int w = static_cast<int>(image->width)*scale;
    int h = static_cast<int>(image->height)*scale;

    // 2) Create rasterizer
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        std::cerr << "Could not create rasterizer.\n";
        nsvgDelete(image);
        return maybeResult<sfTexandWidthAndHeight>();
    }

    // 3) Prepare pixel buffer for rasterized image (RGBA)
    std::vector<unsigned char> img(w * h * 4);

    // 4) Rasterize SVG to RGBA pixel buffer
    nsvgRasterize(rast, image, 0, 0, scale, img.data(), w, h, w * 4);

    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    // 5) Create SFML texture and upload pixels
    sf::Texture texture;
    if (!texture.create(w, h)) {
        std::cerr << "Failed to create SFML texture\n";
        return maybeResult<sfTexandWidthAndHeight>();
    }

    texture.update(img.data());
    auto value = sfTexandWidthAndHeight(texture, w, h);
    return maybeResult<sfTexandWidthAndHeight>(value);
}
