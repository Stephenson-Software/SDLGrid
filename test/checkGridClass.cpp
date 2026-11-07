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

const int SLOT_WIDTH = SCREEN_WIDTH / COLUMNS;
const int SLOT_HEIGHT = SCREEN_HEIGHT / ROWS;

// the drawing checks share this surface, and unless a check says otherwise they lay it out
// as a row of two 10x10 slots, so the first slot's top edge is at (5, 0)
// and its middle at (5, 5), and the second slot's top edge is at (15, 0) and its middle at (15, 5)
const int DRAWN_COLUMNS = 2;
const int DRAWN_ROWS = 1;
const int DRAWN_WIDTH = 20;
const int DRAWN_HEIGHT = 10;

// the red channel of the three colours a drawing check can read back
const int RED_OF_WHITE = 0xFF;
const int RED_OF_BLACK = 0x00;
const int RED_OF_TEXTURE = 0x40;

GridClass theGrid;

int failures = 0;

void expectEqual(const char* description, int expected, int actual);

void checkInit();

void checkSlotLayout();

void checkCreateGridReplacesSlots();

void checkSlotSetters();

void checkInitRejectsNonPositiveCounts();

void checkNonSquareSlotLayout();

void checkNonMultipleScreenSize();

void clearToWhite(SDL_Renderer* renderer);

void redrawOnWhite(SDL_Renderer* renderer, GridClass& grid);

int redAt(SDL_Renderer* renderer, int x, int y);

void checkSlotDrawing(SDL_Renderer* renderer);

void checkRendererSetAfterCreateGrid(SDL_Renderer* renderer);

void checkDrawRectangleArguments(SDL_Renderer* renderer);

void checkColliderDoesNotMoveDrawing(SDL_Renderer* renderer);

void checkUncoveredScreenEdges(SDL_Renderer* renderer);

void checkDrawing();

void expectEqual(const char* description, int expected, int actual) {
	if (expected != actual) {
		cerr << "FAIL: " << description << " (expected " << expected << ", got " << actual << ")" << endl;
		failures++;
	}
}

void checkInit() {
	theGrid.init(COLUMNS, ROWS, SCREEN_WIDTH, SCREEN_HEIGHT);
	expectEqual("getColumns after init", COLUMNS, theGrid.getColumns());
	expectEqual("getRows after init", ROWS, theGrid.getRows());
}

void checkSlotLayout() {
	// no renderer is set, so every slot is built with a NULL renderer and render() is a no-op
	theGrid.createGrid();
	expectEqual("collider.x of slot (0, 0)", 0, theGrid.getGridSlot(0, 0).collider.x);
	expectEqual("collider.y of slot (0, 0)", 0, theGrid.getGridSlot(0, 0).collider.y);
	expectEqual("collider.w of slot (0, 0)", SLOT_WIDTH, theGrid.getGridSlot(0, 0).collider.w);
	expectEqual("collider.h of slot (0, 0)", SLOT_HEIGHT, theGrid.getGridSlot(0, 0).collider.h);
	expectEqual("collider.x of slot (1, 0)", SLOT_WIDTH, theGrid.getGridSlot(1, 0).collider.x);
	expectEqual("collider.y of slot (0, 1)", SLOT_HEIGHT, theGrid.getGridSlot(0, 1).collider.y);
	expectEqual("collider.x of the last slot", SCREEN_WIDTH - SLOT_WIDTH, theGrid.getGridSlot(COLUMNS - 1, ROWS - 1).collider.x);
	expectEqual("collider.y of the last slot", SCREEN_HEIGHT - SLOT_HEIGHT, theGrid.getGridSlot(COLUMNS - 1, ROWS - 1).collider.y);
}

