#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <SDL.h>
#include <SDL2/SDL_ttf.h>

enum {
    MAX_WIDTH = 640, /* Width of game window */
    MAX_HEIGHT = 480, /* Height of game window */
    BALL_SIZE = 45, /* Must be same as BMP width */
    PADDLE_WIDTH = 20,
    PADDLE_HEIGHT = 100,
    PADDLE_SPEED = 4,
    PADDLE_OFFSET = 20, /* Offset of paddle from left or right of window */
    BALL_XSPEED = 5,
    BALL_YSPEED = 2,
    FPS = 60, /* Desired frames per second */
};

struct Body {
    SDL_Rect r; /* size and position*/
    int vx;     /* x velocity */
    int vy;     /* y velocity */
};
typedef struct Body Body;

struct Player {
    Body b;
    int s; /* score */
};
typedef struct Player Player;

struct Game {
    Body b; /* ball info */
    Player p1;
    Player p2; 
};
typedef struct Game Game;

static int clear(SDL_Renderer *r);
static int bound(int a, int b, int c);
static SDL_Texture * loadtex(SDL_Renderer *r, char *path);
static int draw(SDL_Renderer *r, SDL_Texture *t, Game *g);
static void reset(Game *g, int setscore, int setpaddles);
static int randomsign(void);
static void step(Game *g);

int
main(void)
{
    Game g; /* Game state */
    SDL_Window *w; /* Window handle */
    SDL_Renderer *r; /* Renderer used for drawing */
    SDL_Texture *t; /* Ball texture */
    SDL_Event e; /* A keyboard or window event */
    unsigned start; /* Start time of frame in ticks */
    unsigned end; /* End time of frame in ticks */
    int done; /* Flag to break out of game loop */
    int rc; /* Return code of program */

    w = NULL;
    r = NULL;
    t = NULL;
    rc = EXIT_SUCCESS;
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("Can't initialize SDL: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    if (TTF_Init() == -1) {
        SDL_Log("Can't initialize TTF: %s", TTF_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }
    w = SDL_CreateWindow("Pong",
                         SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED,
                         MAX_WIDTH, MAX_HEIGHT, 0);
    if (w == NULL) { goto error; }
    r = SDL_CreateRenderer(w, -1, SDL_RENDERER_ACCELERATED);
    if (r == NULL) { goto error; }

    t = loadtex(r, "./ball.bmp");
    if (t == NULL) { goto error; }

    reset(&g, 1, 1);
    if (draw(r, t, &g) != 0) { goto error; }
    
    for (done = 0; !done; ) {
        start = SDL_GetTicks();
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                done = 1;
                break;
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                case SDLK_w:
                    g.p1.b.vy = -PADDLE_SPEED;
                    break;
                case SDLK_s:
                    g.p1.b.vy = PADDLE_SPEED;
                    break;
                case SDLK_o:
                    g.p2.b.vy = -PADDLE_SPEED;
                    break;
                case SDLK_l:
                    g.p2.b.vy = PADDLE_SPEED;
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (e.key.keysym.sym) {
                case SDLK_w:
                case SDLK_s:
                    g.p1.b.vy = 0;
                    break;
                case SDLK_o:
                case SDLK_l:
                    g.p2.b.vy = 0;
                    break;
                }
                break;
            }
        }
        step(&g);
        if (draw(r, t, &g) != 0) { goto error; }
        end = SDL_GetTicks();
        if (end-start < 1000/FPS)
            SDL_Delay(1000/FPS - (start-end));
    }
    goto shutdown;

error:
    rc = EXIT_FAILURE;
    SDL_Log("%s", SDL_GetError());
    
shutdown:
    if (t != NULL) SDL_DestroyTexture(t);
    if (r != NULL) SDL_DestroyRenderer(r);
    if (w != NULL) SDL_DestroyWindow(w);
    TTF_Quit();
    SDL_Quit();
    return rc;
}

static int
clear(SDL_Renderer *r)
{
    int rc;
    rc = SDL_SetRenderDrawColor(r, 255, 255, 234, 255);
    if (rc != 0)
        return rc;
    rc = SDL_RenderClear(r);
    if (rc != 0)
        return rc;
    return 0;
}

static int
bound(int a, int b, int c)
{
    if (b < a)
        return a;
    if (b > c)
        return c;
    return b;
}

static SDL_Texture *
loadtex(SDL_Renderer *r, char *path)
{
    SDL_Surface *s;
    SDL_Texture *t;

    assert(r);
    assert(path);
    s = SDL_LoadBMP(path);
    if (s == NULL)
        return NULL;
    t = SDL_CreateTextureFromSurface(r, s);
    if (t == NULL)
        return NULL;
    SDL_FreeSurface(s);
    return t;
}

