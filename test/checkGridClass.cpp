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
	// fillBlack, clear, setSolidFlag and setButtonFlag expose no getter, so they are called
	// for their signatures only; the flagged slot is copied into the public tracking vectors
	// in the same way simpleLevel.cpp populates solidSlots
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

int main(int argc, char* args[]) {
	checkInit();
	checkSlotLayout();
	checkCreateGridReplacesSlots();
	checkSlotSetters();
	checkInitRejectsNonPositiveCounts();
	checkNonSquareSlotLayout();
	checkNonMultipleScreenSize();
	if (failures > 0) {
		cerr << failures << " check(s) failed" << endl;
		return 1;
	}
	cout << "all checks passed" << endl;
	return 0;
}
