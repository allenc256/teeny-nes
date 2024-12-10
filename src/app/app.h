#pragma once

#include "src/app/imgui.h"
#include "src/app/pattern_table.h"
#include "src/app/sdl.h"
#include "src/app/stats.h"
#include "src/emu/nes.h"

class App {
public:
  App();

  void run();

private:
  void render_menu();

  SDLRes         sdl_;
  SDLWindowRes   window_;
  SDLRendererRes renderer_;
  ImguiRes       imgui_;

  bool         show_pattern_table_;
  PatternTable pattern_table_;
  bool         show_stats_;
  Stats        stats_;
};
