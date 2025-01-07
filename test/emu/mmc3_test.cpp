#include <format>
#include <fstream>
#include <gtest/gtest.h>
#include <stdexcept>

#include "src/emu/nes.h"

class Labels {
public:
  Labels(std::string path) {
    std::ifstream ifs(path);
    if (!ifs) {
      throw new std::runtime_error(std::format("failed to open file: {}", path)
      );
    }

    std::string line;
    int         addr;
    std::string symbol;
    while (std::getline(ifs, line)) {
      std::istringstream iss(line);
      iss >> symbol >> std::hex >> addr >> symbol;
      if (!iss) {
        throw std::runtime_error(
            std::format("parsing failed for line: {}\n", line)
        );
      }
      labels_[addr] = symbol;
    }
  }

  const std::string *find(uint16_t PC) const {
    auto it = labels_.find(PC);
    if (it != labels_.end()) {
      return &it->second;
    } else {
      return nullptr;
    }
  }

private:
  std::unordered_map<int, std::string> labels_;
};

static void run_test_rom(std::string_view name, int64_t max_cycles) {
  Labels labels(std::format("test_data/{}.labels.txt", name));

  Nes nes;
  nes.load_cart(std::format("test_data/{}.nes", name));
  nes.power_on();

  while (nes.cpu().cycles() < max_cycles) {
    auto label = labels.find(nes.cpu().registers().PC);
    if (label != nullptr) {
      if (*label == ".tests_passed") {
        return;
      } else if (*label == ".test_failed") {
        FAIL() << "A test reported failure.";
      }
    }
    nes.step();
  }
  FAIL() << std::format("Tests failed to pass after {} cycles.", max_cycles);
}

TEST(Mmc3, mmc3_1_clocking) { run_test_rom("mmc3_1_clocking", 1000000); }
TEST(Mmc3, mmc3_2_details) { run_test_rom("mmc3_2_details", 1000000); }

TEST(Mmc3, mmc3_3_a12_clocking) {
  run_test_rom("mmc3_3_a12_clocking", 1000000);
}

TEST(Mmc3, DISABLED_mmc3_4_scanline_timing) {
  run_test_rom("mmc3_4_scanline_timing", 1000000);
}

TEST(Mmc3, mmc3_5_mmc3) { run_test_rom("mmc3_5_mmc3", 1000000); }

TEST(Mmc3, DISABLED_mmc3_6_mmc3_alt) {
  run_test_rom("mmc3_6_mmc3_alt", 1000000);
}
