#pragma once

#include <cstdint>
#include <string_view>

class GameGenieCode {
public:
  GameGenieCode();
  GameGenieCode(std::string_view code);

  const char *code() const { return code_; }
  bool        applies(uint16_t addr, uint8_t x) const;
  uint8_t     value() const { return value_; }

  static bool is_valid_code(std::string_view code);

private:
  void init_length_6(std::string_view code);
  void init_length_8(std::string_view code);

  char     code_[9];
  uint16_t addr_;
  uint8_t  value_;
  uint8_t  compare_;
  bool     compare_enabled_;
};
