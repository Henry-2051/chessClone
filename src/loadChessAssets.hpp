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
