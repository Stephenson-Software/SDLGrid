#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <vector>
#include "GridClass.h"

using namespace std;

const int COLUMNS = 20;
const int ROWS = 20;

const int SCREEN_WIDTH = 500; // must be multiples of COLUMNS
const int SCREEN_HEIGHT = 500; // must be multiples of ROWS

SDL_Window* gWindow = NULL;

SDL_Renderer* gRenderer = NULL;

SDL_Texture* grassTexture = NULL;

GridClass theGrid;

bool init();

bool loadMedia();

void cleanUp();

bool init() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		cerr << "SDL_Init failed: " << SDL_GetError() << endl;
		return false;
	}

	// set texture filtering to linear
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	gWindow = SDL_CreateWindow("blank", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (gWindow == NULL) {
		cerr << "SDL_CreateWindow failed: " << SDL_GetError() << endl;
		return false;
	}
	gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (gRenderer == NULL) {
		cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << endl;
		return false;
	}
	
	theGrid.init(COLUMNS, ROWS, SCREEN_WIDTH, SCREEN_HEIGHT);
	theGrid.setRenderer(gRenderer);
	
	// initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		cerr << "IMG_Init failed: " << IMG_GetError() << endl;
		return false;
	}
	return true;
}

bool loadMedia() {
	SDL_Surface* tempSurface;
	tempSurface = IMG_Load("grassTexture.png");
	if (tempSurface == NULL) {
		cerr << "Unable to load grassTexture.png: " << IMG_GetError() << endl;
		return false;
	}
	grassTexture = SDL_CreateTextureFromSurface(gRenderer, tempSurface);
	if (grassTexture == NULL) {
		cerr << "Unable to create a texture from grassTexture.png: " << SDL_GetError() << endl;
	}
	SDL_FreeSurface(tempSurface);
	return grassTexture != NULL;
}

void cleanUp() {
	SDL_DestroyTexture(grassTexture);
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	IMG_Quit();
	SDL_Quit();
}

void setRow(int row, SDL_Texture* textureToSet) {
	// set every slot in the row to the given texture
	for (int i = 0; i < COLUMNS; i++) {
		theGrid.getGridSlot(i, row).setTexture(textureToSet);
	}
}

int main(int argc, char* args[]) {
	if (!init() || !loadMedia()) {
		cleanUp();
		return 1;
	}
	theGrid.createGrid();
	SDL_Event e;
	bool running = true;
	while (running) {
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				running = false;
			}
		}
		SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(gRenderer);

//		setRow(9, grassTexture);

		theGrid.drawGrid();		
		
		SDL_RenderPresent(gRenderer);
	}
	cleanUp();
}