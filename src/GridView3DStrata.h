// GridView3DStrata.h: a GridClass drawn in 3D by Strata
// (https://github.com/Preponderous-Software/strata), optional, with the 2D
// window as the fallback.
//
// A program keeps its GridClass, its 2D drawing and its event loop, and adds
// five calls:
//
//   gridview3d::open(w, h, title)   load Strata and open its w x h window;
//                                   false (and gridview3d::reason()) when it
//                                   can't run here, and the program opens its
//                                   2D window as before
//   gridview3d::ownsWindow()        whether the 3D window is open: SDL's video
//                                   subsystem must then stay off (it would take
//                                   the GL context the 3D window draws with), so
//                                   the 2D grid is drawn into memory instead,
//                                   with a software renderer on a surface
//   gridview3d::show(grid, looks, markers)   draw one frame (src/GridView3D.h
//                                   says what it shows)
//   gridview3d::pollEvent(&e)       the player's input in the 3D window, as the
//                                   SDL events the 2D window would have
//                                   produced: a key is the same key, closing
//                                   the window is SDL_QUIT, and a click or tap
//                                   on a slot is a click at the middle of that
//                                   slot in the 2D window
//   gridview3d::close()
//
// libstrata (libstrata.so.0.9 on Linux, strata.dll on Windows) is loaded at
// run time from STRATA_LIBRARY, from beside the executable or from the
// library path; tools/fetch-strata.sh fetches it from Strata's release.
// Nothing links against it, so a program built with this header runs
// unchanged, in 2D, where it is missing. Linux builds link -ldl.
//
// C++11, like GridClass.
#ifndef GRIDVIEW3D_STRATA_H
#define GRIDVIEW3D_STRATA_H

#include <SDL.h>
#include <cstdlib>
#include <string>
#include <vector>

#include "GridView3D.h"

#define STRATA_STATIC  // declarations only: every entry point is looked up at run time
#include "../third_party/strata/strata.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

