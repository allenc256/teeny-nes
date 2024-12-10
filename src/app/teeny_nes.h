#pragma once

#include "src/app/imgui.h"
#include "src/app/name_table.h"
#include "src/app/pattern_table.h"
#include "src/app/sdl.h"
#include "src/app/stats.h"
#include "src/emu/nes.h"

class TeenyNes {
public:
  TeenyNes();

  void run();

private:
  void render_imgui();
  void render_imgui_menu();

  SDLRes         sdl_;
  SDLWindowRes   window_;
  SDLRendererRes renderer_;
  ImguiRes       imgui_;

  Nes nes_;

  bool         show_pattern_table_;
  bool         show_name_table_;
  bool         show_stats_;
  PatternTable pattern_table_;
  NameTable    name_table_;
  Stats        stats_;
};
