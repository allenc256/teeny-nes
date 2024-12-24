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
    SDL_SCANCODE_RIGHT
};

uint8_t KeyboardController::poll(int64_t frames) {
  const Uint8 *states = SDL_GetKeyboardState(NULL);
  uint8_t      result = 0;
  for (int i = 0; i < 8; i++) {
    if (states[KEYMAP[i]]) {
      result |= (uint8_t)(1u << i);
    }
  }
  if ((frames & 3) == 0) {
    if (states[SDL_SCANCODE_X]) {
      result |= A;
    }
    if (states[SDL_SCANCODE_Z]) {
      result |= B;
    }
  }
  return result;
}
