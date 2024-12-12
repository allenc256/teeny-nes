#include <SDL.h>
#include <imgui.h>

#include "src/app/game_window.h"
#include "src/app/palette.h"
#include "src/app/pixel.h"

GameWindow::GameWindow(Nes &nes, SDL_Renderer *renderer)
    : nes_(nes),
      frame_(renderer, 256, 240) {}

void GameWindow::render() {
  if (!nes_.is_powered_up()) {
    assert(false);
    return;
  }

  if (ImGui::Begin("Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    prepare_frame();
    ImGui::Image((ImTextureID)frame_.get(), ImVec2(256 * 2, 240 * 2));
  }
  ImGui::End();
}

void GameWindow::prepare_frame() {
  int    pitch;
  Pixel *dst;
  int    emphasis = nes_.ppu().emphasis();
  auto   src      = nes_.ppu().frame();
  auto   palette  = PALETTE[emphasis];

  assert(emphasis >= 0 && emphasis < 8);

  SDL_LockTexture(frame_.get(), nullptr, (void **)&dst, &pitch);
  pitch /= 4;
  for (int row = 0; row < 240; row++) {
    for (int col = 0; col < 256; col++) {
      int src_idx  = row * 256 + col;
      int dst_idx  = row * pitch + col;
      dst[dst_idx] = palette[src[src_idx]];
    }
  }
  SDL_UnlockTexture(frame_.get());
}