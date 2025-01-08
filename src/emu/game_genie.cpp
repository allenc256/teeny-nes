#include <cctype>
#include <format>
#include <stdexcept>

#include "src/emu/game_genie.h"

// Reference: https://tuxnes.sourceforge.net/gamegenie.html

static constexpr const char *GG_DECODE = "APZLGITYEOXUKSVN";
static constexpr int         INVALID   = -1;

static int decode_gg_char(char ch) {
  for (int i = 0; i < 16; i++) {
    if (ch == GG_DECODE[i]) {
      return i;
    }
  }
  return INVALID;
}

GameGenieCode::GameGenieCode()
    : code_{0},
      addr_(0),
      value_(0),
      compare_(0),
      compare_enabled_(false) {}

GameGenieCode::GameGenieCode(std::string_view code) {
  if (code.length() == 6) {
    init_length_6(code);
  } else if (code.length() == 8) {
    init_length_8(code);
  } else {
    throw std::invalid_argument(
        std::format("invalid game genie code: {} (length must be 6 or 8)", code)
    );
  }
}

void GameGenieCode::init_length_6(std::string_view code) {
  int n[6];
  for (int i = 0; i < 6; i++) {
    char ch  = std::toupper(code[i]);
    code_[i] = ch;
    n[i]     = decode_gg_char(ch);
    if (n[i] == INVALID) {
      throw std::invalid_argument(
          std::format("invalid game genie code: {} (position {})", code, i)
      );
    }
  }

  code_[6] = 0;
  addr_    = (uint16_t)(0x8000 + (((n[3] & 7) << 12) | ((n[5] & 7) << 8) |
                               ((n[4] & 8) << 8) | ((n[2] & 7) << 4) |
                               ((n[1] & 8) << 4) | (n[4] & 7) | (n[3] & 8)));
  value_   = (uint8_t)(((n[1] & 7) << 4) | ((n[0] & 8) << 4) | (n[0] & 7) |
                     (n[5] & 8));
  compare_ = 0;
  compare_enabled_ = false;
}

void GameGenieCode::init_length_8(std::string_view code) {
  int n[8];
  for (int i = 0; i < 8; i++) {
    char ch  = std::toupper(code[i]);
    code_[i] = ch;
    n[i]     = decode_gg_char(ch);
    if (n[i] == INVALID) {
      throw std::invalid_argument(
          std::format("invalid game genie code: {} (position {})", code, i)
      );
    }
  }

  code_[8] = 0;
  addr_    = (uint16_t)(0x8000 + (((n[3] & 7) << 12) | ((n[5] & 7) << 8) |
                               ((n[4] & 8) << 8) | ((n[2] & 7) << 4) |
                               ((n[1] & 8) << 4) | (n[4] & 7) | (n[3] & 8)));
  value_   = (uint8_t)(((n[1] & 7) << 4) | ((n[0] & 8) << 4) | (n[0] & 7) |
                     (n[7] & 8));
  compare_ = (uint8_t)(((n[7] & 7) << 4) | ((n[6] & 8) << 4) | (n[6] & 7) |
                       (n[5] & 8));
  compare_enabled_ = true;
}

bool GameGenieCode::applies(uint16_t addr, uint8_t x) const {
  return addr_ == addr && (!compare_enabled_ || compare_ == x);
}

bool GameGenieCode::is_valid_code(std::string_view code) {
  if (code.length() != 6 && code.length() != 8) {
    return false;
  }
  for (auto ch : code) {
    if (decode_gg_char(std::toupper(ch)) == INVALID) {
      return false;
    }
  }
  return true;
}
