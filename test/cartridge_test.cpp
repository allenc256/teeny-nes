#include "cartridge.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

TEST(Cartridge, read_cartridge) {
  std::ifstream is("test_data/nestest.nes", std::ios::binary);
  EXPECT_NO_THROW(read_cartridge(is));
}
