// GridView3D.h: what a GridClass looks like in 3D, as plain data.
//
// Optional. GridClass keeps drawing in 2D exactly as before; a program that
// wants its grid in 3D as well includes src/GridView3DStrata.h, which draws
// this scene with Strata (https://github.com/Preponderous-Software/strata)
// and falls back to the 2D window where Strata can't run. This header knows
// nothing of Strata or a GPU, so test/checkGridClass.cpp checks it natively.
//
//   - every slot is a tile on a dark bed, laid out as the 2D grid is (column
//     0 on the left, row 0 at the back, as row 0 is at the top in 2D), with
//     the slot's width and height kept in proportion;
//   - a textured slot's tile takes its colour and height from its texture's
//     Look: set one per texture (lookFromSurface averages a texture's
//     pixels), and a texture with none gets the next colour of a fixed
//     palette, so different textures always look different;
//   - a slot with no texture is drawn as the 2D outline is: a white tile on a
//     black edge, or, after fillBlack, a raised black block;
//   - a Marker (a sprite, a cursor: anything a program draws over the grid)
//     is a block standing over the screen rectangle it covers in 2D;
//   - a click or tap is turned into the slot under it by casting a ray from
//     the camera through the pointer onto the tiles' tops (pickSlot), and
//     then into a click at the middle of that slot in the 2D window
//     (clickPoint), so a program's own 2D click handling decides what it
//     means.
//
// C++11, like GridClass.
#ifndef GRIDVIEW3D_H
#define GRIDVIEW3D_H

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "GridClass.h"

namespace gridview3d {

// Strata's primitive kinds (strata.h, STRATA_PRIMITIVE_*).
const int CUBE = 0;

struct Shape {
	int kind;
	float position[3];
	float scale[3];
	float color[4];
};

struct Camera {
	float eye[3];
	float target[3];
	float fovDegrees;
};

// How a texture's slots look in 3D: a colour (linear 0-1 RGB) and how tall
// the tile stands, in slot heights.
struct Look {
	float color[3];
	float height;
};

// Something drawn over the grid in 2D, as a block in 3D: the screen
// rectangle it covers, its colour and how tall it stands.
struct Marker {
	SDL_Rect rect;
	float color[3];
	float height;
};

// graphik grid3d's lighting and projection, as Strata's other grid views.
const float CLEAR_COLOR[4] = {0.10f, 0.12f, 0.16f, 1.0f};
const float SUN_DIRECTION[3] = {-0.45f, 0.85f, 0.35f};
const float NEAR_PLANE = 0.3f;
const float FAR_PLANE = 300.0f;
const float FOV_DEGREES = 42.0f;
const float SHADOW_DISTANCE = 40.0f;
const double PI = 3.14159265358979323846;

const float TILE_GAP = 0.06f;           // between tiles, in slot heights
const float MIN_HEIGHT = 0.04f;         // the flattest a tile is drawn
const float BED_DEPTH = 0.12f;          // the dark bed under the tiles
const float BED_MARGIN = 0.25f;
const float BED_COLOR[3] = {0.10f, 0.10f, 0.12f};
const float OUTLINE_COLOR[3] = {0.02f, 0.02f, 0.02f};  // the 2D outline's black
const float EMPTY_COLOR[3] = {0.95f, 0.95f, 0.93f};    // the 2D window's white
const float EMPTY_HEIGHT = 0.08f;
const float OUTLINE_HEIGHT = 0.04f;
const float OUTLINE_INSET = 0.10f;      // how much of the tile the outline shows
const float FILLED_HEIGHT = 0.35f;
const float DEFAULT_HEIGHT = 0.3f;      // a texture with no Look of its own

// Colours for textures with no Look, in the order they are first seen.
const float PALETTE[8][3] = {
	{0.36f, 0.62f, 0.86f}, {0.40f, 0.70f, 0.32f}, {0.55f, 0.38f, 0.22f}, {0.86f, 0.70f, 0.30f},
	{0.70f, 0.36f, 0.70f}, {0.85f, 0.40f, 0.30f}, {0.40f, 0.70f, 0.70f}, {0.60f, 0.60f, 0.62f},
};

inline Look look(float r, float g, float b, float height) {
	Look l;
	l.color[0] = r;
	l.color[1] = g;
	l.color[2] = b;
	l.height = height;
	return l;
}

// The average colour of a surface's pixels (alpha-weighted), as a Look of
// the given height: what a texture made from that surface looks like from
// afar. Call it with the surface a texture is made from, before freeing it.
inline Look lookFromSurface(SDL_Surface* surface, float height) {
	double sum[3] = {0, 0, 0}, weight = 0;
	if (surface != NULL && surface->w > 0 && surface->h > 0) {
		SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
		if (rgba != NULL) {
			if (SDL_LockSurface(rgba) == 0) {
				for (int y = 0; y < rgba->h; y++) {
					const Uint8* row = static_cast<const Uint8*>(rgba->pixels) + y * rgba->pitch;
					for (int x = 0; x < rgba->w; x++) {
						const double a = row[4 * x + 3] / 255.0;
						for (int k = 0; k < 3; k++) {
							sum[k] += a * (row[4 * x + k] / 255.0);
						}
						weight += a;
					}
				}
				SDL_UnlockSurface(rgba);
			}
			SDL_FreeSurface(rgba);
		}
	}
	if (weight <= 0) {
		return look(PALETTE[7][0], PALETTE[7][1], PALETTE[7][2], height);
	}
	return look(float(sum[0] / weight), float(sum[1] / weight), float(sum[2] / weight), height);
}

// Each texture's Look: set by the program, or the palette's next colour.
class Looks {
  public:
	void set(SDL_Texture* texture, const Look& l) {
		for (size_t i = 0; i < entries.size(); i++) {
			if (entries[i].first == texture) {
				entries[i].second = l;
				return;
			}
		}
		entries.push_back(std::make_pair(texture, l));
	}
	Look get(SDL_Texture* texture) {
		for (size_t i = 0; i < entries.size(); i++) {
			if (entries[i].first == texture) {
				return entries[i].second;
			}
		}
		const float* c = PALETTE[unnamed % 8];
		unnamed++;
		const Look l = look(c[0], c[1], c[2], DEFAULT_HEIGHT);
		entries.push_back(std::make_pair(texture, l));
		return l;
	}
	size_t size() const { return entries.size(); }

