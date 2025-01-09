#include <SDL.h>
#include <SDL_filesystem.h>
#include <chrono>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "src/app/app_window.h"
#include "src/app/palette.h"

static constexpr int WINDOW_WIDTH  = 784;
static constexpr int WINDOW_HEIGHT = 539;

AppWindow::AppWindow()
    : paused_(false),
      show_ppu_window_(false),
      show_gg_window_(false),
      window_(
          "teeny-nes",
          (int)(WINDOW_WIDTH * sdl_.scale_factor()),
          (int)(WINDOW_HEIGHT * sdl_.scale_factor())
      ),
      renderer_(window_.get()),
      imgui_(window_.get(), renderer_.get()),
      game_window_(nes_, renderer_.get()),
      ppu_window_(nes_, renderer_.get()),
      gg_window_(nes_) {
  nes_.input().set_controller(&keyboard_, 0);

  ImGui::GetIO().FontGlobalScale = sdl_.scale_factor();
  ImGui::GetStyle().ScaleAllSizes(sdl_.scale_factor());

  char *pref_path_cstr = SDL_GetPrefPath("teeny-nes", "teeny-nes");
  if (!pref_path_cstr) {
    throw std::runtime_error("failed to get pref path");
  }
  pref_path_ = pref_path_cstr;
  SDL_free(pref_path_cstr);
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
  keyboard_.set_enabled(game_window_.focused());
  timer_.run(nes_);
}

void AppWindow::render() {
  if (SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return;
  }

  show_ppu_window_ &= nes_.is_powered_on();
  show_gg_window_ &= nes_.is_powered_on();

  render_imgui();

  ImGuiIO &io = ImGui::GetIO();
  SDL_RenderSetScale(
      renderer_.get(),
      io.DisplayFramebufferScale.x,
      io.DisplayFramebufferScale.y
  );
  SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 0);
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
    game_window_.render();
  }
  if (show_ppu_window_) {
    ppu_window_.render();
  }
  if (show_gg_window_) {
    gg_window_.render();
  }

  ImGui::Render();
}

void AppWindow::render_imgui_menu() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Game")) {
      if (ImGui::MenuItem("Open")) {
        open_rom();
      }
      bool prev_paused = paused_;
      ImGui::MenuItem("Pause", nullptr, &paused_, nes_.is_powered_on());
      if (prev_paused && !paused_) {
        timer_.reset();
      }
      if (ImGui::MenuItem("Power Off", nullptr, false, nes_.is_powered_on())) {
        power_off();
      }
      ImGui::MenuItem(
          "Show PPU Window", nullptr, &show_ppu_window_, nes_.is_powered_on()
      );
      ImGui::MenuItem(
          "Game Genie Codes", nullptr, &show_gg_window_, nes_.is_powered_on()
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
    power_off();
    nes_.load_cart(*result);
    rom_name_ = result->stem();
    power_on();
  }
}

static constexpr int AUDIO_QUEUE_TARGET = 2048;

void AppWindow::queue_audio() {
  if (paused_ || !nes_.is_powered_on()) {
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
    nes_.apu().set_sample_rate(44000);
  } else {
    nes_.apu().set_sample_rate(44200);
  }

  // if (queued == 0) {
  //   std::cout << "underflow detected!\n";
  // }

  int ret =
      SDL_QueueAudio(audio_dev_.get(), samples, available * sizeof(float));
  if (ret != 0) {
    throw std::runtime_error(
        std::format("audio failed to queue: {}", SDL_GetError())
    );
  }
}

static std::filesystem::path make_codes_path(
    const std::filesystem::path &pref_path, const std::string &rom_name
) {
  std::filesystem::path path = pref_path / rom_name;
  path.replace_extension(".codes");
  return path;
}

static std::filesystem::path make_sram_path(
    const std::filesystem::path &pref_path, const std::string &rom_name
) {
  std::filesystem::path path = pref_path / rom_name;
  path.replace_extension(".sav");
  return path;
}

void AppWindow::power_on() {
  auto codes_path = make_codes_path(pref_path_, rom_name_);
  gg_window_.load_codes(codes_path);

  auto sram_path = make_sram_path(pref_path_, rom_name_);
  nes_.cart().load_sram(sram_path);

  nes_.power_on();
  timer_.reset();
}

void AppWindow::power_off() {
  if (!nes_.is_powered_on()) {
    return;
  }

  auto codes_path = make_codes_path(pref_path_, rom_name_);
  gg_window_.save_codes(codes_path);

  auto sram_path = make_sram_path(pref_path_, rom_name_);
  nes_.cart().save_sram(sram_path);

  nes_.power_off();

  paused_ = false;
}
