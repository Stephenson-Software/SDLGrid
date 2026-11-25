#include <SDL.h>
#include <SDL_image.h>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include "GridClass.h"
#include "GridView3D.h"

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

void checkUncheckedScreenSize();

void checkConstructedState();

void checkCreateGridKeepsTrackedSlots();

void clearToWhite(SDL_Renderer* renderer);

void redrawOnWhite(SDL_Renderer* renderer, GridClass& grid);

int redAt(SDL_Renderer* renderer, int x, int y);

void checkSlotDrawing(SDL_Renderer* renderer);

void checkRendererSetAfterCreateGrid(SDL_Renderer* renderer);

void checkDrawRectangleArguments(SDL_Renderer* renderer);

void checkColliderDoesNotMoveDrawing(SDL_Renderer* renderer);

void checkUncoveredScreenEdges(SDL_Renderer* renderer);

void checkTextureStretchedToSlot(SDL_Renderer* renderer);

void checkTrackedSlotsAreCopies(SDL_Renderer* renderer);

void checkUnconfiguredGridDrawsNothing(SDL_Renderer* renderer);

void checkDrawing();

void expectNear(const char* description, float expected, float actual);

void checkView3DLayout();

void checkView3DLooks(SDL_Renderer* renderer);

void checkView3DShapes(SDL_Renderer* renderer);

void checkView3DMarkers();

void checkView3DPicking();

void checkView3D();

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

void checkUncheckedScreenSize() {
	// init checks only the column and row counts, so a screen narrower than the column count
	// and a negative screen height are both stored; 3 / 4 truncates to a slot width of 0, and
	// -12 / 5 truncates toward zero to a slot height of -2 rather than rounding down to -3,
	// so the last of the 5 rows starts at y -8 and every slot starts at x 0
	GridClass undersizedGrid;
	undersizedGrid.init(4, 5, 3, -12);
	expectEqual("getColumns after init on a 3x-12 screen", 4, undersizedGrid.getColumns());
	expectEqual("getRows after init on a 3x-12 screen", 5, undersizedGrid.getRows());
	undersizedGrid.createGrid();
	expectEqual("collider.w of slot (0, 0) on a 3x-12 screen", 0, undersizedGrid.getGridSlot(0, 0).collider.w);
	expectEqual("collider.h of slot (0, 0) on a 3x-12 screen", -2, undersizedGrid.getGridSlot(0, 0).collider.h);
	expectEqual("collider.x of the last slot on a 3x-12 screen", 0, undersizedGrid.getGridSlot(3, 4).collider.x);
	expectEqual("collider.y of the last slot on a 3x-12 screen", -8, undersizedGrid.getGridSlot(3, 4).collider.y);
}

void checkConstructedState() {
	// a slot that has never been through init sits at (0, 0) with zero width and height, and a
	// grid that has never been through init has zero columns and zero rows
	GridSlot constructedSlot;
	expectEqual("collider.x of a constructed slot", 0, constructedSlot.collider.x);
	expectEqual("collider.y of a constructed slot", 0, constructedSlot.collider.y);
	expectEqual("collider.w of a constructed slot", 0, constructedSlot.collider.w);
	expectEqual("collider.h of a constructed slot", 0, constructedSlot.collider.h);

	GridClass constructedGrid;
	expectEqual("getColumns of a constructed grid", 0, constructedGrid.getColumns());
	expectEqual("getRows of a constructed grid", 0, constructedGrid.getRows());
}

