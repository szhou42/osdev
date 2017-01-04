#include <blend.h>

uint32_t blend_colors(uint32_t color1, uint32_t color2) {
    uint32_t alpha1 = GET_ALPHA(color1);
    uint32_t red1 = GET_RED(color1);
    uint32_t green1 = GET_GREEN(color1);
    uint32_t blue1 = GET_BLUE(color1);

    uint32_t alpha2 = GET_ALPHA(color2);
    uint32_t red2 = GET_RED(color2);
    uint32_t green2 = GET_GREEN(color2);
    uint32_t blue2 = GET_BLUE(color2);

    uint32_t r = (uint32_t)((alpha1 * 1.0 / 255) * red1);
    uint32_t g = (uint32_t)((alpha1 * 1.0 / 255) * green1);
    uint32_t b = (uint32_t)((alpha1 * 1.0 / 255) * blue1);

    r = r + (((255 - alpha1) * 1.0 / 255) * (alpha2 * 1.0 / 255)) * red2;
    g = g + (((255 - alpha1) * 1.0 / 255) * (alpha2 * 1.0 / 255)) * green2;
    b = b + (((255 - alpha1) * 1.0 / 255) * (alpha2 * 1.0 / 255)) * blue2;

    uint32_t new_alpha = (uint32_t)(alpha1 + ((255 - alpha1) * 1.0 / 255) * alpha2);
    uint32_t color1_over_color2 = (new_alpha << 24) |  (r << 16) | (g << 8) | (b << 0);
    return color1_over_color2;
}
