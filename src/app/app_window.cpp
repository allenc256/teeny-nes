#include <SDL.h>
#include <chrono>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "src/app/app_window.h"
#include "src/app/palette.h"

AppWindow::AppWindow()
    : paused_(false),
      show_ppu_window_(false),
      show_xy_tooltip_(false),
      window_("teeny-nes", 1024, 768),
      renderer_(window_.get()),
      imgui_(window_.get(), renderer_.get()),
      game_window_(nes_, renderer_.get()),
      ppu_window_(nes_, renderer_.get()) {
  nes_.input().set_controller(&keyboard_, 0);
}

void AppWindow::run() {
  SDL_PauseAudioDevice(audio_dev_.get(), 0);

  while (process_events()) {
    step();
    queue_audio();
    render();
  }
}

bool AppWindow::process_events() {
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

void AppWindow::step() {
  if (!nes_.is_powered_on()) {
    return;
  }
  if (paused_) {
    return;
  }
  timer_.run(nes_);
}

void AppWindow::render() {
  if (SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return;
  }

  show_ppu_window_ &= nes_.is_powered_on();

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

void AppWindow::render_imgui() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  // ImGui::ShowDemoWindow();

  render_imgui_menu();

  if (nes_.is_powered_on()) {
    game_window_.render(show_xy_tooltip_);
  }
  if (show_ppu_window_) {
    ppu_window_.render();
  }

  ImGui::Render();
}

void AppWindow::render_imgui_menu() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Debug")) {
      if (ImGui::MenuItem("Open")) {
        open_rom();
      }
      bool prev_paused = paused_;
      ImGui::MenuItem("Pause", nullptr, &paused_, nes_.is_powered_on());
      if (prev_paused && !paused_) {
        timer_.reset();
      }
      ImGui::MenuItem(
          "Show X/Y Tooltip", nullptr, &show_xy_tooltip_, nes_.is_powered_on()
      );
      ImGui::MenuItem(
          "Show PPU Window", nullptr, &show_ppu_window_, nes_.is_powered_on()
      );
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void AppWindow::open_rom() {
  nfdu8filteritem_t filters[1] = {{"NES ROMs", "nes"}};
  auto              result     = nfd_.open_dialog(filters, 1);
  if (result.has_value()) {
    nes_.power_off();
    nes_.load_cart(*result);
    nes_.power_on();
    timer_.reset();
  }
}

static constexpr int AUDIO_QUEUE_TARGET = 1024;

void AppWindow::queue_audio() {
  if (paused_) {
    return;
  }

  auto &output = nes_.apu().output();
  float samples[ApuBuffer::CAPACITY];
  int   available = output.available();
  assert(available <= (int)ApuBuffer::CAPACITY);
  if (available == 0) {
    return;
  }

  for (int i = 0; i < available; i++) {
    samples[i] = output.read();
  }

  // Dynamically adjust sample rate per ideas in this thread:
  // https://forums.nesdev.org/viewtopic.php?f=3&t=11612.
  int queued = (int)(SDL_GetQueuedAudioSize(audio_dev_.get()) / sizeof(float));
  if (queued > AUDIO_QUEUE_TARGET) {
    nes_.apu().set_sample_rate(44050);
  } else {
    nes_.apu().set_sample_rate(44150);
  }

  int ret =
      SDL_QueueAudio(audio_dev_.get(), samples, available * sizeof(float));
  if (ret != 0) {
    throw std::runtime_error(
        std::format("audio failed to queue: {}", SDL_GetError())
    );
  }
}