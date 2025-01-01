#include <SDL.h>
#include <imgui.h>

#include "src/app/game_window.h"
#include "src/app/palette.h"
#include "src/app/pixel.h"

static constexpr float SCALE_X = 3.0f;
static constexpr float SCALE_Y = 2.25f;

GameWindow::GameWindow(Nes &nes, SDL_Renderer *renderer)
    : nes_(nes),
      frame_(renderer, 256, 240) {}

static float clamp(float x, float min, float max) {
  return std::min(std::max(x, min), max);
}

static void render_tooltip() {
  ImVec2 item  = ImGui::GetItemRectMin();
  ImVec2 mouse = ImGui::GetMousePos();
  float  x     = clamp((mouse.x - item.x) / SCALE_X, 0, 255);
  float  y     = clamp((mouse.y - item.y) / SCALE_Y, 0, 239);
  ImGui::SetTooltip("x=%03.0f, y=%03.0f", x, y);
}

void GameWindow::render(bool show_tooltip) {
  if (!nes_.is_powered_on()) {
    assert(false);
    return;
  }

  if (ImGui::Begin("Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    prepare_frame();
    ImGui::Image(
        (ImTextureID)frame_.get(), ImVec2(256 * SCALE_X, 240 * SCALE_Y)
    );
    if (show_tooltip && ImGui::IsItemHovered()) {
      render_tooltip();
    }
  }
  ImGui::End();
}

void GameWindow::prepare_frame() {
  int    pitch;
  Pixel *dst;
  int    emphasis = nes_.ppu().color_emphasis();
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
