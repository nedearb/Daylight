//
//  main.cpp
//  Daylight
//
//  Created by Braeden Atlee on 12/19/16.
//

#include <png.h>
#include <stdio.h>
#include <cmath>
#include <ctime>
#include <vector>
#include <map>
#include <string>
#include <SDL2/SDL.h>

using namespace std;

#define DIM 3
#define DRAW_2D (DIM==2)
#define DRAW_3D (DIM==3)

#define ENABLE_MOUSE 0

#define TILE_SIZE_2D 8

#define PI 3.14159265358979323
#define TAU (PI*2)


#define VIEW_WIDTH 960
#define VIEW_HEIGHT 640
#define ASPECT_RATIO (VIEW_WIDTH/VIEW_HEIGHT)
#define FOV (TAU/6)

#define HUD_HEIGHT 16
#define SCREEN_WIDTH VIEW_WIDTH
#define SCREEN_HEIGHT (VIEW_HEIGHT+HUD_HEIGHT)

#define FAR 1000


#define INP_MOVE_LEFT 0
#define INP_MOVE_RIGHT 1
#define INP_MOVE_FORWARD 2
#define INP_MOVE_BACKWARD 3
#define INP_TURN_LEFT 4
#define INP_TURN_RIGHT 5

#define PLAYER_SIZE (0.1)

#define LEVEL_WIDTH 32
#define LEVEL_HEIGHT 32

#define TILE_EMPTY 0
#define TILE_BRICK 1
#define TILE_METAL 2
#define TILE_BLUE_WALL 3
#define TILE_GREEN_BRICK 4
#define TILE_MIRROR 5
#define TILE_COUNT 6


#define TEXTURE_WIDTH 16
#define TEXTURE_HEIGHT 16

#define HUD_COLOR 0x202020
#define CLEAR_COLOR 0xFFFFFF
#define CEIL_COLOR 0x55C0FF
#define FLOOR_COLOR 0x404040


float posX, posY;
float yawLook;

bool inputs[6];
int mouseXDif = 0;
int mouseYDif = 0;

Uint32 oldTicks = 0;
Uint32 newTicks = 0;
float frameTime = 0;
int averageFps = 0;
float sumFps = 0;
int countFps = 0;

#define ENTITY_TYPE_TEST1 0
#define ENTITY_TYPE_TEST2 1
#define ENTITY_TYPE_COUNT 2

struct Entity{
    Entity(){}
    Entity(float x, float y, int type) : x(x), y(y), type(type){}
    float x, y;
    int type;
    
    //temp:
    float dist;
};

vector<Entity> entityList;


unsigned char level[LEVEL_WIDTH][LEVEL_HEIGHT];

unsigned int tileTextures[TILE_COUNT-1][TEXTURE_WIDTH][TEXTURE_HEIGHT];
unsigned int entityTextures[ENTITY_TYPE_COUNT][TEXTURE_WIDTH][TEXTURE_HEIGHT];
#define FONT_ROW_COUNT 16
#define FONT_COL_COUNT 16
#define FONT_CHAR_COUNT (FONT_ROW_COUNT*FONT_COL_COUNT)
#define FONT_CHAR_WIDTH 7
#define FONT_CHAR_HEIGHT 12
bool font[FONT_CHAR_COUNT][FONT_CHAR_WIDTH][FONT_CHAR_HEIGHT];

void setRenderDrawColorRGB(SDL_Renderer* renderer, unsigned int color){
    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, 0xFF);
}

void setRenderDrawColorRGB(SDL_Renderer* renderer, unsigned int color, unsigned int shift){
    SDL_SetRenderDrawColor(renderer, ((color >> 16) & 0xFF) >> shift, ((color >> 8) & 0xFF) >> shift, ((color) & 0xFF) >> shift, 0xFF);
}

void setRenderDrawColorARGB(SDL_Renderer* renderer, unsigned int color){
    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF);
}