void checkCreateGridReplacesSlots() {
	// re-initialise with half as many columns and rows, so every slot doubles in size;
	// if createGrid appended instead of replacing, getGridSlot would still resolve into
	// the stale slots at the front of the vector, whose sizes were computed for the old layout
	int halfColumns = COLUMNS / 2;
	int halfRows = ROWS / 2;
	theGrid.init(halfColumns, halfRows, SCREEN_WIDTH, SCREEN_HEIGHT);
	theGrid.createGrid();
	expectEqual("getColumns after a second init", halfColumns, theGrid.getColumns());
	expectEqual("getRows after a second init", halfRows, theGrid.getRows());
	expectEqual("collider.w of slot (0, 0) after a second createGrid", SLOT_WIDTH * 2, theGrid.getGridSlot(0, 0).collider.w);
	expectEqual("collider.h of slot (0, 0) after a second createGrid", SLOT_HEIGHT * 2, theGrid.getGridSlot(0, 0).collider.h);
	expectEqual("collider.x of slot (1, 0) after a second createGrid", SLOT_WIDTH * 2, theGrid.getGridSlot(1, 0).collider.x);
	expectEqual("collider.y of slot (0, 1) after a second createGrid", SLOT_HEIGHT * 2, theGrid.getGridSlot(0, 1).collider.y);
	expectEqual("collider.x of the last slot after a second createGrid", SCREEN_WIDTH - SLOT_WIDTH * 2, theGrid.getGridSlot(halfColumns - 1, halfRows - 1).collider.x);
	expectEqual("collider.y of the last slot after a second createGrid", SCREEN_HEIGHT - SLOT_HEIGHT * 2, theGrid.getGridSlot(halfColumns - 1, halfRows - 1).collider.y);
}

void checkSlotSetters() {
	// setSolidFlag and setButtonFlag expose no getter, so they are called for their signatures
	// only, and fillBlack and clear do nothing visible here because no renderer is set (see
	// checkSlotDrawing); the flagged slot is copied into the public tracking vectors in the
	// same way simpleLevel.cpp populates solidSlots
	GridSlot& slot = theGrid.getGridSlot(0, 0);
	slot.fillBlack();
	slot.render();
	slot.clear();
	slot.render();
	slot.setSolidFlag(true);
	theGrid.solidSlots.push_back(slot);
	slot.setButtonFlag(true);
	theGrid.buttonSlots.push_back(slot);
	expectEqual("solidSlots size", 1, (int) theGrid.solidSlots.size());
	expectEqual("buttonSlots size", 1, (int) theGrid.buttonSlots.size());
	expectEqual("collider.w of the copied button slot", SLOT_WIDTH * 2, theGrid.buttonSlots[0].collider.w);
}

void checkInitRejectsNonPositiveCounts() {
	// each rejected init below prints its own message on stderr; only lines starting
	// with FAIL: are failed expectations
	GridClass freshGrid;
	freshGrid.init(3, -1, SCREEN_WIDTH, SCREEN_HEIGHT);
	expectEqual("getColumns after a rejected init on a fresh grid", 0, freshGrid.getColumns());
	expectEqual("getRows after a rejected init on a fresh grid", 0, freshGrid.getRows());

	// a rejected init stores nothing, so a grid that was already initialised keeps the
	// dimensions and slot sizes of its previous init rather than returning to zero
	GridClass initialisedGrid;
	initialisedGrid.init(4, 5, 200, 300);
	initialisedGrid.init(0, 7, SCREEN_WIDTH, SCREEN_HEIGHT);
	expectEqual("getColumns after a rejected init on an initialised grid", 4, initialisedGrid.getColumns());
	expectEqual("getRows after a rejected init on an initialised grid", 5, initialisedGrid.getRows());
	initialisedGrid.createGrid();
	expectEqual("collider.w of slot (0, 0) after a rejected init", 50, initialisedGrid.getGridSlot(0, 0).collider.w);
	expectEqual("collider.h of slot (0, 0) after a rejected init", 60, initialisedGrid.getGridSlot(0, 0).collider.h);
}

