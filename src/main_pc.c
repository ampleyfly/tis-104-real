#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_surface.h"
#include "SDL2/SDL_video.h"

#include "app.h"
#include "disp.h"
#include "font.h"
#include "fonts.h"
#include "indexmap.h"
#include "pin.h"
#include "screen.h"
#include "spi_sw.h"
#include "tft.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 640;
const char *WINDOW_TITLE = "tis-104-real";

#define WIDTH (29)
#define HEIGHT (27)

static SDL_Surface *screen;

static uint32_t black;
static uint32_t white;
static uint32_t darkgrey;
static uint32_t red;

static uint32_t to_sdl_color(uint16_t color) {
    switch (color) {
    case COLOR_BLACK:
        return black;
    case COLOR_WHITE:
        return white;
    case COLOR_DARKGREY:
        return darkgrey;
    default:
        return red;
    }
    return red;
}

void tft_init(struct tft_t *tft, struct disp_t *disp, struct screen *scr, struct font *font) {
    tft->font = font;
    tft->disp = disp;
    tft->scr = scr;
    tft->max_x = TFT_WIDTH;
    tft->max_y = TFT_HEIGHT;
    tft->bg_color = COLOR_BLACK;
    tft->fg_color = COLOR_WHITE;
}

void tft_begin(struct tft_t *tft) {
    tft_set_background_color(tft, COLOR_BLACK);
    tft_set_foreground_color(tft, COLOR_WHITE);

    tft_clear(tft);
}

void tft_clear(struct tft_t *tft) {
    tft_fill_rectangle(tft, 0, 0, tft->max_x - 1, tft->max_y - 1, tft->bg_color);
}

void tft_set_backlight(struct tft_t *tft, bool flag) {
    (void)tft;
    (void)flag;
}

void tft_fill_rectangle(struct tft_t *tft, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    (void)tft;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    SDL_FillRect(screen, NULL, to_sdl_color(color));
}

void tft_set_background_color(struct tft_t *tft, uint16_t color) {
    tft->bg_color = color;
}

void tft_set_foreground_color(struct tft_t *tft, uint16_t color) {
    tft->fg_color = color;
}

void tft_draw_char(struct tft_t *tft, uint8_t x, uint8_t y, char ch) {
    uint16_t pixel_x = x * tft->font->width;
    uint16_t pixel_y = y * tft->font->height;
    // Set window
    int x0 = pixel_x;
    int y0 = pixel_y;
    int x1 = pixel_x + tft->font->width;
    int y1 = pixel_y + tft->font->height;
    uint32_t charPixels[6 * 8] = {0};
    // For each column of font character
    for (uint8_t i = 0; i < tft->font->width; i++) {
        uint8_t charData = font_read_column(tft->font, ch, i);

        // For every row in font character
        for (uint8_t k = 0; k < tft->font->height; k++) {
            if (charData & 1) {
                charPixels[i * 8 + k] = to_sdl_color(tft->fg_color);
            } else {
                charPixels[i * 8 + k] = to_sdl_color(tft->bg_color);
            }
            charData >>= 1;
        }
    }
    for (int y = y0; y < y1; ++y)
    {
        for (int x = x0; x < x1; ++x)
        {
            int yy = y - y0;
            int xx = x - x0;
            *((uint32_t *)screen->pixels + (y0 + yy) * WINDOW_WIDTH + x0 + xx) = charPixels[xx * 8 + yy];
        }
    }
}

int main(void) {
    static uint8_t buf[WIDTH * HEIGHT];
    static struct indexmap indices;
    static struct font font;
    static struct screen scr;
    static struct disp_t disp;
    static struct spi_t spi;
    static struct tft_t tft;

                                                            // Arduino pin
    pin_t led = pin_init(PIN_PORT_D, 2, PIN_DIR_OUTPUT);    // 2
    pin_t rs  = pin_init(PIN_PORT_D, 5, PIN_DIR_OUTPUT);    // 5
    pin_t rst = pin_init(PIN_PORT_D, 6, PIN_DIR_OUTPUT);    // 6
    pin_t cs  = pin_init(PIN_PORT_D, 7, PIN_DIR_OUTPUT);    // 7

    indexmap_init(&indices, WIDTH, HEIGHT, buf);
    screen_init(&scr, &indices);
    disp_init(&disp, &spi, rs, cs, rst, led);
    font_init(&font, monoblipp6x8);
    tft_init(&tft, &disp, &scr, &font);

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0);

    screen = SDL_GetWindowSurface(window);

    white = SDL_MapRGB(screen->format, 255, 255, 255);
    black = SDL_MapRGB(screen->format, 0, 0, 0);
    darkgrey = SDL_MapRGB(screen->format, 169, 169, 169);
    red = SDL_MapRGB(screen->format, 255, 0, 0);

    app_init(&scr, &tft);

    bool running = true;
    while (app_loop(&scr, &tft) && running) {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    default:
                        break;
                }
            }
        }

        SDL_UpdateWindowSurface(window);
        SDL_Delay(10);
    }
    app_deinit(&tft);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
