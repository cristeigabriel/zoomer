/* Basic Configuration Header for "Zoomer" by Cristei Gabriel-Marian */
/* https://github.com/cristeigabriel/zoomer */

//#define DRAG_NMOUSE_BUTTONS 2
#define DRAG_NKEYSETS 2

/* -- CONTROLS -- */
const float g_zoom_speed = 0.05f;

const float g_drag_speed_x = 0.75f;
const float g_drag_speed_y = 0.75f;

const float g_nav_inc_x = 20.0f;
const float g_nav_inc_y = 20.0f;

/* This throws an error, but it will be fine when included in main.c */
/* TODO: This will not work, no matter what I seem to try. The SDL gods hate me. */
//unsigned drag_mouse_buttons[DRAG_NBUTTONS] = { SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT };

/* Of course this doesn't work, why would it work? That would be too easy. */
const int drag_keys[DRAG_NKEYSETS][4] = { 
	{ SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT },
	{ SDLK_w,  SDLK_s,    SDLK_a,    SDLK_d     }
};
