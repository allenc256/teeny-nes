#include "cart.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

TEST(Cart, read_cart) {
  std::ifstream is("test_data/nestest.nes", std::ios::binary);
  EXPECT_NO_THROW(read_cart(is));
}