png_bytep* loadPng(const char* path, int& width, int& height){
    FILE* file = fopen(path, "rb");
    if(!file) return nullptr;
    
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png) return nullptr;
    
    png_infop info = png_create_info_struct(png);
    if(!info) return nullptr;
    
    if(setjmp(png_jmpbuf(png))) abort();
    
    png_init_io(png, file);
    
    png_read_info(png, info);
    
    width = png_get_image_width(png, info);
    height = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    png_byte bitDepth = png_get_bit_depth(png, info);
    
    if(bitDepth == 16){
        png_set_strip_16(png);
    }
    
    if(colorType == PNG_COLOR_TYPE_PALETTE){
        png_set_palette_to_rgb(png);
    }
    
    if(colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8){
        png_set_expand_gray_1_2_4_to_8(png);
    }
    
    if(png_get_valid(png, info, PNG_INFO_tRNS)){
        png_set_tRNS_to_alpha(png);
    }
    
    if(colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_PALETTE){
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }
    
    if(colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA){
        png_set_gray_to_rgb(png);
    }
    
    png_read_update_info(png, info);
    
    png_bytep* rowPointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int y = 0; y < height; y++) {
        rowPointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
    }
    
    png_read_image(png, rowPointers);
    
    fclose(file);
    
    return rowPointers;
}

void loadPngAsTexture(unsigned int texture[TEXTURE_WIDTH][TEXTURE_HEIGHT], const char* path){
    
    int width, height;
    png_bytep* rowPointers = loadPng(path, width, height);
    
    if(rowPointers){
        for(int y = 0; y < height; y++) {
            png_bytep row = rowPointers[y];
            for(int x = 0; x < width; x++) {
                png_bytep px = &(row[x * 4]);
                texture[x][y] = (px[0]<<16) | (px[1]<<8) | (px[2]) | (px[3]<<24);
            }
        }
    }
}

void loadPngAsFont(const char* path){
    
    int width, height;
    png_bytep* rowPointers = loadPng(path, width, height);
    
    if(rowPointers){
        for(int y = 0; y < height; y++) {
            png_bytep row = rowPointers[y];
            for(int x = 0; x < width; x++) {
                png_bytep px = &(row[x * 4]);
                int i = (x/FONT_CHAR_WIDTH)+((y/FONT_CHAR_HEIGHT)*FONT_ROW_COUNT);
                int ix = x%FONT_CHAR_WIDTH;
                int iy = y%FONT_CHAR_HEIGHT;
                font[i][ix][iy] = (px[3]) == 0 ? false : true;
            }
        }
    }
}

void loadPngAsLevel(const char* path){
    int width, height;
    png_bytep* rowPointers = loadPng(path, width, height);
    
    if(rowPointers && height>=2){
        png_bytep tileKeyRow = rowPointers[0];
        map<unsigned int, unsigned char> tileKey;
        
        png_bytep entityKeyRow = rowPointers[1];
        map<unsigned int, int> entityKey;
        
        for(int x = width-1; x >= 0; x--) {
            png_bytep px = &(tileKeyRow[x * 4]);
            tileKey[(px[0]<<16) | (px[1]<<8) | (px[2]) | (px[3]<<24)] = x;
        }
        
        for(int x = width-1; x >= 0; x--) {
            png_bytep px = &(entityKeyRow[x * 4]);
            entityKey[(px[0]<<16) | (px[1]<<8) | (px[2]) | (px[3]<<24)] = x;
        }
        
        for(int y = 2; y < height; y++) {
            png_bytep row = rowPointers[y];
            for(int x = 0; x < width; x++) {
                png_bytep px = &(row[x * 4]);
                unsigned int c = (px[0]<<16) | (px[1]<<8) | (px[2]) | (px[3]<<24);
                if(tileKey.find(c) != tileKey.end()){
                    level[x][y-2] = tileKey[c];
                }else if(entityKey.find(c) != entityKey.end()){
                    if(entityKey[c] == 0){
                        posX = x+.5;
                        posY = y+.5;
                    }else{
                        entityList.push_back(Entity(x+.5, y-2+.5, entityKey[c]-1));
                        level[x][y-2] = 0;
                    }
                }
            }
        }
    }
}

