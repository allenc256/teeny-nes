#include <algorithm>
#include <cctype>
#include <cstddef>
#include <format>
#include <fstream>
#include <imgui.h>

#include "src/app/game_genie_window.h"
#include "src/emu/nes.h"

void GameGenieWindow::render() {
  if (ImGui::Begin("Game Genie Codes", nullptr)) {
    float width1      = ImGui::GetFontSize() * 6;
    float width2      = ImGui::GetFontSize() * 20;
    bool  should_sync = false;
    int   to_delete   = -1;

    if (ImGui::BeginTable("gg_codes", 4, ImGuiTableFlags_SizingFixedFit)) {
      for (std::size_t i = 0; i < codes_.size(); i++) {
        ImGui::TableNextColumn();
        {
          ImGui::PushItemWidth(width1);
          ImGui::Text("%s", codes_[i].code.c_str());
          ImGui::PopItemWidth();
        }
        ImGui::TableNextColumn();
        {
          ImGui::PushItemWidth(width2);
          ImGui::TextWrapped("%s", codes_[i].desc.c_str());
          ImGui::PopItemWidth();
        }
        ImGui::TableNextColumn();
        {
          ImGui::PushID(i);
          bool was_enabled = codes_[i].enabled;
          ImGui::Checkbox("Enabled", &codes_[i].enabled);
          should_sync |= was_enabled ^ codes_[i].enabled;
          ImGui::PopID();
        }
        ImGui::TableNextColumn();
        {
          ImGui::PushID(i);
          if (ImGui::Button("Delete")) {
            to_delete = i;
          }
          ImGui::PopID();
        }
      }

      ImGui::TableNextColumn();
      {
        ImGui::PushItemWidth(width1);
        ImGui::InputTextWithHint(
            "##new_code", "New Code", new_code_, sizeof(new_code_)
        );
        ImGui::PopItemWidth();
      }
      ImGui::TableNextColumn();
      {
        ImGui::PushItemWidth(width2);
        ImGui::InputTextWithHint(
            "##new_desc", "Description", new_desc_, sizeof(new_desc_)
        );
        ImGui::PopItemWidth();
      }
      ImGui::TableNextColumn();
      {
        ImGui::Checkbox("Enabled", &new_enabled_);
      }
      ImGui::TableNextColumn();
      {
        if (ImGui::Button("Add")) {
          if (GameGenieCode::is_valid_code(new_code_)) {
            codes_.push_back(
                {.enabled = new_enabled_, .code = new_code_, .desc = new_desc_}
            );
            sanitize_code(codes_.back());
            new_code_[0] = 0;
            new_desc_[0] = 0;
            should_sync  = true;
          }
        }
      }

      ImGui::EndTable();
    }

    if (to_delete >= 0) {
      codes_.erase(codes_.begin() + to_delete);
      should_sync = true;
    }

    if (should_sync) {
      sync_codes();
    }
  }
  ImGui::End();
}

void GameGenieWindow::sanitize_code(Code &code) {
  for (std::size_t i = 0; i < code.code.size(); i++) {
    code.code[i] = std::toupper(code.code[i]);
  }

  // Remove newlines from description.
  code.desc.erase(
      std::remove(code.desc.begin(), code.desc.end(), '\n'), code.desc.end()
  );
}

void GameGenieWindow::sync_codes() {
  nes_.cart().clear_gg_codes();
  for (auto &code : codes_) {
    if (code.enabled) {
      nes_.cart().add_gg_code(code.code);
    }
  }
}

void GameGenieWindow::save_codes(const std::filesystem::path &path) {
  if (codes_.empty()) {
    if (std::filesystem::exists(path) && !std::filesystem::remove(path)) {
      throw std::runtime_error(
          std::format("failed to delete file: {}", path.string())
      );
    }
    return;
  }

  std::ofstream ofs(path);
  if (!ofs) {
    throw std::runtime_error(
        std::format("failed to open file for writing: {}", path.string())
    );
  }

  for (auto &code : codes_) {
    ofs << std::format(
        "{:<10} {:<10} {}\n",
        code.code,
        code.enabled ? "enabled" : "disabled",
        code.desc
    );
  }
}

void GameGenieWindow::load_codes(const std::filesystem::path &path) {
  codes_.clear();

  if (!std::filesystem::exists(path)) {
    return;
  }

  std::ifstream ifs(path);
  if (!ifs) {
    throw std::runtime_error(
        std::format("failed to open file for reading: {}", path.string())
    );
  }

  std::string code, enabled, desc;
  while (true) {
    ifs >> code >> enabled >> std::ws;
    std::getline(ifs, desc);
    if (ifs) {
      codes_.push_back(
          {.enabled = enabled == "enabled", .code = code, .desc = desc}
      );
    } else {
      break;
    }
  }

  sync_codes();
}
