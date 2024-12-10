#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "src/app/teeny_nes.h"

TeenyNes::TeenyNes()
    : window_("teeny-nes", 1024, 768),
      renderer_(window_.get()),
      imgui_(window_.get(), renderer_.get()),
      show_pattern_table_(true),
      show_name_table_(true),
      show_stats_(true),
      pattern_table_(nes_, renderer_.get()),
      name_table_(nes_, renderer_.get()),
      stats_(nes_) {
  nes_.load_cart("hello_world.nes");
  nes_.power_up();
  while (nes_.ppu().frames() < 2) {
    nes_.step();
  }
}

void TeenyNes::run() {
  ImGuiIO &io = ImGui::GetIO();

  bool done = false;
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        done = true;
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window_.get()))
        done = true;
    }
    if (SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_MINIMIZED) {
      SDL_Delay(10);
      continue;
    }

    render_imgui();

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
    ImGui_ImplSDLRenderer2_RenderDrawData(
        ImGui::GetDrawData(), renderer_.get()
    );
    SDL_RenderPresent(renderer_.get());
  }
}

void TeenyNes::render_imgui() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  ImGui::ShowDemoWindow();

  render_imgui_menu();

  if (show_pattern_table_) {
    pattern_table_.render();
  }
  if (show_name_table_) {
    name_table_.render();
  }
  if (show_stats_) {
    stats_.render();
  }

  ImGui::Render();
}

void TeenyNes::render_imgui_menu() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Debug")) {
      ImGui::MenuItem("Show Pattern Table", nullptr, &show_pattern_table_);
      ImGui::MenuItem("Show Name Table", nullptr, &show_name_table_);
      ImGui::MenuItem("Show Stats", nullptr, &show_stats_);
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
