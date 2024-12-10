#include <SDL.h>
#include <cstring>
#include <imgui.h>

#include "src/app/pattern_table.h"
#include "src/app/pixel.h"

PatternTable::PatternTable(Nes &nes, SDL_Renderer *renderer)
    : nes_(nes),
      textures_{{renderer, 128, 128}, {renderer, 128, 128}},
      scale_(2) {}

static void show_tooltip(uint16_t base_addr, float scale) {
  ImVec2 item  = ImGui::GetItemRectMin();
  ImVec2 mouse = ImGui::GetMousePos();
  int    col   = (int)((mouse.x - item.x) / (8 * scale));
  int    row   = (int)((mouse.y - item.y) / (8 * scale));
  col          = std::min(std::max(col, 0), 15);
  row          = std::min(std::max(row, 0), 15);
  int pattern  = row * 16 + col;
  ImGui::SetTooltip("pattern_table=0x%04x, pattern=0x%02x", base_addr, pattern);
}

void PatternTable::render() {
  prepare_textures();

  ImVec2           size   = {128 * scale_, 128 * scale_};
  constexpr ImVec2 uv0    = {0, 0};
  constexpr ImVec2 uv1    = {1, 1};
  constexpr ImVec4 tint   = {1, 1, 1, 1};
  const ImVec4    &border = ImGui::GetStyleColorVec4(ImGuiCol_Border);

  ImGui::Begin("Pattern Table", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::BeginGroup();
    {
      ImGui::Image(
          (ImTextureID)textures_[0].get(), size, uv0, uv1, tint, border
      );
      if (ImGui::IsItemHovered()) {
        show_tooltip(0x0000, scale_);
      }
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    {
      ImGui::Image(
          (ImTextureID)textures_[1].get(), size, uv0, uv1, tint, border
      );
      if (ImGui::IsItemHovered()) {
        show_tooltip(0x1000, scale_);
      }
    }
    ImGui::EndGroup();

    ImGui::PopStyleVar();
  }
  ImGui::End();
}

static constexpr Pixel GRAYSCALE[4] = {
    {0, 0, 0},
    {0x3f, 0x3f, 0x3f},
    {0x7f, 0x7f, 0x7f},
    {0xbf, 0xbf, 0xbf},
};

static void
extract_pattern_table(Ppu &ppu, uint16_t base, Pixel *pixels, int pitch) {
  for (int i = 0; i < 256; i++) {
    int row = i >> 4;
    int col = i & 0xf;
    extract_pattern(ppu, base, i, pixels, pitch, col * 8, row * 8, GRAYSCALE);
  }
}

void PatternTable::prepare_textures() {
  if (!nes_.is_powered_up()) {
    clear_texture(textures_[0].get(), 128);
    clear_texture(textures_[1].get(), 128);
    return;
  }

  int    pitch;
  Pixel *pixels;

  SDL_LockTexture(textures_[0].get(), nullptr, (void **)&pixels, &pitch);
  extract_pattern_table(nes_.ppu(), 0x0000, pixels, pitch);
  SDL_UnlockTexture(textures_[0].get());

  SDL_LockTexture(textures_[1].get(), nullptr, (void **)&pixels, &pitch);
  extract_pattern_table(nes_.ppu(), 0x1000, pixels, pitch);
  SDL_UnlockTexture(textures_[1].get());
}

void extract_pattern(
    Ppu        &ppu,
    uint16_t    pattern_base_addr,
    int         pattern,
    Pixel      *pix,
    int         pix_pitch,
    int         pix_x,
    int         pix_y,
    const Pixel palette[4]
) {
  pix_pitch >>= 2;
  assert(pattern_base_addr == 0x0000 || pattern_base_addr == 0x1000);
  assert(pattern >= 0 && pattern < 256);
  assert(pix_pitch > 0);
  uint16_t addr = (uint16_t)(pattern_base_addr + (pattern << 4));
  for (uint16_t y_off = 0; y_off < 8; y_off++) {
    uint8_t lo_mem = ppu.peek(addr + y_off);
    uint8_t hi_mem = ppu.peek(addr + y_off + 8);
    for (int x_off = 0; x_off < 8; x_off++) {
      bool lo      = lo_mem & (1u << (8 - x_off));
      bool hi      = hi_mem & (1u << (8 - x_off));
      int  pix_idx = (pix_y + y_off) * pix_pitch + (pix_x + x_off);
      int  pal_idx = lo + (hi << 1);
      pix[pix_idx] = palette[pal_idx];
    }
  }
}
