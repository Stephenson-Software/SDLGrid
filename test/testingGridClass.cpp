#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "GridClass.h"
#include "GridView3DStrata.h"

using namespace std;

const int COLUMNS = 20;
const int ROWS = 20;

const int SCREEN_WIDTH = 500; // must be multiples of COLUMNS
const int SCREEN_HEIGHT = 500; // must be multiples of ROWS

SDL_Window* gWindow = NULL;

SDL_Renderer* gRenderer = NULL;

// what the 2D grid is drawn into while the 3D window is shown instead
SDL_Surface* gOffscreen = NULL;

// how each texture's slots look in 3D
gridview3d::Looks looks;

SDL_Texture* grassTexture = NULL;

GridClass theGrid;

bool init();

bool loadMedia();

void cleanUp();

// opens the 2D window, or, while the grid is shown in its own 3D window
// (src/GridView3DStrata.h), only SDL's events and a software renderer that
// draws the 2D grid into memory: SDL's video subsystem would take over the GL
// context the 3D window draws with
bool initWindowAndRenderer() {
	if (gridview3d::ownsWindow()) {
		if (SDL_Init(SDL_INIT_EVENTS) < 0) {
			cerr << "SDL_Init failed: " << SDL_GetError() << endl;
			return false;
		}
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
		gOffscreen = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
		gRenderer = gOffscreen == NULL ? NULL : SDL_CreateSoftwareRenderer(gOffscreen);
		if (gRenderer == NULL) {
			cerr << "SDL_CreateSoftwareRenderer failed: " << SDL_GetError() << endl;
			return false;
		}
		return true;
	}

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
	return true;
}

bool init() {
	if (!initWindowAndRenderer()) {
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
	looks.set(grassTexture, gridview3d::lookFromSurface(tempSurface, 0.5f));
	if (grassTexture == NULL) {
		cerr << "Unable to create a texture from grassTexture.png: " << SDL_GetError() << endl;
	}
	SDL_FreeSurface(tempSurface);
	return grassTexture != NULL;
}

void cleanUp() {
	SDL_DestroyTexture(grassTexture);
	SDL_DestroyRenderer(gRenderer);
	if (gWindow != NULL) {
		SDL_DestroyWindow(gWindow);
	}
	SDL_FreeSurface(gOffscreen);
	gridview3d::close();
	IMG_Quit();
	SDL_Quit();
}

void setRow(int row, SDL_Texture* textureToSet) {
	// set every slot in the row to the given texture
	for (int i = 0; i < COLUMNS; i++) {
		theGrid.getGridSlot(i, row).setTexture(textureToSet);
	}
}

// the next event: from SDL, then from the 3D window, whose input arrives as
// the events the 2D window would have produced
int pollEvent(SDL_Event* e) {
	if (SDL_PollEvent(e) != 0) {
		return 1;
	}
	return gridview3d::pollEvent(e);
}

// the grid is shown in 3D unless --2d is given or Strata can't run here
void open3D(int argc, char* args[], const char* title) {
	for (int i = 1; i < argc; i++) {
		if (strcmp(args[i], "--2d") == 0) {
			return;
		}
	}
	if (!gridview3d::open(SCREEN_WIDTH, SCREEN_HEIGHT, title)) {
		cerr << "Showing the grid in 2D: " << gridview3d::reason() << endl;
	}
}

int main(int argc, char* args[]) {
	open3D(argc, args, "testingGridClass");
	if (!init() || !loadMedia()) {
		cleanUp();
		return 1;
	}
	theGrid.createGrid();
	SDL_Event e;
	bool running = true;
	while (running) {
		while (pollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				running = false;
			}
		}
		SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderClear(gRenderer);

//		setRow(9, grassTexture);

		theGrid.drawGrid();		
		
		SDL_RenderPresent(gRenderer);

		// and in 3D, when it is shown in 3D
		gridview3d::show(theGrid, looks);
	}
	cleanUp();
}