void checkNonSquareSlotLayout() {
	// every other check uses as many columns as rows, which cannot tell a row stride of
	// columns from a row stride of rows in getGridSlot; with 4 columns and 5 rows, a stride
	// of rows would resolve (0, 1) to the slot at (1, 1) and (3, 1) to the slot at (0, 2)
	GridClass tallGrid;
	tallGrid.init(4, 5, 200, 300);
	tallGrid.createGrid();
	expectEqual("collider.x of slot (3, 0) on a 4x5 grid", 150, tallGrid.getGridSlot(3, 0).collider.x);
	expectEqual("collider.y of slot (3, 0) on a 4x5 grid", 0, tallGrid.getGridSlot(3, 0).collider.y);
	expectEqual("collider.x of slot (0, 1) on a 4x5 grid", 0, tallGrid.getGridSlot(0, 1).collider.x);
	expectEqual("collider.y of slot (0, 1) on a 4x5 grid", 60, tallGrid.getGridSlot(0, 1).collider.y);
	expectEqual("collider.x of slot (3, 1) on a 4x5 grid", 150, tallGrid.getGridSlot(3, 1).collider.x);
	expectEqual("collider.y of slot (3, 1) on a 4x5 grid", 60, tallGrid.getGridSlot(3, 1).collider.y);
	expectEqual("collider.x of the last slot on a 4x5 grid", 150, tallGrid.getGridSlot(3, 4).collider.x);
	expectEqual("collider.y of the last slot on a 4x5 grid", 240, tallGrid.getGridSlot(3, 4).collider.y);
	expectEqual("collider.w of the last slot on a 4x5 grid", 50, tallGrid.getGridSlot(3, 4).collider.w);
	expectEqual("collider.h of the last slot on a 4x5 grid", 60, tallGrid.getGridSlot(3, 4).collider.h);
}

void checkNonMultipleScreenSize() {
	// 210 is not a multiple of 4 and 313 is not a multiple of 5, so init truncates the slot
	// size to 52x62 rather than rounding it to 53x63, and the grid stops short of the
	// right and bottom edges (208 and 310) instead of being stretched to reach 210 and 313
	GridClass unevenGrid;
	unevenGrid.init(4, 5, 210, 313);
	unevenGrid.createGrid();
	expectEqual("collider.w of slot (0, 0) on a 210x313 screen", 52, unevenGrid.getGridSlot(0, 0).collider.w);
	expectEqual("collider.h of slot (0, 0) on a 210x313 screen", 62, unevenGrid.getGridSlot(0, 0).collider.h);
	expectEqual("collider.x of the last slot on a 210x313 screen", 156, unevenGrid.getGridSlot(3, 4).collider.x);
	expectEqual("collider.y of the last slot on a 210x313 screen", 248, unevenGrid.getGridSlot(3, 4).collider.y);
	expectEqual("right edge of the last slot on a 210x313 screen", 208, unevenGrid.getGridSlot(3, 4).collider.x + unevenGrid.getGridSlot(3, 4).collider.w);
	expectEqual("bottom edge of the last slot on a 210x313 screen", 310, unevenGrid.getGridSlot(3, 4).collider.y + unevenGrid.getGridSlot(3, 4).collider.h);
}

void clearToWhite(SDL_Renderer* renderer) {
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderClear(renderer);
}

void redrawOnWhite(SDL_Renderer* renderer, GridClass& grid) {
	clearToWhite(renderer);
	grid.drawGrid();
}

int redAt(SDL_Renderer* renderer, int x, int y) {
	SDL_Rect pixelRect = {x, y, 1, 1};
	Uint32 pixel = 0;
	if (SDL_RenderReadPixels(renderer, &pixelRect, SDL_PIXELFORMAT_ARGB8888, &pixel, sizeof(pixel)) != 0) {
		// -1 matches no colour, so the expectation reading this pixel fails and is counted
		cerr << "SDL_RenderReadPixels failed: " << SDL_GetError() << endl;
		return -1;
	}
	return (pixel >> 16) & 0xFF;
}

