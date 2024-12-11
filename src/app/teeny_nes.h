#pragma once

#include "src/app/imgui.h"
#include "src/app/ppu_window.h"
#include "src/app/sdl.h"
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

  bool      show_ppu_window_;
  PpuWindow ppu_window_;
};
