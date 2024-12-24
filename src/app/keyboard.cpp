#include <SDL_keyboard.h>

#include "src/app/keyboard.h"

static constexpr SDL_Scancode KEYMAP[] = {
    SDL_SCANCODE_S,
    SDL_SCANCODE_A,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_UP,
    SDL_SCANCODE_DOWN,
    SDL_SCANCODE_LEFT,
    SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_X,
    SDL_SCANCODE_Z,
};

int KeyboardController::poll() {
  const Uint8 *states = SDL_GetKeyboardState(NULL);
  int          result = 0;
  for (int i = 0; i < 10; i++) {
    if (states[KEYMAP[i]]) {
      result |= 1u << i;
    }
  }
  return result;
}
