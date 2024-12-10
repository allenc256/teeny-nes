#include "stats.h"

#include <imgui.h>

void Stats::render() {
  ImGuiIO &io = ImGui::GetIO();
  ImGui::Begin("Stats");
  ImGui::Text(
      "Application average %.3f ms/frame (%.1f FPS)",
      1000.0f / io.Framerate,
      io.Framerate
  );
  ImGui::End();
}
