#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <byteswap.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* -- CUSTOM CONFIGURATION -- */
#include "config.h"

#define DEBUG_IMAGE 0

#define FULLSCREEN_W 0
#define FULLSCREEN_H 0

static float g_zoom_time  = ZOOM_DEFAULT_TIME;
static int   g_grid_alpha = DEFAULT_GRID_ALPHA;

struct Zoomer {
	struct SDL_Window*	 window;
	XWindowAttributes		 root_attributes;
	uint32_t*						 image;
	struct SDL_Renderer* renderer;
};

static void
die(const char* fmt, ...)
{
	// Pass on formatting
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	// End with dying
	exit(EXIT_FAILURE);
}

#define STRINGIFY(x) #x
#define SDL_DIE(x)                                                             \
	die(__FILE__ ":%d: Could not initialize SDL " STRINGIFY(x) ": %s\n",         \
			__LINE__, SDL_GetError())
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define clamped(a, b, c) max(b, min(a, c))
#define lerp(a, b, t) (a) + (t) * ((b) - (a))

// Initializes hidden window! You must make it visible after intialization.
static struct SDL_Window*
initialize_window(const char* name, int w, int h)
{
	struct SDL_Window* window;
	Uint32						 flag;

	flag = SDL_WINDOW_HIDDEN;
	if (w == FULLSCREEN_W && h == FULLSCREEN_H)
		flag |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	window = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED,
														SDL_WINDOWPOS_UNDEFINED, w, h, flag);
	if (NULL == window)
		SDL_DIE(window);

	/* Investigating ways to get the currently active monitor */
//	printf("SDL_GetwindowDisplayIndex: %i", 	SDL_GetWindowDisplayIndex(window));

	return window;
}

static struct SDL_Renderer*
initialize_renderer(struct SDL_Window* window)
{
	struct SDL_Renderer* renderer;

	renderer = SDL_CreateRenderer(window, -1, 0);
	if (NULL == renderer)
		SDL_DIE(renderer);

	return renderer;
}

static uint32_t
rgb_to_abgr(uint32_t rgb, uint8_t a)
{
	return (a << 24) + __bswap_32(rgb << 8);
}

static void
install_zoomer(struct Zoomer* zoomer, const char* name, int w, int h)
{
	struct SDL_SysWMinfo info;
	Display*						 display;
	Window							 root;
	XImage*							 image;
	int									 row;
	int									 col;
	unsigned long				 i;

#ifdef _DEBUG
	printf("Installing Zoomer...\n");
#endif

	// Use x11 for backend
	SDL_SetHint("SDL_VIDEODRIVER", "x11");

	// Attempt to initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		SDL_DIE(zoomer);

	// Schedule exit event
	atexit(SDL_Quit);

	// Initializes hidden window, we need it as such to
	// get the video driver, to be able to take a screenshot of the display
	zoomer->window = initialize_window(name, w, h);

	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(zoomer->window, &info))
		SDL_DIE(info);

	// Get window video driver handle
	display = info.info.x11.display;

	// Get root window details
	root = XDefaultRootWindow(display);
	if (XGetWindowAttributes(display, root, &zoomer->root_attributes) == 0)
		SDL_DIE(zoomer->root_attributes);

	// Take screenshot
	image = XGetImage(display, root, 0, 0, zoomer->root_attributes.width,
										zoomer->root_attributes.height, AllPlanes, ZPixmap);
	if (NULL == image)
		SDL_DIE(image);

	// Image width and height will be the same as in the attributes we store
	i = 0;
	zoomer->image =
		malloc((sizeof *zoomer->image) * (image->width * image->height));
	for (row = 0; row < image->height; row++) {
		for (col = 0; col < image->width; col++) {
			unsigned long pixel;

			pixel							 = XGetPixel(image, col, row);
			zoomer->image[++i] = rgb_to_abgr(pixel, 0xff);
		}
	}
	XFree(image);

	// Since we've taken our screenshot, we're ready to present the window
	SDL_ShowWindow(zoomer->window);

	// Initialize renderer
	zoomer->renderer = initialize_renderer(zoomer->window);
}

