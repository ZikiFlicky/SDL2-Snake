#include <SDL2/SDL.h>

#define __USE_POSIX199309
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#define UNREACHABLE()                                                           \
    do {                                                                        \
        fprintf(stderr, "Unreachable triggered (%s:%d)\n", __FILE__, __LINE__); \
        abort();                                                                \
    } while (false);

static void setPixel(SDL_Surface *surface, int y, int x, Uint32 color) {
    int bpp = surface->format->BytesPerPixel;
    char *pixel = &((char *)surface->pixels)[surface->pitch * y + x * bpp];

    switch(bpp) {
    case 1:
      *pixel = color;
      break;
    case 2:
      *(Uint16*)pixel = color;
      break;
    case 3:
        for (int i = 0; i < 3; ++i) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            pixel[i] = color >> ((3 - i - 1) * 8) & 0xff;
#else
            pixel[i] = color >> (i * 8) & 0xff;
#endif
        }
        break;
    case 4:
        *(Uint32*)pixel = color;
        break;
    default:
        UNREACHABLE();
    }
}

static void seedRandom(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srand((unsigned int)ts.tv_sec % (unsigned int)ts.tv_nsec);
}

static void drawRect(SDL_Surface *surface, int y, int x, int w, int h, Uint32 color) {
    int end_x = SDL_min(x + w, surface->w);
    int end_y = SDL_min(y + h, surface->h);

    for (int rect_y = y; rect_y < end_y; ++rect_y) {
        for (int rect_x = x; rect_x < end_x; ++rect_x) {
            setPixel(surface, rect_y, rect_x, color);
        }
    }
}

static void fillSurface(SDL_Surface *surface, Uint32 color) {
    drawRect(surface, 0, 0, surface->w, surface->h, color);
}

/* A good, limiting, modulo, like Python's ((a % b) + b) % b */
static inline int relMod(int a, int b) {
    return ((a % b) + b) % b;
}

#define AMOUNT_CUBES_X 20
#define AMOUNT_CUBES_Y 20
#define CUBE_SIZE 15

struct SnakeNode {
    int x, y;
    struct SnakeNode *next;
};

static struct SnakeNode *SnakeNode_New(int x, int y) {
    struct SnakeNode *node = malloc(sizeof(struct SnakeNode));
    node->x = x;
    node->y = y;
    node->next = NULL;
    return node;
}

static struct SnakeNode *SnakeNode_Append(struct SnakeNode *node, int x, int y) {
    struct SnakeNode *next = SnakeNode_New(x, y);
    node->next = next;
    return next;
}

static struct SnakeNode *SnakeNode_Pop(struct SnakeNode *node) {
    struct SnakeNode *next = node->next;
    free(node);
    return next;
}

/* Returns true if the x and y are found inside of the node or one of the following nodes */
static bool SnakeNode_In(struct SnakeNode *node, int x, int y) {
    for (struct SnakeNode *n = node; n; n = n->next) {
        if (n->x == x && n->y == y) {
            return true;
        }
    }
    return false;
}

/* Generates an x and a y that are not present in any of the nodes */
static void generateRandomNotPresent(struct SnakeNode *node, int *out_x, int *out_y) {
    int x, y;
    do {
        x = rand() % AMOUNT_CUBES_X;
        y = rand() % AMOUNT_CUBES_Y;
    } while (SnakeNode_In(node, x, y));
    *out_x = x;
    *out_y = y;
}

int main(int argc, char **argv) {
    SDL_Window *window;
    SDL_Surface *surface;
    bool running = true;
    int dir_x = 1, dir_y = 0;
    int x = 0, y = 0;
    int apple_x, apple_y;
    Uint32 bg_color, snake_color, apple_color;
    struct SnakeNode *first_node, *current_node;
    int game_points = 0;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    atexit(SDL_Quit);

    window = SDL_CreateWindow("Snake",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            AMOUNT_CUBES_X * CUBE_SIZE,
                            AMOUNT_CUBES_Y * CUBE_SIZE,
                            SDL_WINDOW_SHOWN);
    surface = SDL_GetWindowSurface(window);

    // initialize colors
    bg_color = SDL_MapRGB(surface->format, 0, 0, 0);
    snake_color = SDL_MapRGB(surface->format, 0, 0xff, 0);
    apple_color = SDL_MapRGB(surface->format, 0xff, 0, 0);

    // initialize snake
    current_node = first_node = SnakeNode_New(0, 0);
    // current_node = SnakeNode_Append(current_node, 1, 0);

    seedRandom();
    // generate position for apple
    generateRandomNotPresent(first_node, &apple_x, &apple_y);

    SDL_LockSurface(surface);

    while (running) {
        SDL_Event event;
        int startTime = SDL_GetTicks();
        int frameTime; // The time it took for the whole frame to finish drawing
        bool got_input = false, game_end = false;

        fillSurface(surface, bg_color);
        // check for events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                if (got_input)
                    break;
                // check for input
                switch (event.key.keysym.sym) {
                case SDLK_w:
                case SDLK_UP:
                    if (dir_y == 0) {
                        dir_y = -1;
                        dir_x = 0;
                        got_input = true;
                    }
                    break;
                case SDLK_s:
                case SDLK_DOWN:
                    if (dir_y == 0) {
                        dir_y = 1;
                        dir_x = 0;
                        got_input = true;
                    }
                    break;
                case SDLK_a:
                case SDLK_LEFT:
                    if (dir_x == 0) {
                        dir_y = 0;
                        dir_x = -1;
                        got_input = true;
                    }
                    break;
                case SDLK_d:
                case SDLK_RIGHT:
                    if (dir_x == 0) {
                        dir_y = 0;
                        dir_x = 1;
                        got_input = true;
                    }
                    break;
                }
                break;
            case SDL_QUIT:
                running = false;
                break;
            }
        }

        // calculate new lead position
        y = relMod(y + dir_y, AMOUNT_CUBES_Y);
        x = relMod(x + dir_x, AMOUNT_CUBES_X);
        // change nodes respectively
        for (struct SnakeNode *node = first_node; node; node = node->next) {
            // if we collide with ourselves, end game
            if (node->x == x && node->y == y) {
                game_end = true;
                break;
            }
        }
        // if collided with ourselves, stop running
        if (game_end) {
            break;
        }
        current_node = SnakeNode_Append(current_node, x, y);
        first_node = SnakeNode_Pop(first_node);

        // if we colide with the apple
        if (x == apple_x && y == apple_y) {
            ++game_points;
            printf("Points: %d\n", game_points);
            // generate a valid apple position
            generateRandomNotPresent(first_node, &apple_x, &apple_y);
            current_node = SnakeNode_Append(current_node, x, y);
        } else {
            // show apple
            drawRect(surface,
                    apple_y * CUBE_SIZE,
                    apple_x * CUBE_SIZE,
                    CUBE_SIZE, CUBE_SIZE,
                    apple_color);
        }

        // show snake nodes
        for (struct SnakeNode *node = first_node; node; node = node->next) {
            drawRect(surface,
                    node->y * CUBE_SIZE,
                    node->x * CUBE_SIZE,
                    CUBE_SIZE, CUBE_SIZE,
                    snake_color);
        }

        SDL_UpdateWindowSurface(window);
        // draw the frame in the right time
        frameTime = SDL_GetTicks() - startTime;
        SDL_Delay(SDL_max(100 - frameTime, 0));
    }

    SDL_UnlockSurface(surface);

    SDL_Quit();
    return 0;
}
