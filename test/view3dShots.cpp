// Draws the two demo levels in 3D headlessly with desktop Strata and saves
// each as a PNG, in the demos' 500 x 500 window and a phone held upright
// (390 x 844): testingGridClass's outlined 20x20 grid and simpleLevel's level with its sprite
// resting on the ground row. The levels are built as the demos build them,
// from the same PNGs, with the same Looks.
//
//   cd test
//   g++ -std=c++11 -I../src ../src/GridClass.cpp view3dShots.cpp $(sdl2-config --cflags --libs) -lSDL2_image -ldl -o view3dShots
//   STRATA_LIBRARY=<path to libstrata.so.0.9> ./view3dShots <output directory>
//
// Needs no display: SDL draws into memory and Strata renders offscreen (EGL).
// Exits 1 when Strata can't run here or a picture can't be saved.
#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include <string>
#include <vector>
#include "GridClass.h"
#include "GridView3DStrata.h"

const int COLUMNS = 20;
const int ROWS = 20;
const int GROUND_ROW = 15;
const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;

SDL_Texture* load(SDL_Renderer* renderer, gridview3d::Looks& looks, const char* path, bool colorKey, float height) {
	SDL_Surface* surface = IMG_Load(path);
	if (surface == NULL) {
		std::fprintf(stderr, "could not load %s: %s\n", path, IMG_GetError());
		return NULL;
	}
	if (colorKey) {
		SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0, 0xFF, 0xFF));
	}
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (texture != NULL) {
		looks.set(texture, gridview3d::lookFromSurface(surface, height));
	}
	SDL_FreeSurface(surface);
	return texture;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::fprintf(stderr, "usage: %s <output directory>\n", argv[0]);
		return 2;
	}
	const std::string dir = argv[1];
	SDL_Surface* offscreen = SDL_CreateRGBSurfaceWithFormat(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
	SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(offscreen);
	gridview3d::Looks looks;
	SDL_Texture* dirt = load(renderer, looks, "dirtTexture.png", false, 0.6f);
	SDL_Texture* grass = load(renderer, looks, "grassTexture.png", false, 0.75f);
	SDL_Texture* sky = load(renderer, looks, "skyTexture.png", false, 0.04f);
	SDL_Texture* user = load(renderer, looks, "userTexture.png", true, 1.5f);
	if (!dirt || !grass || !sky || !user) {
		return 1;
	}

	GridClass outlined;
	outlined.init(COLUMNS, ROWS, SCREEN_WIDTH, SCREEN_HEIGHT);
	outlined.setRenderer(renderer);
	outlined.createGrid();

	GridClass level;
	level.init(COLUMNS, ROWS, SCREEN_WIDTH, SCREEN_HEIGHT);
	level.setRenderer(renderer);
	level.createGrid();
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLUMNS; c++) {
			level.getGridSlot(c, r).setTexture(r < GROUND_ROW ? sky : r == GROUND_ROW ? grass : dirt);
		}
	}
	const gridview3d::Look userLook = looks.get(user);
	const gridview3d::Marker sprite = {{SCREEN_WIDTH / 2, GROUND_ROW * (SCREEN_HEIGHT / ROWS) - 50, 25, 50},
			{userLook.color[0], userLook.color[1], userLook.color[2]}, userLook.height};

	struct Size {
		const char* name;
		int width, height;
	};
	const Size sizes[] = {{"500x500", 500, 500}, {"phone-390x844", 390, 844}};
	for (const Size& size : sizes) {
		if (!gridview3d::open(size.width, size.height, nullptr, true)) {
			std::fprintf(stderr, "no 3D view here: %s\n", gridview3d::reason().c_str());
			return 1;
		}
		for (int which = 0; which < 2; which++) {
			GridClass& grid = which == 0 ? outlined : level;
			std::vector<gridview3d::Marker> markers;
			if (which == 1) {
				markers.push_back(sprite);
			}
			// the first frame of a fresh engine fits its shadow cascades; draw twice
			gridview3d::show(grid, looks, markers);
			gridview3d::show(grid, looks, markers);
			const std::string path = dir + "/sdlgrid-3d-" + (which == 0 ? "testingGridClass" : "simpleLevel") + "-" +
					size.name + ".png";
			if (!gridview3d::savePng(path.c_str())) {
				std::fprintf(stderr, "could not save %s\n", path.c_str());
				return 1;
			}
			std::printf("%s\n", path.c_str());
		}
		gridview3d::close();
	}
	SDL_DestroyRenderer(renderer);
	SDL_FreeSurface(offscreen);
	return 0;
}
