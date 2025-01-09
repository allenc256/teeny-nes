#include <gtest/gtest.h>
#include <string_view>

#include "src/emu/game_genie.h"

TEST(GameGenieCode, decode_6) {
  GameGenieCode code("GOSSIP");
  EXPECT_EQ(std::string_view(code.code()), "GOSSIP");
  EXPECT_EQ(code.value(), 0x14);
  EXPECT_TRUE(code.applies(0xd1dd, 0x00));
  EXPECT_TRUE(code.applies(0xd1dd, 0x01));
  EXPECT_FALSE(code.applies(0xd1de, 0x01));
}

TEST(GameGenieCode, decode_8) {
  GameGenieCode code("ZEXPYGLA");
  EXPECT_EQ(std::string_view(code.code()), "ZEXPYGLA");
  EXPECT_EQ(code.value(), 0x02);
  EXPECT_TRUE(code.applies(0x94A7, 0x03));
  EXPECT_FALSE(code.applies(0x94A7, 0x04));
  EXPECT_FALSE(code.applies(0x94A8, 0x03));
}