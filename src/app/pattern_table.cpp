#include <SDL.h>
#include <imgui.h>

#include "src/app/pattern_table.h"

PatternTable::PatternTable(SDL_Renderer *renderer)
    : textures_{{renderer, 256, 256}, {renderer, 256, 256}} {}

void PatternTable::render() {
  prepare_textures();

  ImGui::Begin("Pattern Table", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    ImGui::BeginGroup();
    {
      ImGui::Text("Pattern Table $0000");
      ImGui::Image((ImTextureID)textures_[0].get(), ImVec2(256, 256));
    }
    ImGui::EndGroup();
    ImGui::SameLine(0, 10);
    ImGui::BeginGroup();
    {
      ImGui::Text("Pattern Table $1000");
      ImGui::Image((ImTextureID)textures_[1].get(), ImVec2(256, 256));
    }
    ImGui::EndGroup();
  }
  ImGui::End();
}

struct Pixel {
  uint8_t a;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

void PatternTable::prepare_textures() {
  int    pitch;
  Pixel *pixels;

  SDL_LockTexture(textures_[0].get(), nullptr, (void **)&pixels, &pitch);
  pitch /= 4;
  for (int y = 0; y < 256; y++) {
    for (int x = 0; x < 256; x++) {
      int i     = y * pitch + x;
      pixels[i] = {0, (uint8_t)y, 0, 0};
    }
  }
  SDL_UnlockTexture(textures_[0].get());

  SDL_LockTexture(textures_[1].get(), nullptr, (void **)&pixels, &pitch);
  pitch /= 4;
  for (int y = 0; y < 256; y++) {
    for (int x = 0; x < 256; x++) {
      int i     = y * pitch + x;
      pixels[i] = {0, (uint8_t)x, 0, 0};
    }
  }
  SDL_UnlockTexture(textures_[1].get());
}
