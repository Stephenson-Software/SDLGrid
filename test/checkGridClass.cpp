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

int main(int argc, char* args[]) {
	checkInit();
	checkSlotLayout();
	checkCreateGridReplacesSlots();
	checkSlotSetters();
	if (failures > 0) {
		cerr << failures << " check(s) failed" << endl;
		return 1;
	}
	cout << "all checks passed" << endl;
	return 0;
}
