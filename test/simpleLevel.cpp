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

SDL_Texture* dirtTexture = NULL;
SDL_Texture* grassTexture = NULL;
SDL_Texture* skyTexture = NULL;
SDL_Texture* userTexture = NULL;

GridClass theGrid;

void init();

void loadMedia();

void cleanUp();

bool checkCollision(SDL_Rect a, SDL_Rect b);

bool checkIfHitSolid(SDL_Rect toCheck);

class user {
  private:
	int xpos;
	int ypos;
	int width;
	int height;
	int xvel;
	int yvel;
	SDL_Rect collider;
  public:
	user();
	void init(int x, int y, int w, int h);
	void render();
	void move();
};

user userControl;

void init() {
	SDL_Init(SDL_INIT_VIDEO);

	// set texture filtering to linear
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	gWindow = SDL_CreateWindow("blank", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	
	theGrid.init(COLUMNS, ROWS, SCREEN_WIDTH, SCREEN_HEIGHT);
	theGrid.setRenderer(gRenderer);
	
	userControl.init(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 25, 50);
	
	// initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	IMG_Init(imgFlags);
}

void loadMedia() {
	SDL_Surface* tempSurface;
	tempSurface = IMG_Load("dirtTexture.png");
	dirtTexture = SDL_CreateTextureFromSurface(gRenderer, tempSurface);
	tempSurface = IMG_Load("grassTexture.png");
	grassTexture = SDL_CreateTextureFromSurface(gRenderer, tempSurface);
	tempSurface = IMG_Load("skyTexture.png");
	skyTexture = SDL_CreateTextureFromSurface(gRenderer, tempSurface);
	tempSurface = IMG_Load("userTexture.png");
	SDL_SetColorKey(tempSurface, SDL_TRUE, SDL_MapRGB(tempSurface->format, 0, 0xFF, 0xFF));
	userTexture = SDL_CreateTextureFromSurface(gRenderer, tempSurface);
	SDL_FreeSurface(tempSurface);
}

void cleanUp() {
	SDL_DestroyTexture(grassTexture);
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	IMG_Quit();
	SDL_Quit();
}

user::user() {
	xpos = 0;
	ypos = 0;
	width = 0;
	height = 0;
	xvel = 0;
	yvel = 3;
	collider = {xpos, ypos, width, height};
}

void user::init(int x, int y, int w, int h) {
	xpos = x;
	ypos = y;
	width = w;
	height = h;
	collider = {xpos, ypos, width, height};
}

void user::render() {
	SDL_Rect dest = {xpos, ypos, width, height};
	SDL_RenderCopy(gRenderer, userTexture, NULL, &dest);
}

void user::move() {
	xpos += xvel;
	
	collider.x = xpos;
	
	if (checkIfHitSolid(collider)) {
		xpos -= xvel;
		xvel = 0;
	}
	
	ypos += yvel;
	collider.y = ypos;
	
	if (checkIfHitSolid(collider)) {
		ypos -= yvel;
		yvel = 0;
	}
}

bool checkCollision(SDL_Rect a, SDL_Rect b) {
	int leftA = a.x;
	int rightA = a.x + a.w;
	int topA = a.y;
	int bottomA = a.y + a.h;
	int leftB = b.x;
	int rightB = b.x + b.w;
	int topB = b.y;
	int bottomB = b.y + b.h;
	
	if (rightA <= leftB) {
		return false;
	}
	
	if (leftA >= rightB) {
		return false;
	}
	
	if (bottomA <= topB) {
		return false;
	}
	
	if (topA >= bottomB) {
		return false;
	}
	
	return true;
}

bool checkIfHitSolid(SDL_Rect toCheck) {
	for (size_t i = 0; i < theGrid.solidSlots.size(); i++) {
		if (checkCollision(toCheck, theGrid.solidSlots[i].collider)) {
			return true;
		}
	}
	return false;
}

void setRow(int row, SDL_Texture* textureToSet) {
	// set row to be grass
	for (int i = 0; i < COLUMNS; i++) {
		theGrid.getGridSlot(i, row).setTexture(textureToSet);
	}
}

void setSolidRow(int row) {
	// set row to be solid
	for (int i = 0; i < COLUMNS; i++) {
		theGrid.getGridSlot(i, row).setSolidFlag(true);
		theGrid.solidSlots.push_back(theGrid.getGridSlot(i, row));
	}
}

void buildLevel() {
	for (int i = 0; i < 15; i++) {
		setRow(i, skyTexture);
	}
	setSolidRow(15);
	setRow(15, grassTexture);
	for (int i = 16; i < 20; i++) {
		setRow(i, dirtTexture);
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

		buildLevel();

		theGrid.drawGrid();		
		userControl.render();
		userControl.move();
		
		SDL_RenderPresent(gRenderer);
	}
	cleanUp();
}