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

const int GROUND_ROW = 15; // rows above are sky, rows below are dirt

const int SCREEN_WIDTH = 500; // must be multiples of COLUMNS
const int SCREEN_HEIGHT = 500; // must be multiples of ROWS

SDL_Window* gWindow = NULL;

SDL_Renderer* gRenderer = NULL;

// what the 2D grid is drawn into while the 3D window is shown instead
SDL_Surface* gOffscreen = NULL;

// how each texture's slots look in 3D
gridview3d::Looks looks;

SDL_Texture* dirtTexture = NULL;
SDL_Texture* grassTexture = NULL;
SDL_Texture* skyTexture = NULL;
SDL_Texture* userTexture = NULL;

GridClass theGrid;

bool init();

// loads a PNG as a texture; in 3D, its slots stand height3D tall in its average colour
SDL_Texture* loadTexture(const char* path, bool useColorKey, float height3D);

bool loadMedia();

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
	SDL_Rect getRect();
};

user userControl;

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
	
	userControl.init(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 25, 50);
	
	// initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		cerr << "IMG_Init failed: " << IMG_GetError() << endl;
		return false;
	}
	return true;
}

SDL_Texture* loadTexture(const char* path, bool useColorKey, float height3D) {
	SDL_Surface* tempSurface = IMG_Load(path);
	if (tempSurface == NULL) {
		cerr << "Unable to load " << path << ": " << IMG_GetError() << endl;
		return NULL;
	}
	if (useColorKey) {
		SDL_SetColorKey(tempSurface, SDL_TRUE, SDL_MapRGB(tempSurface->format, 0, 0xFF, 0xFF));
	}
	SDL_Texture* texture = SDL_CreateTextureFromSurface(gRenderer, tempSurface);
	if (texture == NULL) {
		cerr << "Unable to create a texture from " << path << ": " << SDL_GetError() << endl;
	} else {
		looks.set(texture, gridview3d::lookFromSurface(tempSurface, height3D));
	}
	SDL_FreeSurface(tempSurface);
	return texture;
}

bool loadMedia() {
	// in 3D the sky lies flat and the ground stands up, the grass a little
	// above the dirt
	dirtTexture = loadTexture("dirtTexture.png", false, 0.6f);
	grassTexture = loadTexture("grassTexture.png", false, 0.75f);
	skyTexture = loadTexture("skyTexture.png", false, 0.04f);
	userTexture = loadTexture("userTexture.png", true, 1.5f);
	return dirtTexture != NULL && grassTexture != NULL && skyTexture != NULL && userTexture != NULL;
}

void cleanUp() {
	SDL_DestroyTexture(dirtTexture);
	SDL_DestroyTexture(grassTexture);
	SDL_DestroyTexture(skyTexture);
	SDL_DestroyTexture(userTexture);
	SDL_DestroyRenderer(gRenderer);
	if (gWindow != NULL) {
		SDL_DestroyWindow(gWindow);
	}
	SDL_FreeSurface(gOffscreen);
	gridview3d::close();
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

SDL_Rect user::getRect() {
	SDL_Rect rect = {xpos, ypos, width, height};
	return rect;
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

// the sprite in 3D: a block in its texture's colour over its rectangle
gridview3d::Marker userMarker() {
	const gridview3d::Look look = looks.get(userTexture);
	gridview3d::Marker marker = {userControl.getRect(), {look.color[0], look.color[1], look.color[2]}, look.height};
	return marker;
}

void setRow(int row, SDL_Texture* textureToSet) {
	// set every slot in the row to the given texture
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
	for (int i = 0; i < GROUND_ROW; i++) {
		setRow(i, skyTexture);
	}
	setSolidRow(GROUND_ROW);
	setRow(GROUND_ROW, grassTexture);
	for (int i = GROUND_ROW + 1; i < ROWS; i++) {
		setRow(i, dirtTexture);
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
	open3D(argc, args, "simpleLevel");
	if (!init() || !loadMedia()) {
		cleanUp();
		return 1;
	}
	theGrid.createGrid();
	buildLevel();
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

		theGrid.drawGrid();		
		userControl.render();
		userControl.move();
		
		SDL_RenderPresent(gRenderer);

		// and in 3D, when it is shown in 3D: the sprite as a block over the
		// slots it covers
		gridview3d::show(theGrid, looks, std::vector<gridview3d::Marker>(1, userMarker()));
	}
	cleanUp();
}