static void
destroy_zoomer(struct Zoomer* zoomer)
{
#ifdef _DEBUG
	printf("Destroying Zoomer context...\n");
#endif

	SDL_DestroyWindow(zoomer->window);
	SDL_DestroyRenderer(zoomer->renderer);
}

static float
nearest_multiple(float x, float m)
{
	return m * floorf(x / m);
}

static void
snap_to_grid(const SDL_Rect* stat, const SDL_Rect* dyn, int mx, int my,
						 float* x, float* y, float* w, float* h)
{
	// Calculate width and height to fit on grid
	*w = (float)stat->w / (float)dyn->w;
	*h = (float)stat->h / (float)dyn->h;

	// Snap to nearest multiple of w, h
	*x = nearest_multiple((float)mx, *w);
	*y = nearest_multiple((float)my, *h);
}

static void
drag_axis(int *axis, int m, int p, float speed)
{
	*axis -= (m - p) * speed;
}

int
main(int argc, char* argv[])
{
	struct Zoomer zoomer;
	SDL_Texture*  buffer;
	struct SDL_Rect stat;
	struct SDL_Rect dyn;
	int    disps;
	int    gzw;
	int    gzh;
	int    zw;
	int    zh;
	int    px;
	int    py;
	int    w;
	int    h;
	int    pitch;
	Uint64 now;
	Uint64 last;
	float  dt;
	float  zt;
	float  ezt;
	float  p;
	bool   grid;
	bool   altmod;
	struct {
		float y;
		float p;
	} hold;

	for (int i = 1; i < argc; ++i) {
		const char* arg = argv[i];

		if (*arg != '-')
			goto usage;

		switch (arg[1]) {
			case 'h': {
				goto usage;
			} break;

			case 't': {
				if (argc < (i + 1))
					goto usage;

				i += 1;
				g_zoom_time = atof(argv[i]);
			} break;

			default: {
				printf("Unknown input: %s\n\n", &arg[1]);
				goto usage;
			} break;
		}
	}

	// Initiate isntallation
	install_zoomer(&zoomer, "zoomer", FULLSCREEN_W, FULLSCREEN_H);

#if DEBUG_IMAGE == 1
	FILE* file;
	// convert -size 3840x1080 -depth 8 image.rgba image.png
	file = fopen("image.rgba", "w");
	fwrite(zoomer.image,
				 zoomer.root_attributes.width * zoomer.root_attributes.height,
				 sizeof *zoomer.image, file);
	fclose(file);
#endif

	// Allocate buffer
	buffer = SDL_CreateTexture(
		zoomer.renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
		zoomer.root_attributes.width, zoomer.root_attributes.height);
	if (NULL == buffer)
		SDL_DIE(buffer);

	// Write image to buffer
	pitch = zoomer.root_attributes.width * (sizeof *zoomer.image);
	if (SDL_UpdateTexture(buffer, NULL, (const void*)zoomer.image, pitch) != 0)
		SDL_DIE(buffer);

	// Get window width and height to clip rect and calculate boundaries of
	// dragging
	SDL_GetWindowSize(zoomer.window, &w, &h);

	// Get number of screen displays
	if ((disps = SDL_GetNumVideoDisplays()) < 0)
		SDL_DIE(disps);

	// Default rectangle
	int di = SDL_GetWindowDisplayIndex(zoomer.window);
	for (int i = 0; i < di; ++i) {
		/* loop through each window beforehand and add their widths to get the offset */
	}

	stat = (SDL_Rect){.x = 2560 + 1, .y = 0, .w = w, .h = h};

	// Prepare variables for dragging
	px = -1;
	py = -1;

	// Prepare variables for dynamic rectangle, goal zoom and current zoom
	gzw = 0;
	gzh = 0;
	zw	= 0;
	zh	= 0;
	// Zoom time, end zoom time
	zt	= 0.f;
	ezt = 0.f;

	// Have grid disabled by default
	grid = false;

	// Altmod
	altmod = false;

	// Holdline, AKA when you press shift, it remembers the row
	hold.y = -1.f;
	hold.p = -1.f;

	// Prepare delta time
	now	 = SDL_GetPerformanceCounter();
	last = 0;
	dt	 = 0.f;

	bool quit = false;
	// Application main loop
	while (!quit) {
		SDL_Event				 e;
		int							 mx;
		int							 my;
		Uint32					 b;
		float						 dw;
		float						 dh;
		struct SDL_FRect rect;

		b = SDL_GetMouseState(&mx, &my);

		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT || (e.type == SDL_WINDOWEVENT &&	e.window.event == SDL_WINDOWEVENT_CLOSE))
				break;

			if (e.type == SDL_KEYUP) {
				switch (e.key.keysym.sym) {
					case SDLK_g: {
						grid = !grid;
					} break;

					case SDLK_LALT: {
						altmod = false;
					} break;
				}
			} else if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.sym) {
					case SDLK_LALT: {
						altmod = true;
					} break;

					case SDLK_q: {
						quit = true;
				     	} break;
				}

				/* This could probably be simplified, but this does make
				 * customization far easier for the end user, which should be the
				 * top priority. */
				for (int x = 0; x < 4; ++x) {
					for (int y = 0; y < DRAG_NKEYSETS; ++y) {
						if (e.key.keysym.sym == drag_keys[y][x]) {
							switch (x) {
								case 0: { drag_axis(&stat.y, py + 1, py, g_nav_inc_y); } break;
								case 1: { drag_axis(&stat.y, py - 1, py, g_nav_inc_y); } break;
								case 2: { drag_axis(&stat.x, px + 1, px, g_nav_inc_x); } break; 
								case 3: { drag_axis(&stat.x, px - 1, px, g_nav_inc_x); } break;
								default: break;
							}
						}
					}
				}
				if (e.key.keysym.sym > SDLK_0 &&
						e.key.keysym.sym <= SDLK_9) {
					int dispidx;

					dispidx = e.key.keysym.sym - SDLK_0;

					if (dispidx <= disps) {
						if (SDL_GetDisplayBounds(dispidx - 1, &stat) != 0)
							SDL_DIE(stat);

						// Don't fit to screen, actually
						stat.w = w;
						stat.h = h;

						// Reset zoom
						zw = gzw = zh = gzh = 0;
					}
				}
			}

			if (e.type == SDL_MOUSEWHEEL) {
				// Zoom in
				if (!altmod) {

					/* failed attempt to zoom in to cursor */
					//int wx, wy;
					//SDL_GetWindowSize(zoomer.window, &wx, &wy);
					//
					//// mouse distance
					//int mdx = (mx - (wx / 2));
					//int mdy = (my - (wy / 2));
					//
					//stat.x += mdx;
					//stat.y += mdy;

					gzw = clamped(gzw + e.wheel.y * (stat.w * g_zoom_speed), 0, stat.w / 2 - 3);
					gzh = clamped(gzh + e.wheel.y * (stat.h * g_zoom_speed), 0, stat.h / 2 - 3);

					// Store time and expected time for zoom to be finalized
					zt	= dt;
					ezt = zt + g_zoom_time;
				} else { // Grid alpha
					g_grid_alpha = clamped(g_grid_alpha + e.wheel.y * 10, 10, 120);
				}
			}
		}

		// Lerp towards goal zoom
		p	 = fmaxf(0, (ezt - dt) / g_zoom_time);
		zw = lerp(zw, gzw, sinf(1.f - p));
		zh = lerp(zh, gzh, sinf(1.f - p));

		if (hold.p != p)
			hold.y = -1.f;

		hold.p = p;


		// When dragging, move around the focus area
