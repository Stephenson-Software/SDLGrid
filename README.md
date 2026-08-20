# SDLGrid

A Grid class whereby you can set the screen width, screen height and the # of rows and columns. Uses SDL.

## Requirements

- A C++ compiler (the sources are C++11-compatible)
- SDL2 and SDL2_image development headers

`src/GridClass.h` includes `<SDL.h>` and `<SDL_image.h>` directly, so the SDL2 include directory must be on the include path for any translation unit that uses the library.

## Usage

SDLGrid is not built as a library. The two files in `src/` are vendored directly into a consuming project:

- `src/GridClass.h`
- `src/GridClass.cpp`

A grid is set up once, then drawn each frame. The call order below is the one used by `test/testingGridClass.cpp`:

```cpp
#include "GridClass.h"

const int COLUMNS = 20;
const int ROWS = 20;
const int SCREEN_WIDTH = 500;  // must be a multiple of COLUMNS
const int SCREEN_HEIGHT = 500; // must be a multiple of ROWS

GridClass theGrid;

// after the SDL renderer has been created:
theGrid.init(COLUMNS, ROWS, SCREEN_WIDTH, SCREEN_HEIGHT);
theGrid.setRenderer(gRenderer);
theGrid.createGrid();

// each frame:
theGrid.drawGrid();
```

`setRenderer` must be called before `createGrid`, because `createGrid` copies the renderer into each slot as the slots are constructed.

Individual slots are reached by column and row, and a slot with no texture set is drawn as an outlined rectangle:

```cpp
theGrid.getGridSlot(column, row).setTexture(grassTexture);
theGrid.getGridSlot(column, row).setSolidFlag(true);
```

Slot dimensions are computed as `screenWidth / columns` and `screenHeight / rows` using integer division, which is why the screen dimensions should be exact multiples of the column and row counts — otherwise the grid does not reach the right and bottom edges of the window.

### API

`GridClass`:

| Member | Description |
| --- | --- |
| `init(int c, int r, int sW, int sH)` | Stores the column count, row count, screen width and screen height, and computes the slot dimensions |
| `setRenderer(SDL_Renderer* rendererToSet)` | Sets the renderer handed to each slot by `createGrid` |
| `createGrid()` | Constructs and positions `c * r` slots |
| `drawGrid()` | Renders every slot |
| `getGridSlot(int x, int y)` | Returns a reference to the slot at column `x`, row `y` |
| `getColumns()` / `getRows()` | Return the configured column and row counts |
| `solidSlots` / `buttonSlots` | Public vectors a consumer may populate to track slots of interest |

`GridSlot`:

| Member | Description |
| --- | --- |
| `init(int x, int y, int w, int h)` | Sets the slot's position, size and collider |
| `setRenderer(SDL_Renderer* rendererToSet)` | Sets the renderer used to draw the slot |
| `setTexture(SDL_Texture* textureToSet)` | Sets the texture drawn in place of the outline; ownership is not taken |
| `render()` | Draws the texture if one is set, otherwise draws a rectangle |
| `drawRectangle(int x, int y, int w, int h)` | Draws a rectangle at the given position and size, filled or outlined according to the slot's fill state |
| `fillBlack()` / `clear()` | Switch the rectangle between filled and outlined |
| `setSolidFlag(bool setter)` / `setButtonFlag(bool setter)` | Set the slot's solid and button flags |
| `collider` | Public `SDL_Rect` matching the slot's position and size, for collision checks |

## Demo programs

`test/` contains two standalone SDL programs:

- `testingGridClass.cpp` — draws a 20x20 grid of outlined slots in a 500x500 window
- `simpleLevel.cpp` — draws a textured level and moves a sprite that collides with slots marked solid

Both load their PNGs by relative path, so both must be run from inside `test/`:

```sh
cd test
g++ -std=c++11 -I../src ../src/GridClass.cpp testingGridClass.cpp $(sdl2-config --cflags --libs) -lSDL2_image -o testingGridClass
./testingGridClass
```

## License

SDLGrid is made available under the terms of the [MIT License](LICENSE). See [COPYRIGHT.md](COPYRIGHT.md).
