#include <SDL.h>
#include <chrono>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "src/app/palette.h"
#include "src/app/teeny_nes.h"

TeenyNes::TeenyNes()
    : window_("teeny-nes", 1024, 768),
      renderer_(window_.get()),
      imgui_(window_.get(), renderer_.get()),
      game_window_(nes_, renderer_.get()),
      show_ppu_window_(true),
      ppu_window_(nes_, renderer_.get()) {
  nes_.input().set_controller(&keyboard_, 0);
  nes_.load_cart("mario.nes");
  nes_.power_up();
}

void TeenyNes::run() {
  prev_ts_ = std::chrono::high_resolution_clock::now();
  while (process_events()) {
    step();
    render();
  }
}

bool TeenyNes::process_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT)
      return false;
    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(window_.get()))
      return false;
  }
  return true;
}

void TeenyNes::step() {
  static constexpr int64_t cpu_hz     = 1789773;
  static constexpr int64_t max_cycles = cpu_hz / 10;
  using CpuCycles = std::chrono::duration<int64_t, std::ratio<1, cpu_hz>>;

  auto now     = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<CpuCycles>(now - prev_ts_);
  int  cycles  = (int)std::min(elapsed.count(), max_cycles);
  prev_ts_     = now;
  nes_.step(cycles);
}

void TeenyNes::render() {
  if (SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return;
  }

  show_ppu_window_ &= nes_.is_powered_up();

  render_imgui();

  ImGuiIO &io = ImGui::GetIO();
  SDL_RenderSetScale(
      renderer_.get(),
      io.DisplayFramebufferScale.x,
      io.DisplayFramebufferScale.y
  );
  const ImVec4 &bg_color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
  SDL_SetRenderDrawColor(
      renderer_.get(),
      (uint8_t)(bg_color.x * 255.0),
      (uint8_t)(bg_color.y * 255.0),
      (uint8_t)(bg_color.z * 255.0),
      0
  );
  SDL_RenderClear(renderer_.get());
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_.get());
  SDL_RenderPresent(renderer_.get());
}

void TeenyNes::render_imgui() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  ImGui::ShowDemoWindow();

  render_imgui_menu();

  if (nes_.is_powered_up()) {
    game_window_.render();
  }
  if (show_ppu_window_) {
    ppu_window_.render();
  }

  ImGui::Render();
}

void TeenyNes::render_imgui_menu() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Debug")) {
      ImGui::MenuItem(
          "Show PPU Window", nullptr, &show_ppu_window_, nes_.is_powered_up()
      );
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

int main() {
  TeenyNes teeny_nes;
  teeny_nes.run();
  return 0;
}