void checkCreateGridKeepsTrackedSlots() {
	// the copies are taken from a 4x5 grid of 50x60 slots and the grid is then rebuilt as 2x2
	// with 100x150 slots, so a copy that createGrid cleared, or replaced with a rebuilt slot,
	// would change the sizes or the positions read back below
	GridClass rebuiltGrid;
	rebuiltGrid.init(4, 5, 200, 300);
	rebuiltGrid.createGrid();
	rebuiltGrid.solidSlots.push_back(rebuiltGrid.getGridSlot(1, 0));
	rebuiltGrid.buttonSlots.push_back(rebuiltGrid.getGridSlot(0, 1));
	rebuiltGrid.init(2, 2, 200, 300);
	rebuiltGrid.createGrid();
	expectEqual("solidSlots size after a second createGrid", 1, (int) rebuiltGrid.solidSlots.size());
	expectEqual("buttonSlots size after a second createGrid", 1, (int) rebuiltGrid.buttonSlots.size());
	expectEqual("collider.x of the solid slot copy after a second createGrid", 50, rebuiltGrid.solidSlots[0].collider.x);
	expectEqual("collider.w of the solid slot copy after a second createGrid", 50, rebuiltGrid.solidSlots[0].collider.w);
	expectEqual("collider.y of the button slot copy after a second createGrid", 60, rebuiltGrid.buttonSlots[0].collider.y);
	expectEqual("collider.h of the button slot copy after a second createGrid", 60, rebuiltGrid.buttonSlots[0].collider.h);
	expectEqual("collider.w of rebuilt slot (1, 0)", 100, rebuiltGrid.getGridSlot(1, 0).collider.w);
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

void checkTextureStretchedToSlot(SDL_Renderer* renderer) {
	// render copies the whole texture into the slot's rectangle, so a 1x1 texture on the second
	// slot covers both its top-left pixel (10, 0) and its bottom-right pixel (19, 9); a texture
	// copied at its own size would cover (10, 0) only, and one spilling left of the slot would
	// cover the first slot's right edge at (9, 5), which is drawn black by its outline
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
	if (texture == NULL) {
		cerr << "FAIL: SDL_CreateTexture: " << SDL_GetError() << endl;
		failures++;
		return;
	}
	Uint32 texturePixel = 0xFF404040;
	SDL_UpdateTexture(texture, NULL, &texturePixel, sizeof(texturePixel));

	GridClass texturedGrid;
	texturedGrid.init(DRAWN_COLUMNS, DRAWN_ROWS, DRAWN_WIDTH, DRAWN_HEIGHT);
	texturedGrid.setRenderer(renderer);
	texturedGrid.createGrid();
	texturedGrid.getGridSlot(1, 0).setTexture(texture);
	redrawOnWhite(renderer, texturedGrid);
	expectEqual("top-left pixel of a slot with a 1x1 texture", RED_OF_TEXTURE, redAt(renderer, 10, 0));
	expectEqual("bottom-right pixel of a slot with a 1x1 texture", RED_OF_TEXTURE, redAt(renderer, 19, 9));
	expectEqual("right edge of the outlined slot beside a textured slot", RED_OF_BLACK, redAt(renderer, 9, 5));

	SDL_DestroyTexture(texture);
}

void checkTrackedSlotsAreCopies(SDL_Renderer* renderer) {
	// solidSlots holds a copy of the slot, taken with the renderer, position and fill state the
	// slot had at the time, so filling the slot in the grid afterwards leaves the copy outlined,
	// filling the copy leaves the slot in the grid outlined, and drawGrid draws only the grid
	GridClass trackedGrid;
	trackedGrid.init(DRAWN_COLUMNS, DRAWN_ROWS, DRAWN_WIDTH, DRAWN_HEIGHT);
	trackedGrid.setRenderer(renderer);
	trackedGrid.createGrid();
	trackedGrid.solidSlots.push_back(trackedGrid.getGridSlot(0, 0));

	trackedGrid.getGridSlot(0, 0).fillBlack();
	clearToWhite(renderer);
	trackedGrid.solidSlots[0].render();
	expectEqual("top edge of a copied slot", RED_OF_BLACK, redAt(renderer, 5, 0));
	expectEqual("middle of a copied slot after the original is filled", RED_OF_WHITE, redAt(renderer, 5, 5));

	trackedGrid.getGridSlot(0, 0).clear();
	trackedGrid.solidSlots[0].fillBlack();
	clearToWhite(renderer);
	trackedGrid.solidSlots[0].render();
	expectEqual("middle of a filled copy of a slot", RED_OF_BLACK, redAt(renderer, 5, 5));

	redrawOnWhite(renderer, trackedGrid);
	expectEqual("middle of a slot whose copy is filled", RED_OF_WHITE, redAt(renderer, 5, 5));
}

void checkUnconfiguredGridDrawsNothing(SDL_Renderer* renderer) {
	// a grid whose only init was rejected still has zero columns and zero rows, so createGrid
	// builds no slots and drawGrid neither draws a pixel nor changes the white draw colour that
	// redrawOnWhite left behind; drawing any untextured slot would set it to black
	GridClass unconfiguredGrid;
	unconfiguredGrid.init(0, 0, DRAWN_WIDTH, DRAWN_HEIGHT);
	unconfiguredGrid.setRenderer(renderer);
	unconfiguredGrid.createGrid();
	redrawOnWhite(renderer, unconfiguredGrid);
	expectEqual("top-left pixel under a grid with no slots", RED_OF_WHITE, redAt(renderer, 0, 0));
	expectEqual("top edge of where a first slot would be", RED_OF_WHITE, redAt(renderer, 5, 0));

	Uint8 red = 0x00;
	Uint8 green = 0x00;
	Uint8 blue = 0x00;
	Uint8 alpha = 0x00;
	SDL_GetRenderDrawColor(renderer, &red, &green, &blue, &alpha);
	expectEqual("red of the draw colour after drawing a grid with no slots", RED_OF_WHITE, red);
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
	checkTextureStretchedToSlot(renderer);
	checkTrackedSlotsAreCopies(renderer);
	checkUnconfiguredGridDrawsNothing(renderer);
	SDL_DestroyRenderer(renderer);
	SDL_FreeSurface(surface);
}

// ------------------------------------------------------------------- the 3D view (src/GridView3D.h)

void expectNear(const char* description, float expected, float actual) {
	if (std::fabs(expected - actual) > 1e-4f) {
		cerr << "FAIL: " << description << " (expected " << expected << ", got " << actual << ")" << endl;
		failures++;
	}
}

void checkView3DLayout() {
	// a 20x20 grid of 25 pixel slots is 20 units wide and deep, centred on the origin, with
	// slot (0, 0) at the back left as it is at the top left in 2D
	GridClass grid;
	grid.init(COLUMNS, ROWS, SCREEN_WIDTH, SCREEN_HEIGHT);
	expectEqual("slot count before createGrid", 0, grid.getSlotCount());
	grid.createGrid();
	expectEqual("slot count after createGrid", COLUMNS * ROWS, grid.getSlotCount());
	expectEqual("getSlotWidth", SLOT_WIDTH, grid.getSlotWidth());
	expectEqual("getSlotHeight", SLOT_HEIGHT, grid.getSlotHeight());
	SDL_Rect rect = grid.getGridSlot(3, 2).getRect();
	expectEqual("getRect x of slot (3, 2)", 3 * SLOT_WIDTH, rect.x);
	expectEqual("getRect y of slot (3, 2)", 2 * SLOT_HEIGHT, rect.y);
	expectEqual("getRect w of slot (3, 2)", SLOT_WIDTH, rect.w);
	expectEqual("a new slot has no texture", 1, grid.getGridSlot(3, 2).getTexture() == NULL ? 1 : 0);
	expectEqual("a new slot is not filled", 0, grid.getGridSlot(3, 2).isFilled() ? 1 : 0);
	grid.getGridSlot(3, 2).fillBlack();
	expectEqual("fillBlack fills the slot", 1, grid.getGridSlot(3, 2).isFilled() ? 1 : 0);
	gridview3d::Layout l = gridview3d::layoutOf(grid);
	expectEqual("3D layout columns", COLUMNS, l.columns);
	expectEqual("3D layout slot width", SLOT_WIDTH, l.slotWidth);
	float x, z;
	gridview3d::slotCenter(l, 0, 0, x, z);
	expectNear("3D x of slot (0, 0)", -9.5f, x);
	expectNear("3D z of slot (0, 0)", -9.5f, z);
	gridview3d::slotCenter(l, 19, 19, x, z);
	expectNear("3D x of slot (19, 19)", 9.5f, x);
	expectNear("3D z of slot (19, 19)", 9.5f, z);

	// slots twice as wide as they are tall stay twice as wide in 3D
	GridClass wide;
	wide.init(4, 2, 200, 50);
	l = gridview3d::layoutOf(wide);
	expectNear("3D unit width of a 50x25 slot", 2.0f, gridview3d::unitWidth(l));
	expectNear("3D width of four 50x25 slots", 8.0f, gridview3d::worldWidth(l));
	gridview3d::slotCenter(l, 3, 1, x, z);
	expectNear("3D x of the last wide slot", 3.0f, x);
	expectNear("3D z of the last wide slot", 0.5f, z);

	// the middle of each slot in 2D is where a 3D click on it lands, inside its collider
	int cx, cy;
	gridview3d::clickPoint(gridview3d::layoutOf(grid), 3, 2, cx, cy);
	expectEqual("2D x of a 3D click on slot (3, 2)", 3 * SLOT_WIDTH + SLOT_WIDTH / 2, cx);
	expectEqual("2D y of a 3D click on slot (3, 2)", 2 * SLOT_HEIGHT + SLOT_HEIGHT / 2, cy);
	SDL_Point point = {cx, cy};
	expectEqual("a 3D click lands inside the slot's collider", 1, SDL_PointInRect(&point, &grid.getGridSlot(3, 2).collider) ? 1 : 0);
}

void checkView3DLooks(SDL_Renderer* renderer) {
	// lookFromSurface averages a surface's opaque pixels and skips transparent ones
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 3, 1, 32, SDL_PIXELFORMAT_RGBA32);
	Uint8* p = static_cast<Uint8*>(surface->pixels);
	const Uint8 pixels[12] = {255, 0, 0, 255, 0, 0, 255, 255, 0, 255, 0, 0};
	for (int i = 0; i < 12; i++) {
		p[i] = pixels[i];
	}
	gridview3d::Look look = gridview3d::lookFromSurface(surface, 0.7f);
	expectNear("average red of red and blue (green transparent)", 0.5f, look.color[0]);
	expectNear("average green of red and blue (green transparent)", 0.0f, look.color[1]);
	expectNear("average blue of red and blue (green transparent)", 0.5f, look.color[2]);
	expectNear("a surface's Look keeps its height", 0.7f, look.height);
	SDL_FreeSurface(surface);

	// a texture with no Look gets the palette's colours in order, the same one every time
	SDL_Texture* a = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
	SDL_Texture* b = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
	gridview3d::Looks looks;
	const gridview3d::Look first = looks.get(a);
	const gridview3d::Look second = looks.get(b);
	expectEqual("two textures with no Look look different", 1, first.color[0] != second.color[0] || first.color[1] != second.color[1] || first.color[2] != second.color[2] ? 1 : 0);
	expectNear("a texture with no Look keeps its colour", first.color[0], looks.get(a).color[0]);
	looks.set(a, gridview3d::look(0.1f, 0.2f, 0.3f, 0.9f));
	expectNear("a texture's Look can be set after it was first seen", 0.9f, looks.get(a).height);
	expectEqual("each texture is listed once", 2, int(looks.size()));
	SDL_DestroyTexture(a);
	SDL_DestroyTexture(b);
}