  private:
	std::vector<std::pair<SDL_Texture*, Look> > entries;
	int unnamed = 0;
};

// The grid's shape: slots across and down, and a slot's size in pixels.
struct Layout {
	int columns, rows, slotWidth, slotHeight;
};

inline Layout layoutOf(GridClass& grid) {
	Layout l = {grid.getColumns(), grid.getRows(), grid.getSlotWidth(), grid.getSlotHeight()};
	return l;
}

// A slot is one unit deep (z); its width (x) keeps the 2D slot's proportion.
inline float unitWidth(const Layout& l) {
	return l.slotHeight > 0 && l.slotWidth > 0 ? float(l.slotWidth) / float(l.slotHeight) : 1.0f;
}

inline float worldWidth(const Layout& l) { return l.columns * unitWidth(l); }
inline float worldDepth(const Layout& l) { return float(l.rows); }

// World (x, z) of a screen point of the 2D window.
inline void screenToWorld(const Layout& l, float sx, float sy, float& x, float& z) {
	const float sw = l.slotWidth > 0 ? float(l.slotWidth) : 1.0f;
	const float sh = l.slotHeight > 0 ? float(l.slotHeight) : 1.0f;
	x = sx / sw * unitWidth(l) - worldWidth(l) / 2;
	z = sy / sh - worldDepth(l) / 2;
}

// World (x, z) of the middle of the slot at column c, row r.
inline void slotCenter(const Layout& l, int c, int r, float& x, float& z) {
	x = (c + 0.5f) * unitWidth(l) - worldWidth(l) / 2;
	z = (r + 0.5f) - worldDepth(l) / 2;
}

// South of the grid, looking down about 55 degrees, far enough back that the
// whole grid fits a width x height view of the given aspect (a tall phone
// backs it off).
inline Camera fitCamera(const Layout& l, float aspect) {
	const float a = std::max(aspect, 0.45f);
	const float size = 1.12f * std::max(worldDepth(l), worldWidth(l) / a) + 1.0f;
	const float distance = 0.86f * size + 0.9f;
	Camera c = {{0.0f, 1.35f * distance, 0.95f * distance}, {0.0f, -0.3f, 0.05f * size}, FOV_DEGREES};
	return c;
}

inline Shape box(float x, float y, float z, float sx, float sy, float sz, const float rgb[3]) {
	Shape s = {CUBE, {x, y, z}, {sx, sy, sz}, {rgb[0], rgb[1], rgb[2], 1.0f}};
	return s;
}

// How tall the tile of each slot stands, in slot order (row by row).
inline std::vector<float> tileHeights(GridClass& grid, Looks& looks) {
	std::vector<float> out;
	const Layout l = layoutOf(grid);
	if (grid.getSlotCount() < l.columns * l.rows) {
		return out;
	}
	for (int r = 0; r < l.rows; r++) {
		for (int c = 0; c < l.columns; c++) {
			const GridSlot& slot = grid.getGridSlot(c, r);
			float h;
			if (slot.getTexture() != NULL) {
				h = std::max(looks.get(slot.getTexture()).height, MIN_HEIGHT);
			} else {
				h = slot.isFilled() ? FILLED_HEIGHT : EMPTY_HEIGHT;
			}
			out.push_back(h);
		}
	}
	return out;
}

// The bed and every slot's tile. Nothing but the bed before createGrid.
inline std::vector<Shape> gridShapes(GridClass& grid, Looks& looks) {
	std::vector<Shape> out;
	const Layout l = layoutOf(grid);
	const float w = worldWidth(l), d = worldDepth(l), uw = unitWidth(l);
	out.push_back(box(0.0f, -BED_DEPTH / 2, 0.0f, w + 2 * BED_MARGIN, BED_DEPTH, d + 2 * BED_MARGIN, BED_COLOR));
	if (grid.getSlotCount() < l.columns * l.rows) {
		return out;
	}
	for (int r = 0; r < l.rows; r++) {
		for (int c = 0; c < l.columns; c++) {
			const GridSlot& slot = grid.getGridSlot(c, r);
			float x, z;
			slotCenter(l, c, r, x, z);
			if (slot.getTexture() != NULL) {
				const Look k = looks.get(slot.getTexture());
				const float h = std::max(k.height, MIN_HEIGHT);
				out.push_back(box(x, h / 2, z, uw - TILE_GAP, h, 1.0f - TILE_GAP, k.color));
			} else if (slot.isFilled()) {
				out.push_back(box(x, FILLED_HEIGHT / 2, z, uw - TILE_GAP, FILLED_HEIGHT, 1.0f - TILE_GAP, OUTLINE_COLOR));
			} else {
				// the outline: a black edge round a white tile
				out.push_back(box(x, OUTLINE_HEIGHT / 2, z, uw - TILE_GAP / 2, OUTLINE_HEIGHT, 1.0f - TILE_GAP / 2,
						OUTLINE_COLOR));
				out.push_back(box(x, EMPTY_HEIGHT / 2, z, uw - TILE_GAP / 2 - OUTLINE_INSET, EMPTY_HEIGHT,
						1.0f - TILE_GAP / 2 - OUTLINE_INSET, EMPTY_COLOR));
			}
		}
	}
	return out;
}

// Each marker as a block standing over the rectangle it covers in 2D.
inline std::vector<Shape> markerShapes(const Layout& l, const std::vector<Marker>& markers) {
	std::vector<Shape> out;
	for (size_t i = 0; i < markers.size(); i++) {
		const Marker& m = markers[i];
		float x0, z0, x1, z1;
		screenToWorld(l, float(m.rect.x), float(m.rect.y), x0, z0);
		screenToWorld(l, float(m.rect.x + m.rect.w), float(m.rect.y + m.rect.h), x1, z1);
		const float h = std::max(m.height, MIN_HEIGHT);
		out.push_back(box((x0 + x1) / 2, h / 2, (z0 + z1) / 2, std::fabs(x1 - x0), h, std::fabs(z1 - z0), m.color));
	}
	return out;
}

// ----------------------------------------------------------------- picking

// The same right-handed look-at and OpenGL perspective Strata draws with (up
// is +y, the field of view vertical).
struct Vec {
	double x, y, z;
};
inline double dot(Vec a, Vec b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec cross(Vec a, Vec b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline Vec unit(Vec a) {
	const double n = std::sqrt(dot(a, a));
	return {a.x / n, a.y / n, a.z / n};
}

inline void basis(const Camera& c, Vec& forward, Vec& right, Vec& up) {
	forward = unit({double(c.target[0]) - c.eye[0], double(c.target[1]) - c.eye[1], double(c.target[2]) - c.eye[2]});
	right = unit(cross(forward, {0.0, 1.0, 0.0}));
	up = cross(right, forward);
}

// Where world point (x, y, z) lands in a width x height view, in pixels from
// the top-left; false behind the camera.
inline bool projectToScreen(const Camera& c, float x, float y, float z, float width, float height, float& sx,
		float& sy) {
	Vec f, r, u;
	basis(c, f, r, u);
	const Vec o = {double(x) - c.eye[0], double(y) - c.eye[1], double(z) - c.eye[2]};
	const double depth = dot(o, f);
	if (depth <= 0) {
		return false;
	}
	const double focal = 1.0 / std::tan(c.fovDegrees * PI / 360.0);
	const double aspect = double(width) / height;
	sx = float(((focal / aspect) * dot(o, r) / depth + 1.0) / 2.0 * width);
	sy = float((1.0 - focal * dot(o, u) / depth) / 2.0 * height);
	return true;
}

// The slot whose tile top is under view pixel (x, y): the nearest top the
// ray from the camera meets (the gap round a tile counts as the tile's), so
// a tall tile is picked where it is seen. False off the grid.
inline bool pickSlot(const Camera& c, const Layout& l, const std::vector<float>& heights, float x, float y,
		float width, float height, int& column, int& row) {
	if (!(width > 0 && height > 0) || l.columns <= 0 || l.rows <= 0) {
		return false;
	}
	Vec f, r, u;
	basis(c, f, r, u);
	const double focal = 1.0 / std::tan(c.fovDegrees * PI / 360.0);
	const double aspect = double(width) / height;
	const double nx = x / width * 2.0 - 1.0, ny = 1.0 - y / height * 2.0;
	const Vec d = {f.x + r.x * nx * aspect / focal + u.x * ny / focal, f.y + r.y * nx * aspect / focal + u.y * ny / focal,
			f.z + r.z * nx * aspect / focal + u.z * ny / focal};
	if (d.y >= 0) {
		return false;
	}
	const double uw = unitWidth(l);
	double best = -1;
	const size_t n = size_t(l.columns) * size_t(l.rows);
	for (size_t i = 0; i < n; i++) {
		const double top = i < heights.size() ? heights[i] : 0.0;
		const double t = (top - c.eye[1]) / d.y;
		if (t <= 0 || (best >= 0 && t >= best)) {
			continue;
		}
		const double px = c.eye[0] + d.x * t + worldWidth(l) / 2, pz = c.eye[2] + d.z * t + worldDepth(l) / 2;
		const int col = int(std::floor(px / uw)), rw = int(std::floor(pz));
		if (col == int(i % size_t(l.columns)) && rw == int(i / size_t(l.columns))) {
			best = t;
			column = col;
			row = rw;
		}
	}
	return best >= 0;
}

// The middle of a slot in the 2D window, where a 3D click on it lands.
inline void clickPoint(const Layout& l, int column, int row, int& x, int& y) {
	x = column * l.slotWidth + l.slotWidth / 2;
	y = row * l.slotHeight + l.slotHeight / 2;
}

}  // namespace gridview3d

#endif
