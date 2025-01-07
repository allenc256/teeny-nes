#include <SDL.h>
#include <imgui.h>

#include "src/app/game_window.h"
#include "src/app/palette.h"
#include "src/app/pixel.h"

GameWindow::GameWindow(Nes &nes, SDL_Renderer *renderer)
    : nes_(nes),
      frame_(renderer, FRAME_WIDTH, FRAME_HEIGHT) {}

static float clamp(float x, float min, float max) {
  return std::min(std::max(x, min), max);
}

static void render_tooltip() {
  ImVec2 item  = ImGui::GetItemRectMin();
  ImVec2 mouse = ImGui::GetMousePos();
  float  x     = clamp(
      (mouse.x - item.x) / GameWindow::SCALE_X, 0, GameWindow::FRAME_WIDTH - 1
  );
  float y = clamp(
      (mouse.y - item.y) / GameWindow::SCALE_Y, 0, GameWindow::FRAME_HEIGHT - 1
  );
  ImGui::SetTooltip("x=%03.0f, y=%03.0f", x, y);
}

void GameWindow::render(bool show_tooltip) {
  if (!nes_.is_powered_on()) {
    assert(false);
    return;
  }

  auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
               ImGuiWindowFlags_NoSavedSettings;

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  if (ImGui::Begin("Game", nullptr, flags)) {
    prepare_frame();
    ImGui::Image(
        (ImTextureID)frame_.get(),
        ImVec2(FRAME_WIDTH * SCALE_X, FRAME_HEIGHT * SCALE_Y)
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
  for (int row = 0; row < FRAME_HEIGHT; row++) {
    for (int col = 0; col < FRAME_WIDTH; col++) {
      int src_idx  = (row + OVERSCAN) * 256 + col;
      int dst_idx  = row * pitch + col;
      dst[dst_idx] = palette[src[src_idx]];
    }
  }
  SDL_UnlockTexture(frame_.get());
}
