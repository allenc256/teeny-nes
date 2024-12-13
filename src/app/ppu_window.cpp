#include <SDL.h>
#include <imgui.h>

#include "src/app/palette.h"
#include "src/app/pixel.h"
#include "src/app/ppu_window.h"

PpuWindow::PpuWindow(Nes &nes, SDL_Renderer *renderer)
    : nes_(nes),
      pt_tex_(renderer, 256, 128),
      nt_tex_(renderer, 512, 480) {}

void PpuWindow::render() {
  if (!nes_.is_powered_up()) {
    assert(false);
    return;
  }

  if (ImGui::Begin("PPU", nullptr, ImGuiWindowFlags_HorizontalScrollbar)) {
    render_pattern_table();
    render_name_table();
    render_attr_table();
    render_palette();
    render_stats();
  }
  ImGui::End();
}

static void show_pt_tooltip() {
  ImVec2 item  = ImGui::GetItemRectMin();
  ImVec2 mouse = ImGui::GetMousePos();
  int    col   = (int)((mouse.x - item.x) / 16);
  int    row   = (int)((mouse.y - item.y) / 16);
  col          = std::min(std::max(col, 0), 31);
  row          = std::min(std::max(row, 0), 15);

  uint16_t base_addr;
  int      pattern;
  if (col < 16) {
    base_addr = 0x0000;
    pattern   = row * 16 + col;
  } else {
    base_addr = 0x1000;
    pattern   = row * 16 + col - 16;
  }

  ImGui::SetTooltip("pattern_table=0x%04x, pattern=0x%02x", base_addr, pattern);
}

void PpuWindow::render_pattern_table() {
  if (!ImGui::CollapsingHeader("Pattern Table")) {
    return;
  }

  prepare_pt_tex();
  ImGui::Image(
      (ImTextureID)pt_tex_.get(),
      ImVec2(256 * 2, 128 * 2),
      ImVec2(0, 0),
      ImVec2(1, 1),
      ImVec4(1, 1, 1, 1),
      ImGui::GetStyleColorVec4(ImGuiCol_Border)
  );
  if (ImGui::IsItemHovered()) {
    show_pt_tooltip();
  }
}

static void show_nt_tooltip(Ppu &ppu) {
  ImVec2 item        = ImGui::GetItemRectMin();
  ImVec2 mouse       = ImGui::GetMousePos();
  int    abs_col     = (int)((mouse.x - item.x) / 8);
  int    abs_row     = (int)((mouse.y - item.y) / 8);
  abs_col            = std::min(std::max(abs_col, 0), 63);
  abs_row            = std::min(std::max(abs_row, 0), 59);
  int      col0      = abs_col / 32;
  int      row0      = abs_row / 30;
  int      col1      = abs_col % 32;
  int      row1      = abs_row % 30;
  uint16_t base_addr = (uint16_t)(0x2000 + (row0 * 2 + col0) * 1024);
  uint16_t addr      = (uint16_t)(base_addr + row1 * 32 + col1);
  uint8_t  mem       = ppu.peek(addr);
  ImGui::SetTooltip(
      "name_table=0x%04x, pattern=0x%02x, col=%02d, row=%02d",
      base_addr,
      mem,
      col1,
      row1
  );
}

void PpuWindow::render_name_table() {
  if (!ImGui::CollapsingHeader("Name Table")) {
    return;
  }

  prepare_nt_tex();
  ImGui::Image(
      (ImTextureID)nt_tex_.get(),
      ImVec2(512, 480),
      ImVec2(0, 0),
      ImVec2(1, 1),
      ImVec4(1, 1, 1, 1),
      ImGui::GetStyleColorVec4(ImGuiCol_Border)
  );
  if (ImGui::IsItemHovered()) {
    show_nt_tooltip(nes_.ppu());
  }
}

void PpuWindow::render_palette() {
  if (!ImGui::CollapsingHeader("Palette")) {
    return;
  }

  uint16_t base_addr = 0x3f00;
  if (ImGui::BeginTable("Palette", 16)) {
    for (int row = 0; row < 2; row++) {
      for (int col = 0; col < 16; col++) {
        ImGui::TableNextColumn();
        uint8_t mem     = nes_.ppu().peek(base_addr + (uint16_t)col);
        Pixel   pal_col = PALETTE[0][mem & 63];
        ImVec4  bg_col  = ImVec4(
            pal_col.r() / 255.0f,
            pal_col.g() / 255.0f,
            pal_col.b() / 255.0f,
            1.0
        );
        ImVec4 fg_col =
            ImVec4(1.0f - bg_col.x, 1.0f - bg_col.y, 1.0f - bg_col.z, 1.0f);
        ImGui::TableSetBgColor(
            ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(bg_col)
        );
        ImGui::TextColored(fg_col, "%02x", mem);
      }
      base_addr += 0x10;
    }
    ImGui::EndTable();
  }
}