void checkView3DShapes(SDL_Renderer* renderer) {
	gridview3d::Looks looks;
	GridClass grid;
	grid.init(DRAWN_COLUMNS, DRAWN_ROWS, DRAWN_WIDTH, DRAWN_HEIGHT);
	grid.setRenderer(renderer);
	// before createGrid there are no slots: only the bed is drawn
	expectEqual("3D shapes before createGrid (the bed)", 1, int(gridview3d::gridShapes(grid, looks).size()));
	expectEqual("3D tile heights before createGrid", 0, int(gridview3d::tileHeights(grid, looks).size()));
	grid.createGrid();

	// an outlined slot is a white tile on a black edge: two shapes each, after the bed
	std::vector<gridview3d::Shape> shapes = gridview3d::gridShapes(grid, looks);
	expectEqual("3D shapes of two outlined slots", 5, int(shapes.size()));
	expectNear("the outlined slot's top is white", gridview3d::EMPTY_COLOR[0], shapes[2].color[0]);
	expectNear("the outlined slot's edge is black", gridview3d::OUTLINE_COLOR[0], shapes[1].color[0]);

	// fillBlack raises a black block, and clear lays it flat again
	grid.getGridSlot(0, 0).fillBlack();
	shapes = gridview3d::gridShapes(grid, looks);
	expectEqual("3D shapes of a filled and an outlined slot", 4, int(shapes.size()));
	expectNear("a filled slot stands FILLED_HEIGHT tall", gridview3d::FILLED_HEIGHT, shapes[1].scale[1]);
	expectNear("a filled slot is black", gridview3d::OUTLINE_COLOR[0], shapes[1].color[0]);
	grid.getGridSlot(0, 0).clear();
	expectEqual("3D shapes after clear", 5, int(gridview3d::gridShapes(grid, looks).size()));

	// a textured slot is one tile in its texture's Look, standing on the bed, whatever its fill
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
	looks.set(texture, gridview3d::look(0.25f, 0.5f, 0.75f, 0.6f));
	grid.getGridSlot(1, 0).setTexture(texture);
	grid.getGridSlot(1, 0).fillBlack();
	shapes = gridview3d::gridShapes(grid, looks);
	expectEqual("3D shapes of an outlined and a textured slot", 4, int(shapes.size()));
	const gridview3d::Shape& tile = shapes[3];
	expectNear("a textured tile's colour is its Look's", 0.5f, tile.color[1]);
	expectNear("a textured tile's height is its Look's", 0.6f, tile.scale[1]);
	expectNear("a textured tile stands on the bed", 0.0f, tile.position[1] - tile.scale[1] / 2);
	float x, z;
	gridview3d::slotCenter(gridview3d::layoutOf(grid), 1, 0, x, z);
	expectNear("a textured tile is over its slot", x, tile.position[0]);
	std::vector<float> heights = gridview3d::tileHeights(grid, looks);
	expectEqual("one tile height per slot", 2, int(heights.size()));
	expectNear("the textured slot's tile height", 0.6f, heights[1]);
	expectNear("the outlined slot's tile height", gridview3d::EMPTY_HEIGHT, heights[0]);

	// a Look flatter than MIN_HEIGHT is drawn MIN_HEIGHT tall, so it never vanishes
	looks.set(texture, gridview3d::look(0.25f, 0.5f, 0.75f, 0.0f));
	expectNear("a flat Look is drawn MIN_HEIGHT tall", gridview3d::MIN_HEIGHT, gridview3d::gridShapes(grid, looks)[3].scale[1]);

	// setTexture(NULL) brings back the outline, as it does in 2D
	grid.getGridSlot(1, 0).setTexture(NULL);
	grid.getGridSlot(1, 0).clear();
	expectEqual("3D shapes after the texture is removed", 5, int(gridview3d::gridShapes(grid, looks).size()));
	SDL_DestroyTexture(texture);
}

