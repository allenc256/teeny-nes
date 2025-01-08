#pragma once

#include <string>

class Nes;

class GameGenieWindow {
public:
  GameGenieWindow(Nes &nes) : nes_(nes) {}

  void render();

private:
  Nes &nes_;
  char new_code_[9] = {0};
};
