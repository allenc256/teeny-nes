#pragma once

#include <filesystem>
#include <string>
#include <vector>

class Nes;

class GameGenieWindow {
public:
  GameGenieWindow(Nes &nes) : nes_(nes) {}

  void render();

  void load_codes(const std::filesystem::path &path);
  void save_codes(const std::filesystem::path &path);

private:
  struct Code {
    bool        enabled;
    std::string code;
    std::string desc;
  };

  void sync_codes();
  void sanitize_code(Code &code);

  Nes              &nes_;
  std::vector<Code> codes_;
  bool              new_enabled_  = true;
  char              new_code_[9]  = {0};
  char              new_desc_[64] = {0};
};
