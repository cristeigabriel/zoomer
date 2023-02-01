#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <byteswap.h>
#include <stdarg.h>
#include <stdio.h>

#define DEBUG_IMAGE 0

#define FULLSCREEN_W 0
#define FULLSCREEN_H 0

struct Zoomer {
  struct SDL_Window *window;
  XWindowAttributes root_attributes;
  uint32_t *image;
  struct SDL_Renderer *renderer;
};

static void die(const char *fmt, ...) {
  // Pass on formatting
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  // End with dying
  exit(EXIT_SUCCESS);
}

#define STRINGIFY(x) #x
#define SDL_DIE(x)                                                             \
  die(__FILE__ ":%d: Could not initialize SDL " STRINGIFY(x) ": %s\n",         \
      __LINE__, SDL_GetError())

// Initializes hidden window! You must make it visible after intialization.
static struct SDL_Window *initialize_window(const char *name, int w, int h) {
  struct SDL_Window *window;
  Uint32 flag;

  flag = SDL_WINDOW_HIDDEN;
  if (w == FULLSCREEN_W && h == FULLSCREEN_H)
    flag |= SDL_WINDOW_FULLSCREEN_DESKTOP;

  window = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, w, h, flag);
  if (NULL == window)
    SDL_DIE(window);

  return window;
}

static struct SDL_Renderer *initialize_renderer(struct SDL_Window *window) {
  struct SDL_Renderer *renderer;

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  if (NULL == renderer)
    SDL_DIE(renderer);

  return renderer;
}

static void install_zoomer(struct Zoomer *zoomer, const char *name, int w,
                           int h) {
  struct SDL_SysWMinfo info;
  Display *display;
  Window root;
  XImage *image;
  int row;
  int col;
  unsigned long i;

  printf("Installing Zoomer...\n");

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

      pixel = XGetPixel(image, col, row);
      // Swap first with last color channel and add alpha.
      // Quicker (to write) way of changing color mask.
      zoomer->image[i++] = 0xff000000 + __bswap_32(pixel << 8);
    }
  }
  XFree(image);

  // Since we've taken our screenshot, we're ready to present the window
  SDL_ShowWindow(zoomer->window);

  // Initialize renderer
  zoomer->renderer = initialize_renderer(zoomer->window);
}

static void destroy_zoomer(struct Zoomer *zoomer) {
  printf("Destroying Zoomer context...\n");

  SDL_DestroyWindow(zoomer->window);
  SDL_DestroyRenderer(zoomer->renderer);
}

int main(void) {
  struct Zoomer zoomer;
  SDL_Texture *buffer;
  int pitch;

  // Initiate isntallation
  install_zoomer(&zoomer, "Zoomer", FULLSCREEN_W, FULLSCREEN_H);

#if DEBUG_IMAGE == 1
  FILE *file;
  // convert -size 3840x1080 -depth 8 image.rgba image.png
  file = fopen("image.rgba", "w");
  fwrite(zoomer.image,
         zoomer.root_attributes.width * zoomer.root_attributes.height,
         sizeof *zoomer.image, file);
  fclose(file);
#endif

  buffer = SDL_CreateTexture(
      zoomer.renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
      zoomer.root_attributes.width, zoomer.root_attributes.height);
  if (NULL == buffer)
    SDL_DIE(buffer);

  pitch = zoomer.root_attributes.width * (sizeof *zoomer.image);
  if (SDL_UpdateTexture(buffer, NULL, (const void *)zoomer.image, pitch) != 0)
    SDL_DIE(buffer);

  // Application main loop
  for (;;) {
    SDL_Event e;

    if (SDL_PollEvent(&e)) {
      if ((e.type == SDL_QUIT) ||
          (e.type = SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN))
        break;
    }

    // Write the buffer to renderer
    SDL_RenderCopy(zoomer.renderer, buffer, NULL, NULL);

    // Draw line on renderer
    SDL_SetRenderDrawColor(zoomer.renderer, 255, 0, 255, 255);
    SDL_RenderDrawLine(zoomer.renderer, 0, 0, 1920, 1080);

    SDL_RenderPresent(zoomer.renderer);
  }

  SDL_DestroyTexture(buffer);

  // Initiate cleaning up
  destroy_zoomer(&zoomer);

  return EXIT_SUCCESS;
}