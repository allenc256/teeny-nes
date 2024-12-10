#include "stats.h"

#include <cinttypes>
#include <imgui.h>

Stats::Stats(Nes &nes) : nes_(nes) {}

void Stats::render() {
  ImGuiIO &io = ImGui::GetIO();
  ImGui::Begin("Stats");
  ImGui::Text("FPS: %.1f", io.Framerate);
  ImGui::Text("CPU cycles: %" PRIi64, nes_.cpu().cycles());
  ImGui::Text("PPU cycles: %" PRIi64, nes_.ppu().cycles());
  ImGui::Text("PPU frames: %" PRIi64, nes_.ppu().frames());
  ImGui::End();
}