static int
draw(SDL_Renderer *r, SDL_Texture *t, Game *g)
{
    static SDL_Color black = {0, 0, 0, 0};
    
    SDL_Rect tr;
    TTF_Font *font;
    int rc;
    char buf[16];
    SDL_Surface *s;
    char *path;

    assert(r != NULL);
    assert(t != NULL);
    assert(g != NULL);

    tr.x = 0;
    tr.y = 0;
    tr.w = g->b.r.w;
    tr.h = g->b.r.h;

    /* Clear screen */
    rc = clear(r);
    if (rc != 0)
        return rc;
    
    /* Render ball */
    rc = SDL_RenderCopy(r, t, &tr, &g->b.r);
    if (rc != 0)
        return rc;
    /* Render paddles */
    rc = SDL_SetRenderDrawColor(r, 116, 168, 169, 255);
    if (rc != 0)
        return rc;
    rc = SDL_RenderFillRect(r, &g->p1.b.r);
    if (rc != 0)
        return rc;
    rc = SDL_RenderFillRect(r, &g->p2.b.r);
    if (rc != 0)
        return rc;
    
    /* Render player 1 score */
    path = "/Users/jfe/go/src/perkeep.org/clients/web/embed/opensans/OpenSans.ttf";
    font = TTF_OpenFont(path, 24);
    if (font == NULL) {
        SDL_Log("%s", TTF_GetError());
        return -1;
    }
    sprintf(buf, "%d", g->p1.s);
    s = TTF_RenderText_Solid(font, buf, black);
    if (s == NULL)
        return -1;
    t = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    if (t == NULL) {
        return -1;
    }
    tr.x = 5;
    tr.y = MAX_HEIGHT - 5 - s->h;
    tr.w = s->w;
    tr.h = s->h;
    SDL_RenderCopy(r, t, NULL, &tr);
    SDL_DestroyTexture(t);
    
    /* Render player 2 score */
    sprintf(buf, "%d", g->p2.s);
    s = TTF_RenderText_Solid(font, buf, black);
    if (s == NULL)
        return -1;
    t = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    if (t == NULL) {
        return -1;
    }
    tr.x = MAX_WIDTH - 5 - s->w;
    tr.y = MAX_HEIGHT - 5 - s->h;
    tr.w = s->w;
    tr.h = s->h;
    SDL_RenderCopy(r, t, NULL, &tr);
    SDL_DestroyTexture(t);
    
    /* SDL_Log("Closing font"); */
    TTF_CloseFont(font);
    /* Update screen */
    SDL_RenderPresent(r);
    return 0;
}

static int
randomsign(void)
{
    return (rand() % 2) ? -1 : 1;
}

static void
reset(Game *g, int setscore, int setpaddles)
{
    assert(g != NULL);

    /* Position ball in the middle of the screen */
    g->b.r.w = BALL_SIZE;
    g->b.r.h = BALL_SIZE;
    g->b.r.x = (MAX_WIDTH/2) - (g->b.r.w/2);
    g->b.r.y = (MAX_HEIGHT/2) - (g->b.r.w/2);
    g->b.vx = BALL_XSPEED * randomsign();
    g->b.vy = BALL_YSPEED * randomsign();

    /* Position left player paddle */
    g->p1.b.r.w = PADDLE_WIDTH;
    g->p1.b.r.h = PADDLE_HEIGHT;
    g->p1.b.r.x = PADDLE_OFFSET;
    if (setpaddles)
        g->p1.b.r.y = (MAX_HEIGHT/2) - (g->p1.b.r.h/2);
    g->p1.b.vx = 0;
    g->p1.b.vy = 0;

    /* Position right player paddle */
    g->p2.b.r.w = g->p1.b.r.w;
    g->p2.b.r.h = g->p1.b.r.h;
    g->p2.b.r.x = MAX_WIDTH - g->p2.b.r.w - PADDLE_OFFSET;
    if (setpaddles)
        g->p2.b.r.y = (MAX_HEIGHT/2) - (g->p2.b.r.h/2);
    g->p2.b.vx = 0;
    g->p2.b.vy = 0;

    /* Reset scores */
    if (setscore) {
        g->p1.s = 0;
        g->p2.s = 0;
    }
}

static void
step(Game *g)
{
    assert(g != NULL);
    
    if (g->b.r.x <= 0) {
        /* Ball collided with left side. Score point for right player and restart */
        ++g->p2.s;
        reset(g, 0, 0);
    } else if (g->b.r.x >= (MAX_WIDTH - g->b.r.w)) {
        /* Ball collided with right side. Score point for left player and restart */
        ++g->p1.s;
        reset(g, 0, 0);
    } else if (g->b.r.x < (g->p1.b.r.x + PADDLE_WIDTH - 10) &&
               g->b.r.y >= (g->p1.b.r.y - (BALL_SIZE / 2)) &&
               g->b.r.y + BALL_SIZE <= (g->p1.b.r.y + PADDLE_HEIGHT + (BALL_SIZE / 2))) {
        /* Ball collided with left paddle, reverse x direction */
        g->b.vx = -g->b.vx;
    } else if (g->b.r.x+BALL_SIZE > (g->p2.b.r.x + 10) &&
               g->b.r.y >= (g->p2.b.r.y - (BALL_SIZE / 2)) &&
               g->b.r.y + BALL_SIZE <= (g->p2.b.r.y + PADDLE_HEIGHT + (BALL_SIZE / 2))) {
        /* Ball collided with right paddle, reverse x direction */
        g->b.vx = -g->b.vx;
    }
    if (g->b.r.y <= 0 || g->b.r.y >= (MAX_HEIGHT - g->b.r.h)) {
        /* Ball collided with top or bottom, reverse y direction */
        g->b.vy = -g->b.vy;
    }

    /* Update positions of balls and paddles based on velocities */
    g->b.r.x = bound(0, g->b.r.x+g->b.vx, MAX_WIDTH-g->b.r.w);
    g->b.r.y = bound(0, g->b.r.y+g->b.vy, MAX_HEIGHT-g->b.r.h);
    g->p1.b.r.y = bound(0, g->p1.b.r.y+g->p1.b.vy, MAX_HEIGHT-g->p1.b.r.h);
    g->p2.b.r.y = bound(0, g->p2.b.r.y+g->p2.b.vy, MAX_HEIGHT-g->p2.b.r.h);
}