void checkView3DMarkers() {
	// simpleLevel's 25x50 sprite resting on the ground row (row 15) of its 20x20 grid of
	// 25 pixel slots covers one slot across and two down: a block one unit wide, two deep
	GridClass grid;
	grid.init(COLUMNS, ROWS, SCREEN_WIDTH, SCREEN_HEIGHT);
	grid.createGrid();
	gridview3d::Marker sprite = {{250, 325, 25, 50}, {0.0f, 0.0f, 0.0f}, 1.5f};
	std::vector<gridview3d::Shape> shapes = gridview3d::markerShapes(gridview3d::layoutOf(grid), std::vector<gridview3d::Marker>(1, sprite));
	expectEqual("one block per marker", 1, int(shapes.size()));
	expectNear("the marker's x is the middle of its rectangle", 0.5f, shapes[0].position[0]);
	expectNear("the marker's z is the middle of its rectangle", 4.0f, shapes[0].position[2]);
	expectNear("the marker is as wide as its rectangle", 1.0f, shapes[0].scale[0]);
	expectNear("the marker is as deep as its rectangle is tall", 2.0f, shapes[0].scale[2]);
	expectNear("the marker stands its height tall", 1.5f, shapes[0].scale[1]);
}

void checkView3DPicking() {
	// at every view shape (the demos' window, a phone held upright and sideways), a click
	// on the middle or near a corner of any slot's tile top picks that slot; where tiles of
	// different heights stand side by side, a click on the middle of any tile top still
	// does (near a corner, a taller tile in front may hide the point, and is picked instead)
	const int sizes[3][2] = {{500, 500}, {390, 844}, {844, 390}};
	GridClass grid;
	grid.init(6, 4, 300, 200);
	grid.createGrid();
	const gridview3d::Layout l = gridview3d::layoutOf(grid);
	int missed = 0;
	for (int pass = 0; pass < 6; pass++) {
		const int s = pass % 3;
		const bool mixed = pass >= 3;
		std::vector<float> heights;
		for (int i = 0; i < 24; i++) {
			heights.push_back(!mixed ? 0.3f : i % 3 == 0 ? 0.75f : i % 3 == 1 ? 0.04f : 0.3f);
		}
		const float w = float(sizes[s][0]), h = float(sizes[s][1]);
		const gridview3d::Camera camera = gridview3d::fitCamera(l, w / h);
		for (int r = 0; r < l.rows; r++) {
			for (int c = 0; c < l.columns; c++) {
				float x, z;
				gridview3d::slotCenter(l, c, r, x, z);
				const float offsets[3][2] = {{0.0f, 0.0f}, {-0.4f, -0.4f}, {0.4f, 0.4f}};
				for (int o = 0; o < (mixed ? 1 : 3); o++) {
					float sx, sy;
					int column = -1, row = -1;
					const bool seen = gridview3d::projectToScreen(camera, x + offsets[o][0], heights[r * l.columns + c], z + offsets[o][1], w, h, sx, sy);
					const bool onView = seen && sx > 0 && sx < w && sy > 0 && sy < h;
					if (!onView || !gridview3d::pickSlot(camera, l, heights, sx, sy, w, h, column, row) || column != c || row != r) {
						missed++;
					}
				}
			}
		}
		// a corner of the view is off the grid
		int column, row;
		expectEqual("a click in the view's top-left corner picks no slot", 0, gridview3d::pickSlot(camera, l, heights, 1.0f, 1.0f, w, h, column, row) ? 1 : 0);
	}
	expectEqual("clicks on tile tops that picked another slot, or were off the view", 0, missed);
	int column, row;
	expectEqual("a click on an empty view picks no slot", 0, gridview3d::pickSlot(gridview3d::fitCamera(l, 1.0f), l, std::vector<float>(24, 0.3f), 5.0f, 5.0f, 0.0f, 0.0f, column, row) ? 1 : 0);
}

void checkView3D() {
	SDL_Surface* surface = SDL_CreateRGBSurface(0, DRAWN_WIDTH, DRAWN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	SDL_Renderer* renderer = surface == NULL ? NULL : SDL_CreateSoftwareRenderer(surface);
	if (renderer == NULL) {
		cerr << "FAIL: a software renderer for the 3D view checks: " << SDL_GetError() << endl;
		failures++;
		SDL_FreeSurface(surface);
		return;
	}
	checkView3DLayout();
	checkView3DLooks(renderer);
	checkView3DShapes(renderer);
	checkView3DMarkers();
	checkView3DPicking();
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
	checkUncheckedScreenSize();
	checkConstructedState();
	checkCreateGridKeepsTrackedSlots();
	checkDrawing();
	checkView3D();
	if (failures > 0) {
		cerr << failures << " check(s) failed" << endl;
		return 1;
	}
	cout << "all checks passed" << endl;
	return 0;
}
