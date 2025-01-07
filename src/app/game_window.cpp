#include <SDL.h>
#include <imgui.h>

#include "src/app/game_window.h"
#include "src/app/palette.h"
#include "src/app/pixel.h"

static constexpr int   OVERSCAN     = 8;
static constexpr int   FRAME_WIDTH  = 256;
static constexpr int   FRAME_HEIGHT = 240 - OVERSCAN * 2;
static constexpr float FRAME_ASPECT =
    (FRAME_WIDTH * 4.0f) / (FRAME_HEIGHT * 3.0f);

GameWindow::GameWindow(Nes &nes, SDL_Renderer *renderer)
    : nes_(nes),
      frame_(renderer, FRAME_WIDTH, FRAME_HEIGHT) {}

void GameWindow::render() {
  if (!nes_.is_powered_on()) {
    assert(false);
    return;
  }

  auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
               ImGuiWindowFlags_NoSavedSettings |
               ImGuiWindowFlags_NoBringToFrontOnFocus;

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  if (ImGui::Begin("Game", nullptr, flags)) {
    prepare_frame();

    ImVec2 content_region = ImGui::GetContentRegionAvail();
    ImVec2 image_size;
    if (content_region.x > content_region.y * FRAME_ASPECT) {
      image_size.y = content_region.y;
      image_size.x = image_size.y * FRAME_ASPECT;
    } else {
      image_size.x = content_region.x;
      image_size.y = image_size.x / FRAME_ASPECT;
    }
    ImVec2 cursor_pos = ImGui::GetCursorPos();
    ImVec2 center     = ImVec2(
        cursor_pos.x + (content_region.x - image_size.x) * 0.5f,
        cursor_pos.y + (content_region.y - image_size.y) * 0.5f
    );
    ImGui::SetCursorPos(center);

    ImGui::Image((ImTextureID)frame_.get(), image_size);
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