void drawChar(SDL_Renderer* renderer, int xr, int yr, char c){
    SDL_Point points[FONT_CHAR_WIDTH*FONT_CHAR_HEIGHT];
    int i = 0;
    for(int x=0;x<FONT_CHAR_WIDTH;x++){
        for(int y=0;y<FONT_CHAR_HEIGHT;y++){
            if(font[c-32][x][y]){
                points[i].x = xr+x;
                points[i].y = yr+y;
                i++;
            }
        }
    }
    SDL_RenderDrawPoints(renderer, points, i);
}

void drawString(SDL_Renderer* renderer, int xr, int yr, const char* str){
    int i=0;
    while(str[i] != '\0'){
        drawChar(renderer, xr+(i*(FONT_CHAR_WIDTH+1)), yr, str[i]);
        i++;
    }
}

unsigned char safeTile(int x, int y){
    return (x<0||y<0||x>=LEVEL_WIDTH||y>=LEVEL_HEIGHT) ? TILE_EMPTY : level[x][y];
}

bool solid(int x, int y){
    return safeTile(x, y) != 0;
}

bool simpleLineHit(float originX, float originY, float aX, float aY, float len){
    
    int mapX = (int)(originX);
    int mapY = (int)(originY);
    
    float sideDistX;
    float sideDistY;
    
    float deltaDistX = sqrtf(1 + (aY * aY) / (aX * aX));
    float deltaDistY = sqrtf(1 + (aX * aX) / (aY * aY));
    
    int stepX;
    int stepY;
    
    if (aX < 0) {
        stepX = -1;
        sideDistX = (originX - mapX) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (mapX + 1.0 - originX) * deltaDistX;
    }
    if (aY < 0) {
        stepY = -1;
        sideDistY = (originY - mapY) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (mapY + 1.0 - originY) * deltaDistY;
    }
    
    bool sideNS;
    
    float dist = 0;
    
    while(dist < len){
        if(sideDistX < sideDistY){
            sideDistX += deltaDistX;
            mapX += stepX;
            sideNS = false;
        }else{
            sideDistY += deltaDistY;
            mapY += stepY;
            sideNS = true;
        }
        if(sideNS){
            dist = (mapY - originY + (1 - stepY) / 2) / aY;
        }else{
            dist = (mapX - originX + (1 - stepX) / 2) / aX;
        }
        if(solid(mapX, mapY)){
            return dist < len;
        }
    }
    return false;
}

void bubbleSortByDist(vector<Entity>& list) {
    Entity temp;
    int size = (int)list.size();
    for(int i=0; i<size; i++) {
        for(int j=size-1; j>i; j--) {
            if(list[j].dist>list[j-1].dist) {
                temp=list[j-1];
                list[j-1]=list[j];
                list[j]=temp;
            }
        }
    }
}

