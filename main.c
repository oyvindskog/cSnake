#define SDL_MAIN_HANDLED
#define SCREEN_W 640
#define SCREEN_H 480
#define SCREEN_SCALE 1
#define SCREEN_NAME "Snake"
#define SNAKE_BODYPART_SIZE 10
#define FOOD_SIZE 10
#define FPS 40

#include "SDL.h"

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include <SDL_ttf.h>


struct Vector2D;

void gameInit(void);
void gameQuit(void);
void gameHandleEvents(struct Vector2D*);
void initVector2D(struct Vector2D *v, int x, int y);
void placeFood(struct Vector2D *foodPosition);
void printText(char* txt, int x, int y);

// Define game struct for game data
static struct {

    SDL_bool running;
    char* playerName;
    int lengthOfName;
    struct {
        unsigned int w;
        unsigned int h;
        const char* name;
        SDL_Window* window;
        SDL_Renderer* renderer;

    } screen;

    // define "methods"
    void (*init)(void);
    void (*quit)(void);
    void (*handleEvents) (struct Vector2D*);

} Game = {
    SDL_FALSE,
    NULL,
    0,
    {
        SCREEN_SCALE*SCREEN_W,
        SCREEN_SCALE*SCREEN_H,
        SCREEN_NAME,
        NULL,
        NULL
    },
    gameInit,
    gameQuit,
    gameHandleEvents
};

struct Vector2D{
    int x;
    int y;
};

struct Snake{
    struct Vector2D **bodyparts;
    int length;
    int maxlength; //alloctaed memory for bodyparts array
    struct Vector2D direction;
};


void (*gameloop)(struct Snake *player, struct Vector2D *foodPosition, SDL_Rect *walls);
void initPlayer(struct Snake *player);
void highscores(struct Snake *player, struct Vector2D *foodPosition, SDL_Rect *walls);

Uint32 lastKeypress = 0;

void quitOnSDL_QUIT(SDL_Event *event){
    switch(event->type) {
        case SDL_QUIT: {
            Game.running = SDL_FALSE;
        } break;
    }
}

//-----------------------------------------functions
void gameHandleEvents(struct Vector2D *direction){
    SDL_Event event;
    while(SDL_PollEvent(&event)) {

        quitOnSDL_QUIT(&event);

        const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );

        if( currentKeyStates[SDL_SCANCODE_Q]){
            Game.running = SDL_FALSE;
        }

        if( currentKeyStates[ SDL_SCANCODE_UP ] ){
            initVector2D(direction, 0, -1);
        }
        if( currentKeyStates[ SDL_SCANCODE_DOWN ] ){
            initVector2D(direction, 0, 1);
        }
        if( currentKeyStates[ SDL_SCANCODE_LEFT ] ){
            initVector2D(direction, -1, 0);
        }
        if( currentKeyStates[ SDL_SCANCODE_RIGHT ] ){
            initVector2D(direction, 1, 0);
        }
    }
}

//-----------------------------------------------------
void gameInit(void) {
    printf("game_init()n");
    if(SDL_Init(SDL_INIT_EVERYTHING)!=0) {
        printf("SDL error -> %sn", SDL_GetError());
        exit(1);
    }

    unsigned int w = Game.screen.w;
    unsigned int h = Game.screen.h;
    const char* name = Game.screen.name;

    Game.screen.window = SDL_CreateWindow(
        name,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w, h, 0
    );
    Game.screen.renderer = SDL_CreateRenderer(
        Game.screen.window, -1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC
    );

    Game.running = SDL_TRUE;

    Game.playerName = malloc( 20 * sizeof(char) );
}
//---------------------------------------------------------
void gameQuit(void) {
    printf("game_quit()\n");
    SDL_DestroyRenderer(Game.screen.renderer);
    SDL_DestroyWindow(Game.screen.window);

    Game.screen.window = NULL;
    Game.screen.renderer = NULL;

    SDL_Quit();
    Game.running = SDL_FALSE;
    free(Game.playerName);
}

