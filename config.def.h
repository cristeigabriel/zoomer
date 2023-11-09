/* Basic Configuration Header for "Zoomer" by Cristei Gabriel-Marian
 * https://github.com/cristeigabriel/zoomer */

#define DRAG_NKEYSETS 2

const float g_zoom_speed = 0.05f;

const float g_drag_speed_x = 1.00f;
const float g_drag_speed_y = 1.00f;

const float g_nav_inc_x = 50.0f;
const float g_nav_inc_y = 50.0f;

const int drag_keys[DRAG_NKEYSETS][4] = { 
	{ SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT },
	{ SDLK_w,  SDLK_s,    SDLK_a,    SDLK_d     }
};