void draw(SDL_Renderer *renderer){
    
    float moveSpeed = frameTime * 5.0f; // coords per second
    float rotSpeed = frameTime * 3.0f; // radians per second
    float mouseRotSpeed = frameTime * 6.0f;
    
    float otherMoveSpeed = frameTime * 2.0f;
    
    
    if(inputs[INP_TURN_LEFT]){
        yawLook -= rotSpeed;
    }
    if(inputs[INP_TURN_RIGHT]){
        yawLook += rotSpeed;
    }
    if(mouseXDif != 0){
        yawLook += mouseXDif * mouseRotSpeed;
        mouseXDif = 0;
    }
    
    float lookX = cos(yawLook);
    float lookY = sin(yawLook);
    
    float tryX = posX;
    float tryY = posY;
    
    if(inputs[INP_MOVE_FORWARD]){
        tryX += lookX*moveSpeed;
        tryY += lookY*moveSpeed;
    }
    if(inputs[INP_MOVE_BACKWARD]){
        tryX -= lookX*moveSpeed;
        tryY -= lookY*moveSpeed;
    }
    if(inputs[INP_MOVE_LEFT]){
        tryX += lookY*moveSpeed;
        tryY -= lookX*moveSpeed;
    }
    if(inputs[INP_MOVE_RIGHT]){
        tryX -= lookY*moveSpeed;
        tryY += lookX*moveSpeed;
    }
    
    if(DRAW_2D){
        SDL_SetRenderDrawColor(renderer, 0, 0, 0xFF, 0xFF);
        SDL_RenderDrawLine(renderer, posX*TILE_SIZE_2D, posY*TILE_SIZE_2D, (posX+(lookX*10))*TILE_SIZE_2D, (posY+(lookY*10))*TILE_SIZE_2D);
        
        float leftLookX = cos(yawLook-FOV/2);
        float leftLookY = sin(yawLook-FOV/2);
        float rightLookX = cos(yawLook+FOV/2);
        float rightLookY = sin(yawLook+FOV/2);
        SDL_RenderDrawLine(renderer, posX*TILE_SIZE_2D, posY*TILE_SIZE_2D, (posX+(leftLookX*1000))*TILE_SIZE_2D, (posY+(leftLookY*1000))*TILE_SIZE_2D);
        SDL_RenderDrawLine(renderer, posX*TILE_SIZE_2D, posY*TILE_SIZE_2D, (posX+(rightLookX*1000))*TILE_SIZE_2D, (posY+(rightLookY*1000))*TILE_SIZE_2D);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
        
        SDL_Rect rect;
        rect.w = TILE_SIZE_2D;
        rect.h = TILE_SIZE_2D;
        
        
        for(int x=0;x<LEVEL_WIDTH;x++){
            rect.x = x*TILE_SIZE_2D;
            for(int y=0;y<LEVEL_HEIGHT;y++){
                rect.y = y*TILE_SIZE_2D;
                if(solid(x, y)){
                    unsigned int color = tileTextures[safeTile(x, y)-1][0][0];
                    setRenderDrawColorRGB(renderer, color);
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
        
    }
    
    int posTile1X = (int)((posX-PLAYER_SIZE));
    int posTile1Y = (int)((posY-PLAYER_SIZE));
    int posTile2X = (int)((posX+PLAYER_SIZE));
    int posTile2Y = (int)((posY+PLAYER_SIZE));
    int tryTile1X = (int)((tryX-PLAYER_SIZE));
    int tryTile1Y = (int)((tryY-PLAYER_SIZE));
    int tryTile2X = (int)((tryX+PLAYER_SIZE));
    int tryTile2Y = (int)((tryY+PLAYER_SIZE));
    
    if(solid(posTile1X, posTile1Y) || solid(posTile2X, posTile2Y) ||
       solid(posTile1X, posTile2Y) || solid(posTile2X, posTile1Y)){ // Any part of player is in a wall right now?
        //allowed to get out of tile...
    }else if(solid(tryTile1X, tryTile1Y) || solid(tryTile2X, tryTile2Y) ||
             solid(tryTile1X, tryTile2Y) || solid(tryTile2X, tryTile1Y)){ // any part of player will be in a wall soon?
        if(!(solid(tryTile1X, posTile1Y) || solid(tryTile2X, posTile2Y) ||
             solid(tryTile1X, posTile2Y) || solid(tryTile2X, posTile1Y))){ // See if only moving in the X direction works
            tryY = posY;
        }else if(!(solid(posTile1X, tryTile1Y) || solid(posTile2X, tryTile2Y) ||
                   solid(posTile1X, tryTile2Y) || solid(posTile2X, tryTile1Y))){ // See if only moving in the Y direction works
            tryX = posX;
        }else{ // Neither worked
            tryX = posX;
            tryY = posY;
        }
    }
    
    posX = tryX;
    posY = tryY;
    
    for(Entity& e : entityList){
        if(DRAW_2D){
            unsigned int color = entityTextures[e.type][0][0];
            SDL_Rect rect;
            rect.x = e.x * TILE_SIZE_2D;
            rect.y = e.y * TILE_SIZE_2D;
            rect.w = TILE_SIZE_2D;
            rect.h = TILE_SIZE_2D;
            setRenderDrawColorRGB(renderer, color);
            SDL_RenderDrawRect(renderer, &rect);
        }
        
        switch (e.type) {
            case ENTITY_TYPE_TEST2:{
                float len = sqrtf((e.x-posX)*(e.x-posX)+(e.y-posY)*(e.y-posY));
                if(len > 1){
                    float aX = (posX-e.x) / len;
                    float aY = (posY-e.y) / len;
                    if(!simpleLineHit(e.x, e.y, aX, aY, len)){
                        e.x += aX * otherMoveSpeed;
                        e.y += aY * otherMoveSpeed;
                    }
                    if(DRAW_2D){
                        unsigned int color = entityTextures[e.type][0][0];
                        setRenderDrawColorRGB(renderer, color);
                        SDL_RenderDrawLine(renderer, e.x * TILE_SIZE_2D, e.y * TILE_SIZE_2D, (e.x+aX*len) * TILE_SIZE_2D, (e.y+aY*len) * TILE_SIZE_2D);
                    }
                }
                break;
            }
        }
        
        e.dist = sqrtf((posX-e.x)*(posX-e.x)+(posY-e.y)*(posY-e.y));
    }
    bubbleSortByDist(entityList);
    
    for(int x=0;x<VIEW_WIDTH;x++){
        float angle = (x-VIEW_WIDTH/2)*(FOV/VIEW_WIDTH);
        int hitX = 0;
        int hitY = 0;
        bool sideNS = false;
        float dist = 0;
        float wallX = 0;
        float aX = cos(yawLook + angle);
        float aY = sin(yawLook + angle);
        
        int mapX = (int)(posX);
        int mapY = (int)(posY);
        
        float sideDistX;
        float sideDistY;
        
        float deltaDistX = sqrtf(1 + (aY * aY) / (aX * aX));
        float deltaDistY = sqrtf(1 + (aX * aX) / (aY * aY));
        
        int stepX;
        int stepY;
        
        if (aX < 0) {
            stepX = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0 - posX) * deltaDistX;
        }
        if (aY < 0) {
            stepY = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0 - posY) * deltaDistY;
        }
        
        int tiles = 0;
        
        float reflectDist = 0;
        
        
        float fromX = posX;
        float fromY = posY;
        
        int reflections = 0;
        float firstDist = 0;
        
        while(tiles < FAR){
            if(sideDistX < sideDistY){
                sideDistX += deltaDistX;
                mapX += stepX;
                sideNS = false;
            }else{
                sideDistY += deltaDistY;
                mapY += stepY;
                sideNS = true;
            }
            if(sideNS){
                dist = (mapY - fromY + (1 - stepY) / 2) / aY;
            }else{
                dist = (mapX - fromX + (1 - stepX) / 2) / aX;
            }
            if(solid(mapX, mapY)){
                
                reflectDist += dist;
                hitX = mapX;
                hitY = mapY;
                if(sideNS){
                    wallX = fromX + dist * aX;
                }else{
                    wallX = fromY + dist * aY;
                }
                wallX -= floor((wallX));
                
                if(firstDist == 0){
                    firstDist = dist;
                }
                
                if(level[mapX][mapY] == TILE_MIRROR){
                    
                    reflections++;
                    
                    if(DRAW_2D){
                        unsigned int color = 0xFF0000;
                        setRenderDrawColorRGB(renderer, color);
                        SDL_RenderDrawLine(renderer, fromX*TILE_SIZE_2D, fromY*TILE_SIZE_2D, mapX*TILE_SIZE_2D+TILE_SIZE_2D/2, mapY*TILE_SIZE_2D+TILE_SIZE_2D/2);
                    }
                    
                    fromX = mapX+(sideNS?0:(aX>0?0:1));
                    fromY = mapY+(sideNS?(aY>0?0:1):0);
                    
                    if(sideNS){
                        aY = -aY;
                        fromX += wallX;
                    }else{
                        aX = -aX;
                        fromY += wallX;
                    }
                    
                    if (aX < 0) {
                        stepX = -1;
                        sideDistX = (fromX - mapX) * deltaDistX;
                    } else {
                        stepX = 1;
                        sideDistX = (mapX + 1.0 - fromX) * deltaDistX;
                    }
                    if (aY < 0) {
                        stepY = -1;
                        sideDistY = (fromY - mapY) * deltaDistY;
                    } else {
                        stepY = 1;
                        sideDistY = (mapY + 1.0 - fromY) * deltaDistY;
                    }
                }else{
                    break;
                }
            }
            tiles++;
        }
        
        dist = reflectDist;
        
        if(dist >= 0){
            if(DRAW_2D){
                unsigned int color = tileTextures[safeTile(hitX, hitY)-1][0][0];
                setRenderDrawColorRGB(renderer, color, sideNS);
                SDL_RenderDrawLine(renderer, fromX*TILE_SIZE_2D, fromY*TILE_SIZE_2D, hitX*TILE_SIZE_2D+TILE_SIZE_2D/2, hitY*TILE_SIZE_2D+TILE_SIZE_2D/2);
            }
            if(DRAW_3D){
                int h = (VIEW_HEIGHT*ASPECT_RATIO)/dist;
                int yStart = VIEW_HEIGHT/2-h/2;
                //int yEnd = VIEW_HEIGHT/2+h/2;
                //SDL_RenderDrawLine(renderer, x, yStart, x, yEnd);
                
                int texX = (int)(wallX * TEXTURE_WIDTH);
                if((!sideNS && aX > 0) || (sideNS && aY < 0)){
                    texX = TEXTURE_WIDTH - texX - 1;
                }
                for(int texY = 0; texY < TEXTURE_HEIGHT; texY++){
                    int yS = yStart + (int)(texY*(h/(float)TEXTURE_HEIGHT));
                    int yE = yStart + (int)((texY+1)*(h/(float)TEXTURE_HEIGHT));
                    if(yE >= 0 && yS < VIEW_HEIGHT){
                        unsigned int color = tileTextures[safeTile(hitX, hitY)-1][texX][texY];
                        setRenderDrawColorRGB(renderer, color, sideNS);
                        SDL_RenderDrawLine(renderer, x, yS, x, yE);
                    }
                }
            }
        }
        
        if(DRAW_3D){
            for(Entity& e : entityList){
                if(e.dist < firstDist || firstDist<0){
                    float eAngleSize = atan(1/e.dist);
                    float eAngle = atan2(e.y-posY, e.x-posX);
                    float yangle = atan2(sin(yawLook+angle), cos(yawLook+angle));
                    float ac = (yangle - eAngle);
                    while(ac < -PI){
                        ac += 2*PI;
                    }
                    while(ac > PI){
                        ac -= 2*PI;
                    }
                    float w = ac / eAngleSize;
                    if(abs(w) <= .5){
                        float h = (VIEW_HEIGHT*ASPECT_RATIO)/e.dist;
                        int yStart = VIEW_HEIGHT/2-h/2;
                        
                        int texX = (int)((w+.5) * TEXTURE_WIDTH);
                        for(int texY = 0; texY < TEXTURE_HEIGHT; texY++){
                            int yS = yStart + (int)(texY*(h/(float)TEXTURE_HEIGHT));
                            int yE = yStart + (int)((texY+1)*(h/(float)TEXTURE_HEIGHT));
                            if(yE >= 0 && yS < VIEW_HEIGHT){
                                unsigned int color = entityTextures[e.type][texX][texY];
                                if(color >> 24){
                                    setRenderDrawColorRGB(renderer, color);
                                    SDL_RenderDrawLine(renderer, x, yS, x, yE);
                                }
                            }
                        }
                    }
                }
            }
        }
        
    }
    
    SDL_Rect rect;
    rect.w = SCREEN_WIDTH;
    rect.h = HUD_HEIGHT;
    rect.x = 0;
    rect.y = VIEW_HEIGHT;
    setRenderDrawColorRGB(renderer, HUD_COLOR);
    SDL_RenderFillRect(renderer, &rect);
    
    setRenderDrawColorRGB(renderer, 0xFFFFFF);
    drawString(renderer, 2, VIEW_HEIGHT+2, ("FPS: "+std::to_string(averageFps)).c_str());
    
    
}

void init(){
    
    loadPngAsTexture(tileTextures[TILE_BRICK-1], "brick.png");
    loadPngAsTexture(tileTextures[TILE_METAL-1], "metal.png");
    loadPngAsTexture(tileTextures[TILE_BLUE_WALL-1], "blue wall.png");
    loadPngAsTexture(tileTextures[TILE_GREEN_BRICK-1], "green brick.png");
    
    
    loadPngAsTexture(entityTextures[ENTITY_TYPE_TEST1], "test.png");
    loadPngAsTexture(entityTextures[ENTITY_TYPE_TEST2], "test2.png");
    
    loadPngAsFont("font.png");
    
    
    loadPngAsLevel("level.png");
    
    
    posX = (LEVEL_WIDTH) / 2;
    posY = (LEVEL_HEIGHT) / 2;
    yawLook = 0;
    
    oldTicks = SDL_GetTicks();
    newTicks = SDL_GetTicks();
}

int main(int argc, const char * argv[]) {
    
    SDL_Window* window;
    SDL_Renderer *renderer;
    
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        fprintf(stderr, "Unable to initialize SDL:  %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &window, &renderer);
    
    if(!window){
        fprintf(stderr, "Unable to create window and renderer:  %s\n", SDL_GetError());
        return 1;
    }
    
    if(ENABLE_MOUSE){
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    
    init();
    
    SDL_Event event;
    
    bool running = true;
    
    while (running) {
        SDL_PollEvent(&event);
        switch(event.type){
            case SDL_QUIT:
            {
                running = false;
                break;
            }
            case SDL_KEYDOWN:
            {
                switch(event.key.keysym.sym){
                    case 'a': inputs[INP_MOVE_LEFT]     = true; break;
                    case 'd': inputs[INP_MOVE_RIGHT]    = true; break;
                    case 'w': inputs[INP_MOVE_FORWARD]  = true; break;
                    case 's': inputs[INP_MOVE_BACKWARD] = true; break;
                    case SDLK_LEFT: inputs[INP_TURN_LEFT]     = true; break;
                    case SDLK_RIGHT: inputs[INP_TURN_RIGHT]    = true; break;
                }
                break;
            }
            case SDL_KEYUP:
            {
                switch(event.key.keysym.sym){
                    case 'a': inputs[INP_MOVE_LEFT]     = false; break;
                    case 'd': inputs[INP_MOVE_RIGHT]    = false; break;
                    case 'w': inputs[INP_MOVE_FORWARD]  = false; break;
                    case 's': inputs[INP_MOVE_BACKWARD] = false; break;
                    case SDLK_LEFT: inputs[INP_TURN_LEFT]     = false; break;
                    case SDLK_RIGHT: inputs[INP_TURN_RIGHT]    = false; break;
                }
                break;
            }
            case SDL_MOUSEMOTION:
            {
                if(ENABLE_MOUSE){
                    mouseXDif += event.motion.xrel;
                    mouseYDif += event.motion.yrel;
                }
            }
        }
        
        setRenderDrawColorRGB(renderer, CLEAR_COLOR);
        SDL_RenderClear(renderer);
        
        
        if(DRAW_3D){
            SDL_Rect rect;
            
            rect.w = VIEW_WIDTH;
            rect.h = VIEW_HEIGHT/2;
            rect.x = 0;
            rect.y = 0;
            setRenderDrawColorRGB(renderer, CEIL_COLOR);
            SDL_RenderFillRect(renderer, &rect);
            
            rect.y = VIEW_HEIGHT/2;
            setRenderDrawColorRGB(renderer, FLOOR_COLOR);
            SDL_RenderFillRect(renderer, &rect);
        }
        
        draw(renderer);
        
        SDL_RenderPresent(renderer);
        
        oldTicks = newTicks;
        newTicks = SDL_GetTicks();
        frameTime = (newTicks - oldTicks) / 1000.0f;
        sumFps += 1.0f / frameTime;
        countFps++;
        if(countFps==20){
            averageFps = (int)floor(sumFps / countFps);
            countFps = 0;
            sumFps = 0;
        }
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    
    return 0;
}
