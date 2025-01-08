#include <cstddef>
#include <format>
#include <imgui.h>

#include "src/app/game_genie_window.h"
#include "src/emu/nes.h"

void GameGenieWindow::render() {
  if (ImGui::Begin("Game Genie Codes", nullptr)) {
    auto &codes     = nes_.cart().gg_codes();
    float width     = ImGui::GetFontSize() * 10;
    float spacing   = ImGui::GetFontSize() * 11;
    int   to_delete = -1;

    for (std::size_t i = 0; i < codes.size(); i++) {
      ImGui::PushItemWidth(width);
      ImGui::Text("%s", codes[i].code());
      ImGui::PopItemWidth();
      ImGui::SameLine(spacing);
      ImGui::PushID(i);
      if (ImGui::Button("Delete")) {
        to_delete = i;
      }
      ImGui::PopID();
    }

    ImGui::PushItemWidth(width);
    ImGui::InputTextWithHint(
        "##new_code", "New Code", new_code_, sizeof(new_code_)
    );
    ImGui::PopItemWidth();
    ImGui::SameLine(spacing);
    if (ImGui::Button("Add")) {
      if (GameGenieCode::is_valid_code(new_code_)) {
        codes.emplace_back(new_code_);
        new_code_[0] = 0;
      }
    }

    if (to_delete >= 0) {
      codes.erase(codes.begin() + to_delete);
    }
  }
  ImGui::End();
}