namespace gridview3d {

struct StrataApi {
	decltype(&strata_abi_version) abiVersion;
	decltype(&strata_abi_minor) abiMinor;
	decltype(&strata_last_error) lastError;
	decltype(&strata_create) create;
	decltype(&strata_destroy) destroy;
	decltype(&strata_poll_event) pollEvent;
	decltype(&strata_get_drawable_size) drawableSize;
	decltype(&strata_set_camera) setCamera;
	decltype(&strata_camera_look_at) lookAt;
	decltype(&strata_lighting_defaults) lightingDefaults;
	decltype(&strata_set_lighting) setLighting;
	decltype(&strata_set_sun_direction) setSunDirection;
	decltype(&strata_set_point_lights) setPointLights;
	decltype(&strata_begin_frame) beginFrame;
	decltype(&strata_submit_primitives) submitPrimitives;
	decltype(&strata_end_frame) endFrame;
	decltype(&strata_present) present;
	decltype(&strata_save_screenshot_png) saveScreenshotPng;
};

struct Desktop {
	StrataApi api;
	StrataEngine* engine = nullptr;
	int width = 0, height = 0;
	Layout layout = {0, 0, 0, 0};       // the grid last shown, for clicks
	std::vector<float> heights;         // its tiles' heights, for clicks
	std::string reason;
};

inline Desktop& desktop() {
	static Desktop d;
	return d;
}

inline std::string executableDirectory() {
#ifdef _WIN32
	char path[MAX_PATH];
	const DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
	std::string s(path, n);
	const size_t slash = s.find_last_of("\\/");
#else
	char path[4096];
	const ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
	std::string s(path, n > 0 ? size_t(n) : 0);
	const size_t slash = s.find_last_of('/');
#endif
	return slash == std::string::npos ? std::string() : s.substr(0, slash + 1);
}

// Loads libstrata and looks up every entry point; false (and the reason) when
// the library or one of them is missing, or its ABI is not one this reads.
inline bool loadStrata(StrataApi& api, std::string& reason) {
	std::vector<std::string> candidates;
	const char* own = std::getenv("STRATA_LIBRARY");
	if (own && *own) {
		candidates.push_back(own);
	}
#ifdef _WIN32
	candidates.push_back(executableDirectory() + "strata.dll");
	candidates.push_back("strata.dll");
	HMODULE lib = NULL;
	for (size_t i = 0; i < candidates.size() && lib == NULL; i++) {
		lib = LoadLibraryA(candidates[i].c_str());
	}
	auto find = [&](const char* name) { return reinterpret_cast<void*>(GetProcAddress(lib, name)); };
#else
	candidates.push_back(executableDirectory() + "libstrata.so.0.9");
	candidates.push_back("libstrata.so.0.9");
	candidates.push_back("libstrata.so");
	void* lib = nullptr;
	for (size_t i = 0; i < candidates.size() && lib == nullptr; i++) {
		lib = dlopen(candidates[i].c_str(), RTLD_NOW | RTLD_LOCAL);
	}
	auto find = [&](const char* name) { return dlsym(lib, name); };
#endif
	if (!lib) {
		reason = "libstrata was not found";
		return false;
	}
	bool ok = true;
#define GRIDVIEW3D_FIND(field, name) \
	ok = ok && (*reinterpret_cast<void**>(&api.field) = find(name)) != nullptr
	GRIDVIEW3D_FIND(abiVersion, "strata_abi_version");
	GRIDVIEW3D_FIND(abiMinor, "strata_abi_minor");
	GRIDVIEW3D_FIND(lastError, "strata_last_error");
	GRIDVIEW3D_FIND(create, "strata_create");
	GRIDVIEW3D_FIND(destroy, "strata_destroy");
	GRIDVIEW3D_FIND(pollEvent, "strata_poll_event");
	GRIDVIEW3D_FIND(drawableSize, "strata_get_drawable_size");
	GRIDVIEW3D_FIND(setCamera, "strata_set_camera");
	GRIDVIEW3D_FIND(lookAt, "strata_camera_look_at");
	GRIDVIEW3D_FIND(lightingDefaults, "strata_lighting_defaults");
	GRIDVIEW3D_FIND(setLighting, "strata_set_lighting");
	GRIDVIEW3D_FIND(setSunDirection, "strata_set_sun_direction");
	GRIDVIEW3D_FIND(setPointLights, "strata_set_point_lights");
	GRIDVIEW3D_FIND(beginFrame, "strata_begin_frame");
	GRIDVIEW3D_FIND(submitPrimitives, "strata_submit_primitives");
	GRIDVIEW3D_FIND(endFrame, "strata_end_frame");
	GRIDVIEW3D_FIND(present, "strata_present");
	GRIDVIEW3D_FIND(saveScreenshotPng, "strata_save_screenshot_png");
#undef GRIDVIEW3D_FIND
	if (!ok) {
		reason = "libstrata is missing an entry point";
		return false;
	}
	// The sun and its direction arrived in ABI 1.6.
	if (api.abiVersion() != STRATA_ABI_VERSION || api.abiMinor() < 6) {
		reason = "libstrata is too old or too new (ABI " + std::to_string(api.abiVersion()) + "." +
				std::to_string(api.abiMinor()) + ", needs 1.6 or a later 1.x)";
		return false;
	}
	return true;
}

// Opens the 3D view: a width x height window (or, headless, an offscreen
// target for still pictures). False, with the reason in reason(), when Strata
// can't run here.
inline bool open(int width, int height, const char* title, bool headless = false) {
	Desktop& d = desktop();
	if (d.engine) {
		return true;
	}
	if (!loadStrata(d.api, d.reason)) {
		return false;
	}
	StrataEngineDesc desc = {};
	desc.struct_size = sizeof(desc);
	desc.abi_version = STRATA_ABI_VERSION;
	desc.surface = headless ? STRATA_SURFACE_HEADLESS : STRATA_SURFACE_WINDOW;
	desc.width = width;
	desc.height = height;
	desc.flags = headless ? 0u : uint32_t(STRATA_FLAG_VSYNC);
	desc.title = headless ? nullptr : title;
	if (d.api.create(&desc, &d.engine) != STRATA_OK) {
		d.reason = std::string("Strata could not start: ") + d.api.lastError();
		d.engine = nullptr;
		return false;
	}
	int32_t w = width, h = height;
	if (d.api.drawableSize(d.engine, &w, &h) == STRATA_OK && w > 0 && h > 0) {
		d.width = w;
		d.height = h;
	} else {
		d.width = width;
		d.height = height;
	}
	// graphik grid3d's lighting: the sun, shadows out to 40 units, no fog.
	StrataLighting lighting;
	d.api.lightingDefaults(STRATA_LIGHTING_SUN, &lighting);
	lighting.shadow_distance = SHADOW_DISTANCE;
	lighting.fog = 0;
	d.api.setLighting(d.engine, &lighting);
	d.api.setSunDirection(d.engine, SUN_DIRECTION);
	d.api.setPointLights(d.engine, nullptr, 0);
	return true;
}

inline const std::string& reason() { return desktop().reason; }

inline bool active() { return desktop().engine != nullptr; }

inline bool ownsWindow() { return active(); }

inline Camera camera() {
	const Desktop& d = desktop();
	return fitCamera(d.layout, d.height > 0 ? d.width / float(d.height) : 1.0f);
}

// One frame: the grid's tiles and the markers over them.
inline void show(GridClass& grid, Looks& looks, const std::vector<Marker>& markers = std::vector<Marker>()) {
	Desktop& d = desktop();
	if (!d.engine) {
		return;
	}
	d.layout = layoutOf(grid);
	d.heights = tileHeights(grid, looks);
	const Camera c = camera();
	const float up[3] = {0.0f, 1.0f, 0.0f};
	StrataCamera sc;
	d.api.lookAt(c.eye, c.target, up, c.fovDegrees, d.width / float(d.height), NEAR_PLANE, FAR_PLANE, &sc);
	d.api.setCamera(d.engine, &sc);
	StrataFrameDesc frame = {};
	frame.clear_flags = STRATA_CLEAR_COLOR | STRATA_CLEAR_DEPTH;
	for (int i = 0; i < 4; i++) {
		frame.clear_color[i] = CLEAR_COLOR[i];
	}
	d.api.beginFrame(d.engine, &frame);
	std::vector<Shape> shapes = gridShapes(grid, looks);
	const std::vector<Shape> extra = markerShapes(d.layout, markers);
	shapes.insert(shapes.end(), extra.begin(), extra.end());
	std::vector<StrataRenderItem> items(shapes.size());
	for (size_t i = 0; i < shapes.size(); i++) {
		items[i] = StrataRenderItem();
		items[i].kind = shapes[i].kind;
		for (int k = 0; k < 3; k++) {
			items[i].position[k] = shapes[i].position[k];
			items[i].scale[k] = shapes[i].scale[k];
		}
		for (int k = 0; k < 4; k++) {
			items[i].color[k] = shapes[i].color[k];
		}
	}
	if (!items.empty()) {
		d.api.submitPrimitives(d.engine, items.data(), int32_t(items.size()));
	}
	d.api.endFrame(d.engine);
	d.api.present(d.engine);
}

// A click or tap at (x, y) in the 3D window, as the SDL mouse button event
// the 2D window would have produced at the middle of the slot under it; off
// the grid, a click outside the 2D window.
inline void clickEvent(int type, int button, float x, float y, SDL_Event* e) {
	const Desktop& d = desktop();
	int column = 0, row = 0, cx = -1, cy = -1;
	if (pickSlot(camera(), d.layout, d.heights, x, y, float(d.width), float(d.height), column, row)) {
		clickPoint(d.layout, column, row, cx, cy);
	}
	SDL_zerop(e);
	e->type = Uint32(type);
	e->button.button = Uint8(button);
	e->button.state = type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
	e->button.clicks = 1;
	e->button.x = cx;
	e->button.y = cy;
}

// A key (by USB HID usage id, which SDL2 scancodes are) as the SDL key event
// the 2D window would have produced. Behind the 3D window SDL has no video
// subsystem, and so no keyboard layout: letters and digits are read by where
// they are on a US keyboard.
inline void keyEvent(bool down, int scancode, int repeat, SDL_Event* e) {
	SDL_zerop(e);
	e->type = down ? SDL_KEYDOWN : SDL_KEYUP;
	e->key.state = down ? SDL_PRESSED : SDL_RELEASED;
	e->key.repeat = Uint8(repeat ? 1 : 0);
	e->key.keysym.scancode = SDL_Scancode(scancode);
	if (SDL_WasInit(SDL_INIT_VIDEO)) {
		e->key.keysym.sym = SDL_GetKeyFromScancode(SDL_Scancode(scancode));
	} else if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
		e->key.keysym.sym = SDL_Keycode('a' + (scancode - SDL_SCANCODE_A));
	} else if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
		e->key.keysym.sym = SDL_Keycode('1' + (scancode - SDL_SCANCODE_1));
	} else if (scancode == SDL_SCANCODE_0) {
		e->key.keysym.sym = SDLK_0;
	} else {
		e->key.keysym.sym = SDL_SCANCODE_TO_KEYCODE(scancode);
	}
}