//--------------------------------------------------

void initVector2D(struct Vector2D *v, int x, int y){
    v->x = x;
    v->y = y;
}
//-----------------------------------------------------

void addVectors2D(struct Vector2D *v, struct Vector2D *v2){
    v->x += v2->x;
    v->y += v2->y;
}

//-----------------------------------------------------
/* returns string representation of Vector2D
 * memory have to be deallocated when done with returned string
 */
char *vector2DToString(struct Vector2D *v){
    int length = 0;
    length += snprintf( NULL, 0, "%d", v->x );
    length += snprintf( NULL, 0, "%d", v->y );
    length += 3;
    char* str = malloc( length + 1 );
    snprintf( str, length + 1, "(%d,%d)", v->x, v->y );
    return str;
}

//--------------------------------------------------------
// allocate memory for an array of pointers Vector2Ds
struct Vector2D** allocateVector2DArray(int size){
    struct Vector2D **tempVector2Ds;
    tempVector2Ds = malloc(size * sizeof *tempVector2Ds);
    if (tempVector2Ds == NULL) {
        return NULL;
    }
    for (int i=0; i<size; i++){
        tempVector2Ds[i] = malloc(sizeof tempVector2Ds[i]);
        if (tempVector2Ds[i] == NULL){
            return NULL;
        }
        initVector2D(tempVector2Ds[i], -1, -1);
    }
    return tempVector2Ds;
}

//--------------------------------------------------------
// allocate more memory for snake array of pointers Vector2Ds
struct Vector2D** expandVector2DArray(struct Vector2D **snake, int size, int increment){

    struct Vector2D** tmp;
    int totalSize = increment + size;
    tmp = realloc(snake, totalSize * sizeof *snake);
    if (tmp == NULL) {
        //TODO: handle failed realloc, temp_struct is unchanged
        printf("Error reallocating mem");
    } else {
        for (int i = size; i < totalSize; ++i) { //
            tmp[i] = malloc(sizeof *snake[i]);
            initVector2D(tmp[i], -1, -1);
        }
    }

    return tmp;
}

//--------------------------------------------------------
void moveBody(struct Vector2D **bodyparts, int size){

    for (int i=size-1; i>0; i--){
        //printf("%d : %s--",i,vector2DToString(bodyparts[i]));
        bodyparts[i]->x = bodyparts[i-1]->x;
        bodyparts[i]->y = bodyparts[i-1]->y;
    }
}
//--------------------------------------------------------
void moveSnake(struct Snake *player){

    moveBody(player->bodyparts, player->length);
    //move snake head
    addVectors2D(player->bodyparts[0], &player->direction);
}
//-----------------------------------------------------
void drawSnake(SDL_Renderer* renderer, struct Vector2D **snake, int snakeLength){

    for (int i=0; i<snakeLength; i++){
        SDL_Rect r;
        r.x = SNAKE_BODYPART_SIZE * snake[i]->x;
        r.y = SNAKE_BODYPART_SIZE * snake[i]->y;
        r.w = r.h = SNAKE_BODYPART_SIZE;

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255 );
        SDL_RenderFillRect(renderer, &r );
    }
}
//------------------------------------------------------
void drawFood(SDL_Renderer* renderer, struct Vector2D *foodPosition){
    SDL_Rect r;
    r.x = foodPosition->x;
    r.y = foodPosition->y;
    r.w = r.h = FOOD_SIZE;

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255 );
    SDL_RenderFillRect(renderer, &r );
}
//------------------------------------------------------
void drawWalls(SDL_Renderer* renderer, SDL_Rect *walls, int numberOfWalls){
    SDL_SetRenderDrawColor(renderer, 0, 255, 51, 255 );
    for(int i=0; i<numberOfWalls; i++){
        SDL_RenderFillRect(renderer, &walls[i] );
    }
}
//------------------------------------------------------

