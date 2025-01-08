#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

#include "src/emu/cart.h"

TEST(Cart, read_cart) { EXPECT_NO_THROW(read_cart("test_data/nestest.nes")); }
