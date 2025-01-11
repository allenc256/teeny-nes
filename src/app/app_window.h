#pragma once

#include <chrono>
#include <filesystem>

#include "src/app/game_genie_window.h"
#include "src/app/game_window.h"
#include "src/app/imgui.h"
#include "src/app/keyboard.h"
#include "src/app/nfd.h"
#include "src/app/sdl.h"
#include "src/emu/nes.h"
#include "src/emu/timer.h"

class AppWindow {
public:
  AppWindow();

  void run();

private:
  using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;

  bool process_events();
  void step();
  void render();
  void render_imgui();
  void render_imgui_menu();
  void open_rom();
  void queue_audio();

  void power_on();
  void power_off();
  void reset();

  void save_rom_state();
  void load_rom_state();

  Nes                nes_;
  KeyboardController keyboard_;
  Timer              timer_;
  bool               paused_;

  std::filesystem::path pref_path_;
  std::string           rom_name_;

  bool show_gg_window_;

  Nfd               nfd_;
  SDLRes            sdl_;
  SDLWindowRes      window_;
  SDLRendererRes    renderer_;
  SDLAudioDeviceRes audio_dev_;
  ImguiRes          imgui_;
  GameWindow        game_window_;
  GameGenieWindow   gg_window_;
};