SDL_Rect getSnakeRect(struct Vector2D **snake, int index){
    SDL_Rect r;
    r.x = SNAKE_BODYPART_SIZE * snake[index]->x;
    r.y = SNAKE_BODYPART_SIZE * snake[index]->y;
    r.w = r.h = SNAKE_BODYPART_SIZE;

    return r;
}
//------------------------------------------------------
SDL_bool checkWallCollisions(struct Vector2D **snake, SDL_Rect *walls, int numberOfWalls){

    SDL_Rect head = getSnakeRect(snake, 0);
    for (int i=0; i<numberOfWalls; i++){

        if ( SDL_HasIntersection(&head, &walls[i]) ){
            return SDL_TRUE;
        }
    }

    return SDL_FALSE;
}

//------------------------------------------------------
SDL_bool checkSnakeCollisions(struct Vector2D **snake, int length){
    SDL_Rect head = getSnakeRect(snake, 0);

    for(int i=1; i<length; i++){
        SDL_Rect bodypart = getSnakeRect(snake, i);
        if ( SDL_HasIntersection(&head, &bodypart) ){
            return SDL_TRUE;
        }
    }
    return SDL_FALSE;
}
//------------------------------------------------------

//------------------------------------------------------
SDL_bool checkFoodCollisions(struct Vector2D **snake, struct Vector2D *foodPosition){
    SDL_Rect head = getSnakeRect(snake, 0);
    SDL_Rect r;
    r.x = foodPosition->x;
    r.y = foodPosition->y;
    r.w = r.h = FOOD_SIZE;
    if (SDL_HasIntersection(&r, &head)){
        return SDL_TRUE;
    }
    return SDL_FALSE;
}
//------------------------------------------------------

void mainloop(struct Snake *player, struct Vector2D *foodPosition, SDL_Rect *walls){

    moveSnake(player);
    Game.handleEvents(&player->direction);

    SDL_SetRenderDrawColor( Game.screen.renderer, 255, 0, 0, 255 );
    SDL_RenderClear(Game.screen.renderer);

    drawSnake(Game.screen.renderer, player->bodyparts, player->length);
    drawFood(Game.screen.renderer, foodPosition);
    drawWalls(Game.screen.renderer, walls, 4);

    //Render score
    char* score = malloc(5 * sizeof(char));
    sprintf(score, "%d", player->length);
    printText(score, 0, 0 );
    free(score);
    SDL_RenderPresent(Game.screen.renderer);

    // Check collisions
    if (checkFoodCollisions(player->bodyparts, foodPosition)){

        if (player->length + 5 > player->maxlength){
            player->bodyparts = expandVector2DArray(player->bodyparts, player->length, 100);
            player->maxlength += 100;
        }
        player->length += 5;
        placeFood(foodPosition);
    }
    if (checkWallCollisions(player->bodyparts, walls, 4) ||
        checkSnakeCollisions(player->bodyparts, player->length)
        ){
       for (int i = 0; i < player->length; ++i) free(player->bodyparts[i]);
       free(player->bodyparts);
       initPlayer(player);
       gameloop = highscores;
    }

}

void placeFood(struct Vector2D *foodPosition){
    int x = (rand() % (SCREEN_W - (3 * FOOD_SIZE))) + FOOD_SIZE;
    int y = (rand() % (SCREEN_H - (3 * FOOD_SIZE))) + FOOD_SIZE;
    initVector2D(foodPosition, x, y);
}

void initPlayer(struct Snake *player){
    player->length = 10;
    player->maxlength = 100;

    //Define starting direction
    struct Vector2D direction;
    initVector2D(&direction, 1, 0);
    player->direction = direction;
    player->bodyparts = allocateVector2DArray(player->maxlength);

    //Define starting position
    for (int i=0; i<player->length; i++){
        initVector2D(player->bodyparts[i], 22 - i, 20);
    }
}