void checkSlotDrawing(SDL_Renderer* renderer) {
	GridClass drawnGrid;
	drawnGrid.init(DRAWN_COLUMNS, DRAWN_ROWS, DRAWN_WIDTH, DRAWN_HEIGHT);
	drawnGrid.setRenderer(renderer);
	drawnGrid.createGrid();

	redrawOnWhite(renderer, drawnGrid);
	expectEqual("top edge of an outlined slot", RED_OF_BLACK, redAt(renderer, 5, 0));
	expectEqual("middle of an outlined slot", RED_OF_WHITE, redAt(renderer, 5, 5));

	// redrawOnWhite left the draw colour white, so black here can only have come from drawing
	Uint8 red = 0xFF;
	Uint8 green = 0xFF;
	Uint8 blue = 0xFF;
	Uint8 alpha = 0x00;
	SDL_GetRenderDrawColor(renderer, &red, &green, &blue, &alpha);
	expectEqual("red of the draw colour after drawGrid", RED_OF_BLACK, red);
	expectEqual("alpha of the draw colour after drawGrid", 0xFF, alpha);

	drawnGrid.getGridSlot(0, 0).fillBlack();
	redrawOnWhite(renderer, drawnGrid);
	expectEqual("middle of a filled slot", RED_OF_BLACK, redAt(renderer, 5, 5));
	expectEqual("middle of the slot beside a filled slot", RED_OF_WHITE, redAt(renderer, 15, 5));

	drawnGrid.getGridSlot(0, 0).clear();
	redrawOnWhite(renderer, drawnGrid);
	expectEqual("middle of a cleared slot", RED_OF_WHITE, redAt(renderer, 5, 5));

	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
	if (texture == NULL) {
		cerr << "FAIL: SDL_CreateTexture: " << SDL_GetError() << endl;
		failures++;
		return;
	}
	Uint32 texturePixel = 0xFF404040;
	SDL_UpdateTexture(texture, NULL, &texturePixel, sizeof(texturePixel));

	// the fill state is set under the texture, so the texture has to cover both the edge and
	// the middle, and removing the texture has to reveal the fill that was hidden
	drawnGrid.getGridSlot(0, 0).fillBlack();
	drawnGrid.getGridSlot(0, 0).setTexture(texture);
	redrawOnWhite(renderer, drawnGrid);
	expectEqual("top edge of a filled slot with a texture", RED_OF_TEXTURE, redAt(renderer, 5, 0));
	expectEqual("middle of a filled slot with a texture", RED_OF_TEXTURE, redAt(renderer, 5, 5));

	drawnGrid.getGridSlot(0, 0).setTexture(NULL);
	redrawOnWhite(renderer, drawnGrid);
	expectEqual("middle of a filled slot after its texture is removed", RED_OF_BLACK, redAt(renderer, 5, 5));

	// the first slot is still filled and the second is given the texture, so both are drawn
	// differently from a new slot until createGrid replaces them
	drawnGrid.getGridSlot(1, 0).setTexture(texture);
	redrawOnWhite(renderer, drawnGrid);
	expectEqual("middle of a filled slot before a second createGrid", RED_OF_BLACK, redAt(renderer, 5, 5));
	expectEqual("middle of a textured slot before a second createGrid", RED_OF_TEXTURE, redAt(renderer, 15, 5));

	drawnGrid.createGrid();
	redrawOnWhite(renderer, drawnGrid);
	expectEqual("middle of a filled slot after a second createGrid", RED_OF_WHITE, redAt(renderer, 5, 5));
	expectEqual("middle of a textured slot after a second createGrid", RED_OF_WHITE, redAt(renderer, 15, 5));

	SDL_DestroyTexture(texture);
}

void checkRendererSetAfterCreateGrid(SDL_Renderer* renderer) {
	// the slots are built with a NULL renderer, so GridClass::setRenderer alone must not reach them
	GridClass lateGrid;
	lateGrid.init(DRAWN_COLUMNS, DRAWN_ROWS, DRAWN_WIDTH, DRAWN_HEIGHT);
	lateGrid.createGrid();
	lateGrid.setRenderer(renderer);
	redrawOnWhite(renderer, lateGrid);
	expectEqual("top edge of a slot built before setRenderer", RED_OF_WHITE, redAt(renderer, 5, 0));

	lateGrid.getGridSlot(0, 0).setRenderer(renderer);
	redrawOnWhite(renderer, lateGrid);
	expectEqual("top edge of a slot given its own renderer", RED_OF_BLACK, redAt(renderer, 5, 0));
	expectEqual("top edge of the slot beside it", RED_OF_WHITE, redAt(renderer, 15, 0));

	lateGrid.createGrid();
	redrawOnWhite(renderer, lateGrid);
	expectEqual("top edge of a slot rebuilt after setRenderer", RED_OF_BLACK, redAt(renderer, 15, 0));
}