// The next input from the 3D window, as an SDL event; 0 when there is none.
inline int pollEvent(SDL_Event* e) {
	Desktop& d = desktop();
	if (!d.engine) {
		return 0;
	}
	StrataEvent ev;
	while (d.api.pollEvent(d.engine, &ev)) {
		switch (ev.type) {
			case STRATA_EVENT_QUIT:
				SDL_zerop(e);
				e->type = SDL_QUIT;
				return 1;
			case STRATA_EVENT_KEY_DOWN:
			case STRATA_EVENT_KEY_UP:
				keyEvent(ev.type == STRATA_EVENT_KEY_DOWN, ev.scancode, ev.repeat, e);
				return 1;
			case STRATA_EVENT_MOUSE_BUTTON_DOWN:
			case STRATA_EVENT_MOUSE_BUTTON_UP:
				clickEvent(ev.type == STRATA_EVENT_MOUSE_BUTTON_DOWN ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP, ev.button,
						ev.x, ev.y, e);
				return 1;
			case STRATA_EVENT_RESIZED:
				if (ev.width > 0 && ev.height > 0) {
					d.width = ev.width;
					d.height = ev.height;
				}
				break;
			default:
				break;
		}
	}
	return 0;
}

// The last frame as a PNG (for the screenshots in docs/).
inline bool savePng(const char* path) {
	Desktop& d = desktop();
	return d.engine && d.api.saveScreenshotPng(d.engine, path) == STRATA_OK;
}

inline void close() {
	Desktop& d = desktop();
	if (d.engine) {
		d.api.destroy(d.engine);
		d.engine = nullptr;
	}
}

}  // namespace gridview3d

#endif
