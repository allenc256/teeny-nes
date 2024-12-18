#pragma once

#include <chrono>

#include "src/app/game_window.h"
#include "src/app/imgui.h"
#include "src/app/keyboard.h"
#include "src/app/ppu_window.h"
#include "src/app/sdl.h"
#include "src/emu/nes.h"

class TeenyNes {
public:
  TeenyNes();

  void run();

private:
  using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;

  bool process_events();
  void step();
  void render();
  void render_imgui();
  void render_imgui_menu();

  Nes                nes_;
  KeyboardController keyboard_;
  Timestamp          prev_ts_;

  SDLRes         sdl_;
  SDLWindowRes   window_;
  SDLRendererRes renderer_;
  ImguiRes       imgui_;
  GameWindow     game_window_;
  bool           show_ppu_window_;
  PpuWindow      ppu_window_;
};