void printText(char* txt, int x, int y){
    SDL_Color textColor = { 255, 255, 255, 0 };
    int fontSize = 20;
    TTF_Init();
    TTF_Font * font = TTF_OpenFont("arial.ttf", fontSize);
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, txt, textColor);
    SDL_Texture* text = SDL_CreateTextureFromSurface(Game.screen.renderer, textSurface);
    int text_width = textSurface->w;
    int text_height = textSurface->h;
    SDL_FreeSurface(textSurface);
    SDL_Rect renderQuad = { x, y, text_width, text_height };
    SDL_RenderCopy(Game.screen.renderer, text, NULL, &renderQuad);

    SDL_DestroyTexture(text);
    TTF_CloseFont(font);

}

void getName(void){
    const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );
    int time = SDL_GetTicks() - lastKeypress;
    for (int i=0; i<30; i++){
        if( currentKeyStates[i]){
            SDL_Keycode keyCode = SDL_GetKeyFromScancode(i);

            if (Game.lengthOfName < 19 && time > 100){
                lastKeypress = SDL_GetTicks();
                Game.playerName[Game.lengthOfName] = (char) keyCode;
                Game.playerName[Game.lengthOfName + 1] = '\0';
                Game.lengthOfName++;
            }
        }
    }
    if (currentKeyStates[SDL_SCANCODE_RETURN]){
        gameloop = mainloop;
    }
    if (currentKeyStates[SDL_SCANCODE_BACKSPACE] &&
        Game.lengthOfName > 0 &&
        time > 100
        ){
        Game.playerName[Game.lengthOfName] = '\0';
        Game.lengthOfName--;
        lastKeypress = SDL_GetTicks();
    }
}

void highscores(struct Snake *player, struct Vector2D *foodPosition, SDL_Rect *walls){
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        quitOnSDL_QUIT(&event);
        getName();
    }
    SDL_SetRenderDrawColor( Game.screen.renderer, 255, 0, 0, 255 );
    SDL_RenderClear(Game.screen.renderer);
    drawSnake(Game.screen.renderer, player->bodyparts, player->length);
    //Render food
    drawFood(Game.screen.renderer, foodPosition);
    drawWalls(Game.screen.renderer, walls, 4);
    if (Game.lengthOfName > 0){
        printText(Game.playerName, 20, 20);
    }
    SDL_RenderPresent(Game.screen.renderer);

}

int main()
{
    // Initialize srand
    srand(time(NULL));

    // Set initial game state
    gameloop = mainloop;

    // Create player
    struct Snake player;
    initPlayer(&player);


    //Define starting position for food
    struct Vector2D foodPosition;
    placeFood(&foodPosition);

    // Initialize game
    Game.init();

    //Define walls
    SDL_Rect walls[4];
    walls[0].x = 0;
    walls[0].y = 0;
    walls[0].w = SCREEN_W;
    walls[0].h = 10;
    walls[1].x = 0;
    walls[1].y = SCREEN_H - 10;
    walls[1].w = SCREEN_W;
    walls[1].h = 10;
    walls[2].x = 0;
    walls[2].y = 10;
    walls[2].w = 10;
    walls[2].h = SCREEN_H - 20;
    walls[3].x = SCREEN_W - 10;
    walls[3].y = 10;
    walls[3].w = 10;
    walls[3].h = SCREEN_H - 20;

    Uint32 frameStart;
    int frameTime;
    int frameDelay = 1000 / FPS;
    // Game loop
    while(Game.running) {

        frameStart = SDL_GetTicks();
        // run the loop that corresponds with game state
        gameloop(&player, &foodPosition, walls);

        frameTime = SDL_GetTicks() - frameStart;

        if (frameDelay > frameTime){
            SDL_Delay(frameDelay - frameTime); //Delay as much as needed;
        }

    }

    // Clean up on finish
    for (int i = 0; i < player.length; ++i) free(player.bodyparts[i]);
    free(player.bodyparts);
    Game.quit();

    return 0;
}
