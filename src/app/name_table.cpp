#include <SDL.h>
#include <imgui.h>

#include "src/app/name_table.h"
#include "src/app/pattern_table.h"

NameTable::NameTable(Nes &nes, SDL_Renderer *renderer)
    : nes_(nes),
      texture_(renderer, 512, 480) {}

static void show_tooltip(Ppu &ppu, uint16_t base_addr) {
  ImVec2 item   = ImGui::GetItemRectMin();
  ImVec2 mouse  = ImGui::GetMousePos();
  int    col    = (int)((mouse.x - item.x) / 8);
  int    row    = (int)((mouse.y - item.y) / 8);
  col           = std::min(std::max(col, 0), 31);
  row           = std::min(std::max(row, 0), 29);
  uint16_t addr = base_addr + (uint16_t)(row * 32 + col);
  uint8_t  mem  = ppu.peek(addr);
  ImGui::SetTooltip(
      "name_table=0x%04x, pattern=0x%02x, col=%02d, row=%02d",
      base_addr,
      mem,
      col,
      row
  );
}

void NameTable::render() {
  prepare_texture();

  constexpr ImVec4 tint   = {1, 1, 1, 1};
  const ImVec4    &border = ImGui::GetStyleColorVec4(ImGuiCol_Border);

  ImGui::Begin("Name Table", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::BeginGroup();
    {
      ImGui::Image(
          (ImTextureID)texture_.get(),
          ImVec2(256, 240),
          ImVec2(0.0, 0.0),
          ImVec2(0.5, 0.5),
          tint,
          border
      );
      if (ImGui::IsItemHovered()) {
        show_tooltip(nes_.ppu(), 0x2000);
      }
      ImGui::SameLine();
      ImGui::Image(
          (ImTextureID)texture_.get(),
          ImVec2(256, 240),
          ImVec2(0.5, 0.0),
          ImVec2(1.0, 0.5),
          tint,
          border
      );
      if (ImGui::IsItemHovered()) {
        show_tooltip(nes_.ppu(), 0x2400);
      }
    }
    ImGui::EndGroup();
    ImGui::BeginGroup();
    {
      ImGui::Image(
          (ImTextureID)texture_.get(),
          ImVec2(256, 240),
          ImVec2(0.0, 0.5),
          ImVec2(0.5, 1.0),
          tint,
          border
      );
      if (ImGui::IsItemHovered()) {
        show_tooltip(nes_.ppu(), 0x2800);
      }
      ImGui::SameLine();
      ImGui::Image(
          (ImTextureID)texture_.get(),
          ImVec2(256, 240),
          ImVec2(0.5, 0.5),
          ImVec2(1.0, 1.0),
          tint,
          border
      );
      if (ImGui::IsItemHovered()) {
        show_tooltip(nes_.ppu(), 0x2c00);
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

static void render_name_table(
    Ppu &ppu, uint16_t base, Pixel *pix, int pix_pitch, int pix_x, int pix_y
) {
  assert(base == 0x2000 || base == 0x2400 || base == 0x2800 || base == 0x2c00);
  for (int nt_y = 0; nt_y < 30; nt_y++) {
    for (int nt_x = 0; nt_x < 32; nt_x++) {
      uint16_t addr    = (uint16_t)(base + nt_y * 32 + nt_x);
      uint8_t  pattern = ppu.peek(addr);
      extract_pattern(
          ppu,
          ppu.bg_pattern_table_addr(),
          pattern,
          pix,
          pix_pitch,
          pix_x + nt_x * 8,
          pix_y + nt_y * 8,
          GRAYSCALE
      );
    }
  }
}

void NameTable::prepare_texture() {
  if (!nes_.is_powered_up()) {
    clear_texture(texture_.get(), 480);
    return;
  }

  int    pitch;
  Pixel *pixels;

  SDL_LockTexture(texture_.get(), nullptr, (void **)&pixels, &pitch);
  render_name_table(nes_.ppu(), 0x2000, pixels, pitch, 0, 0);
  render_name_table(nes_.ppu(), 0x2400, pixels, pitch, 256, 0);
  render_name_table(nes_.ppu(), 0x2800, pixels, pitch, 0, 240);
  render_name_table(nes_.ppu(), 0x2c00, pixels, pitch, 256, 240);
  SDL_UnlockTexture(texture_.get());
}