void PpuWindow::render_stats() {
  if (!ImGui::CollapsingHeader("Stats")) {
    return;
  }

  if (ImGui::BeginTable("Stats", 2, ImGuiTableFlags_SizingFixedFit)) {
    ImGui::TableNextColumn();
    ImGui::Text("Cycles:");
    ImGui::TableNextColumn();
    ImGui::Text("%" PRIi64, nes_.ppu().cycles());

    ImGui::TableNextColumn();
    ImGui::Text("Frames:");
    ImGui::TableNextColumn();
    ImGui::Text("%" PRIi64, nes_.ppu().frames());

    ImGui::EndTable();
  }
}

static void extract_pattern(
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

static void extract_pattern_table(
    Ppu        &ppu,
    uint16_t    pattern_base_addr,
    Pixel      *pix,
    int         pix_pitch,
    int         pix_x,
    int         pix_y,
    const Pixel palette[4]
) {
  for (int i = 0; i < 256; i++) {
    int row = i >> 4;
    int col = i & 0xf;
    extract_pattern(
        ppu,
        pattern_base_addr,
        i,
        pix,
        pix_pitch,
        pix_x + col * 8,
        pix_y + row * 8,
        palette
    );
  }
}

void PpuWindow::prepare_pt_tex() {
  int    pix_pitch;
  Pixel *pix;
  Pixel  pal[4] = {
      PALETTE[0][0x0f], PALETTE[0][0x00], PALETTE[0][0x10], PALETTE[0][0x20]
  };

  SDL_LockTexture(pt_tex_.get(), nullptr, (void **)&pix, &pix_pitch);
  extract_pattern_table(nes_.ppu(), 0x0000, pix, pix_pitch, 0, 0, pal);
  extract_pattern_table(nes_.ppu(), 0x1000, pix, pix_pitch, 128, 0, pal);
  SDL_UnlockTexture(pt_tex_.get());
}

static void extract_name_table(
    Ppu &ppu, uint16_t base, Pixel *pix, int pix_pitch, int pix_x, int pix_y
) {
  Pixel pal[4] = {
      PALETTE[0][0x0f], PALETTE[0][0x00], PALETTE[0][0x10], PALETTE[0][0x20]
  };

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
          pal
      );
    }
  }
}

void PpuWindow::prepare_nt_tex() {
  int    pitch;
  Pixel *pixels;

  SDL_LockTexture(nt_tex_.get(), nullptr, (void **)&pixels, &pitch);
  extract_name_table(nes_.ppu(), 0x2000, pixels, pitch, 0, 0);
  extract_name_table(nes_.ppu(), 0x2400, pixels, pitch, 256, 0);
  extract_name_table(nes_.ppu(), 0x2800, pixels, pitch, 0, 240);
  extract_name_table(nes_.ppu(), 0x2c00, pixels, pitch, 256, 240);
  SDL_UnlockTexture(nt_tex_.get());
}

static void
extract_attr_table(Ppu &ppu, uint16_t base_addr, int (*palettes)[16]) {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      uint16_t addr          = (uint16_t)(base_addr + row * 8 + col);
      uint8_t  mem           = ppu.peek(addr);
      int      top_left      = mem & 0b00000011;
      int      top_right     = (mem & 0b00001100) >> 2;
      int      bot_left      = (mem & 0b00110000) >> 4;
      int      bot_right     = (mem & 0b11000000) >> 6;
      int      x             = col * 2;
      int      y             = row * 2;
      palettes[x][y]         = top_left;
      palettes[x + 1][y]     = top_right;
      palettes[x][y + 1]     = bot_left;
      palettes[x + 1][y + 1] = bot_right;
    }
  }
}

void PpuWindow::render_attr_table() {
  if (!ImGui::CollapsingHeader("Attribute Table")) {
    return;
  }
  int palettes[16][16];
  extract_attr_table(nes_.ppu(), 0x23c0, palettes);
  for (int row = 0; row < 15; row++) {
    char text[17] = {0};
    for (int i = 0; i < 16; i++) {
      text[i] = (char)(palettes[i][row] + '0');
    }
    ImGui::Text("%s", text);
  }
}
