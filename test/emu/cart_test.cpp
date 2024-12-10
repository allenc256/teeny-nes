#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

#include "src/emu/cart.h"

TEST(Cart, read_cart) {
  std::ifstream is("test_data/nestest.nes", std::ios::binary);
  EXPECT_NO_THROW(read_cart(is));
}
