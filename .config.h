/* Basic Configuration Header for "Zoomer" by Cristei Gabriel-Marian
 * https://github.com/cristeigabriel/zoomer */

#define DRAG_NKEYSETS 2

#define ZOOM_DEFAULT_TIME  (0.3f)
#define DEFAULT_GRID_ALPHA 30

const float g_zoom_speed         = 0.05f;

const float g_drag_speed_x = 0.75f;
const float g_drag_speed_y = 0.75f;

const float g_nav_inc_x = 50.0f;
const float g_nav_inc_y = 50.0f;

const int drag_keys[DRAG_NKEYSETS][4] = { 
/*        UP       DOWN       LEFT       RIGHT */
	{ SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT },
	{ SDLK_w,  SDLK_s,    SDLK_a,    SDLK_d     }
};


