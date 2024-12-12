#include <SDL.h>
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
  nes_.load_cart("hello_world.nes");
  nes_.power_up();
  while (nes_.ppu().frames() < 5) {
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

    show_ppu_window_ &= nes_.is_powered_up();

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