void checkDrawRectangleArguments(SDL_Renderer* renderer) {
	// the slot sits at the left of the surface and the rectangle at the right, spanning x 12 to
	// 17 and y 2 to 7, so a rectangle drawn from the slot's own position and size would show up
	// at (5, 0) and (5, 5) and leave the requested area white
	GridSlot slot;
	slot.setRenderer(renderer);
	slot.init(0, 0, 10, 10);

	clearToWhite(renderer);
	slot.drawRectangle(12, 2, 6, 6);
	expectEqual("top edge of a rectangle at the given position", RED_OF_BLACK, redAt(renderer, 15, 2));
	expectEqual("bottom edge of a rectangle of the given size", RED_OF_BLACK, redAt(renderer, 15, 7));
	expectEqual("middle of an outlined rectangle", RED_OF_WHITE, redAt(renderer, 15, 5));
	expectEqual("top edge of the slot asked to draw elsewhere", RED_OF_WHITE, redAt(renderer, 5, 0));

	slot.fillBlack();
	clearToWhite(renderer);
	slot.drawRectangle(12, 2, 6, 6);
	expectEqual("middle of a filled rectangle", RED_OF_BLACK, redAt(renderer, 15, 5));
	expectEqual("middle of the slot asked to fill elsewhere", RED_OF_WHITE, redAt(renderer, 5, 5));
}

void checkColliderDoesNotMoveDrawing(SDL_Renderer* renderer) {
	// render draws from the slot's private position, so moving the public collider to the
	// right of the surface leaves the outline at the left
	GridSlot slot;
	slot.setRenderer(renderer);
	slot.init(0, 0, 10, 10);
	slot.collider.x = 10;
	clearToWhite(renderer);
	slot.render();
	expectEqual("top edge of a slot whose collider was moved", RED_OF_BLACK, redAt(renderer, 5, 0));
	expectEqual("top edge at the position the collider was moved to", RED_OF_WHITE, redAt(renderer, 15, 0));

	slot.init(0, 0, 10, 10);
	expectEqual("collider.x after init overwrites a moved collider", 0, slot.collider.x);
}

void checkUncoveredScreenEdges(SDL_Renderer* renderer) {
	// 20 is not a multiple of 3 and 10 is not a multiple of 3, so the 3x3 grid has 6x3 slots
	// and stops at x 18 and y 9, leaving the last two columns and the last row of pixels
	// undrawn; rounding the slot size up to 7x4 would reach both edges of the surface
	GridClass unevenGrid;
	unevenGrid.init(3, 3, DRAWN_WIDTH, DRAWN_HEIGHT);
	unevenGrid.setRenderer(renderer);
	unevenGrid.createGrid();
	for (int row = 0; row < unevenGrid.getRows(); row++) {
		for (int column = 0; column < unevenGrid.getColumns(); column++) {
			unevenGrid.getGridSlot(column, row).fillBlack();
		}
	}
	redrawOnWhite(renderer, unevenGrid);
	expectEqual("last pixel covered by a filled 3x3 grid", RED_OF_BLACK, redAt(renderer, 17, 8));
	expectEqual("pixel right of a filled 3x3 grid", RED_OF_WHITE, redAt(renderer, 18, 8));
	expectEqual("pixel below a filled 3x3 grid", RED_OF_WHITE, redAt(renderer, 17, 9));
}

void checkDrawing() {
	// a software renderer drawing to an in-memory surface needs no window and no SDL_Init
	SDL_Surface* surface = SDL_CreateRGBSurface(0, DRAWN_WIDTH, DRAWN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (surface == NULL) {
		cerr << "FAIL: SDL_CreateRGBSurface: " << SDL_GetError() << endl;
		failures++;
		return;
	}
	SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
	if (renderer == NULL) {
		cerr << "FAIL: SDL_CreateSoftwareRenderer: " << SDL_GetError() << endl;
		failures++;
		SDL_FreeSurface(surface);
		return;
	}
	checkSlotDrawing(renderer);
	checkRendererSetAfterCreateGrid(renderer);
	checkDrawRectangleArguments(renderer);
	checkColliderDoesNotMoveDrawing(renderer);
	checkUncoveredScreenEdges(renderer);
	SDL_DestroyRenderer(renderer);
	SDL_FreeSurface(surface);
}

int main(int argc, char* args[]) {
	checkInit();
	checkSlotLayout();
	checkCreateGridReplacesSlots();
	checkSlotSetters();
	checkInitRejectsNonPositiveCounts();
	checkNonSquareSlotLayout();
	checkNonMultipleScreenSize();
	checkDrawing();
	if (failures > 0) {
		cerr << failures << " check(s) failed" << endl;
		return 1;
	}
	cout << "all checks passed" << endl;
	return 0;
}