#if 0
		/* No other mouse button will work, wtf? */
		for (int i = 0; i < DRAG_NBUTTONS; ++i) {
			if (SDL_BUTTON(b) == (1 << drag_buttons[i]) && (px != -1) && (py != -1)) {
				drag_axis(&stat.x, mx, px, g_drag_speed_x);
				drag_axis(&stat.y, my, py, g_drag_speed_y);

				hold.y = -1.f;
			}
			fprintf(stdout, "%i\n", drag_buttons[i]);
		}
#endif /* 0 */
		if (SDL_BUTTON(b) == (1 << SDL_BUTTON_RIGHT) && (px != -1) && (py != -1)) {
			drag_axis(&stat.x, mx, px, g_drag_speed_x);
			drag_axis(&stat.y, my, py, g_drag_speed_y);

			hold.y = -1.f;
		}

		// Make sure static rectangle is clamped
		stat.x =
			clamped(stat.x, -zw, (zoomer.root_attributes.width - stat.w) + zw);
		stat.y =
			clamped(stat.y, -zh, (zoomer.root_attributes.height - stat.h) + zh);

		// Write the buffer to renderer

		dyn    = stat;
		dyn.x += zw;
		dyn.y += zh;
		dyn.w -= zw * 2;
		dyn.h -= zh * 2;
		SDL_RenderCopy(zoomer.renderer, buffer, &dyn, NULL);

		// Snap mouse to grid
		snap_to_grid(&stat, &dyn, mx, my, &rect.x, &rect.y, &rect.w, &rect.h);

		/* grid logic */
		if (grid) {
			// Do grid view
			dw = (float)stat.w / (float)dyn.w;
			dh = (float)stat.h / (float)dyn.h;

			// NOTE: should we even batch this manually? I don't see
			// any performance issues. It should not even be a problem, like,
			// ever, if I add some minimum value kinda-thing as to display
			// the grid

			SDL_SetRenderDrawBlendMode(zoomer.renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(zoomer.renderer, 255, 255, 255, g_grid_alpha);

			for (int i = 1; i < (stat.w / (int)dw); ++i) {
				SDL_RenderDrawLineF(
					zoomer.renderer,
					((float)i * dw),
					0.f,
					(i * (float)dw), (float)stat.h
				);
			}

			for (int i = 1; i < (stat.h / (int)dh); ++i) {
				SDL_RenderDrawLineF(
					zoomer.renderer,
					0.f,
					((float)i * dh),
					(float)stat.w,
					((float)i * dh)
				);
			}

// Don't run for now since it's not done
#if 0
			SDL_SetRenderDrawColor(zoomer.renderer, 255, 255, 255, 70);

			SDL_FRect cursor = rect;

			if (hold.y != -1.f)
				cursor.y = hold.y;

			SDL_RenderFillRectF(zoomer.renderer, &cursor);
#endif
		}

		SDL_RenderPresent(zoomer.renderer);

		// Post-render events
		if (e.key.keysym.sym == SDLK_LSHIFT) {
			if (e.type == SDL_KEYDOWN && hold.y == -1.f) {
				hold.y = rect.y;
			} else if (e.type == SDL_KEYUP) {
				hold.y = -1.f;
			}
		}

		// Update delta time
		last = now;
		now	 = SDL_GetPerformanceCounter();
		dt += (float)((now - last) / (float)SDL_GetPerformanceFrequency());

		// Update previous mouse position
		px = mx;
		py = my;

		// Give program some breathing time.
		SDL_Delay( 1 );
	}

	// Get rid of buffer
	SDL_DestroyTexture(buffer);

	// Initiate cleaning up
	destroy_zoomer(&zoomer);

	return EXIT_SUCCESS;

usage:
	printf("USAGE: %s <options>\n"
				 "-h         -- Displays this help and exits.\n\n"
				 "-t <float> -- Specifies zoom time.\n"
				 "How to configure:\n\n"
				 "config.h is where your personal settings are stored. If config.h"
				 "doesn't exist, running \'sudo ./build.sh\' will generate a new"
				 "one using config.def.h as a template. After modifying, run"
				 "\'sudo ./build.sh\' to rebuild and reinstall Zoomer.",
				 argv[0]);

	return EXIT_SUCCESS;
}
