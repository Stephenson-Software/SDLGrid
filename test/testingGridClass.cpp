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

void init();

void loadMedia();

void cleanUp();

void init() {
	SDL_Init(SDL_INIT_VIDEO);

	// set texture filtering to linear
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	gWindow = SDL_CreateWindow("blank", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	
	theGrid.init(COLUMNS, ROWS, SCREEN_WIDTH, SCREEN_HEIGHT);
	theGrid.setRenderer(gRenderer);
	
	// initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	IMG_Init(imgFlags);
}

void loadMedia() {
	SDL_Surface* tempSurface;
	tempSurface = IMG_Load("grassTexture.png");
	grassTexture = SDL_CreateTextureFromSurface(gRenderer, tempSurface);
	SDL_FreeSurface(tempSurface);
}

void cleanUp() {
	SDL_DestroyTexture(grassTexture);
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	IMG_Quit();
	SDL_Quit();
}

void setRow(int row, SDL_Texture* textureToSet) {
	// set row to be grass
	for (int i = 0; i < COLUMNS; i++) {
		theGrid.getGridSlot(i, row).setTexture(grassTexture);
	}
}

int main(int argc, char* args[]) {
	init();
	loadMedia();
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