#include <cstring>
#include <format>

#include "src/emu/apu.h"
#include "src/emu/cart.h"
#include "src/emu/cpu.h"
#include "src/emu/input.h"
#include "src/emu/ppu.h"

static constexpr std::array<Cpu::OpCode, 256> init_op_codes() {
  using enum Cpu::Instruction;
  using enum Cpu::AddrMode;
  using enum Cpu::Flags;
  using enum Cpu::OpCodeFlags;

  std::array<Cpu::OpCode, 256> op_code;

  for (int i = 0; i < 256; i++) {
    op_code[i].code = (uint8_t)i;
  }

  op_code[0x69] = {0x69, ADC, IMMEDIATE, 2, 2};
  op_code[0x65] = {0x65, ADC, ZERO_PAGE, 2, 3};
  op_code[0x75] = {0x75, ADC, ZERO_PAGE_X, 2, 4};
  op_code[0x6d] = {0x6d, ADC, ABSOLUTE, 3, 4};
  op_code[0x7d] = {0x7d, ADC, ABSOLUTE_X, 3, 4};
  op_code[0x79] = {0x79, ADC, ABSOLUTE_Y, 3, 4};
  op_code[0x61] = {0x61, ADC, INDIRECT_X, 2, 6};
  op_code[0x71] = {0x71, ADC, INDIRECT_Y, 2, 5};

  op_code[0x29] = {0x29, AND, IMMEDIATE, 2, 2};
  op_code[0x25] = {0x25, AND, ZERO_PAGE, 2, 3};
  op_code[0x35] = {0x35, AND, ZERO_PAGE_X, 2, 4};
  op_code[0x2d] = {0x2d, AND, ABSOLUTE, 3, 4};
  op_code[0x3d] = {0x3d, AND, ABSOLUTE_X, 3, 4};
  op_code[0x39] = {0x39, AND, ABSOLUTE_Y, 3, 4};
  op_code[0x21] = {0x21, AND, INDIRECT_X, 2, 6};
  op_code[0x31] = {0x31, AND, INDIRECT_Y, 2, 5};

  op_code[0x0a] = {0x0a, ASL, ACCUMULATOR, 1, 2, FORCE_OOPS};
  op_code[0x06] = {0x06, ASL, ZERO_PAGE, 2, 5, FORCE_OOPS};
  op_code[0x16] = {0x16, ASL, ZERO_PAGE_X, 2, 6, FORCE_OOPS};
  op_code[0x0e] = {0x0e, ASL, ABSOLUTE, 3, 6, FORCE_OOPS};
  op_code[0x1e] = {0x1e, ASL, ABSOLUTE_X, 3, 6, FORCE_OOPS};

  op_code[0x90] = {0x90, BCC, RELATIVE, 2, 2};

  op_code[0xb0] = {0xb0, BCS, RELATIVE, 2, 2};

  op_code[0xf0] = {0xf0, BEQ, RELATIVE, 2, 2};

  op_code[0x24] = {0x24, BIT, ZERO_PAGE, 2, 3};
  op_code[0x2c] = {0x2c, BIT, ABSOLUTE, 3, 4};

  op_code[0x30] = {0x30, BMI, RELATIVE, 2, 2};

  op_code[0xd0] = {0xd0, BNE, RELATIVE, 2, 2};

  op_code[0x10] = {0x10, BPL, RELATIVE, 2, 2};

  op_code[0x00] = {0x00, BRK, IMPLICIT, 2, 7};

  op_code[0x50] = {0x50, BVC, RELATIVE, 2, 2};

  op_code[0x70] = {0x70, BVS, RELATIVE, 2, 2};

  op_code[0x18] = {0x18, CLC, IMPLICIT, 1, 2};

  op_code[0xd8] = {0xd8, CLD, IMPLICIT, 1, 2};

  op_code[0x58] = {0x58, CLI, IMPLICIT, 1, 2};

  op_code[0xb8] = {0xb8, CLV, IMPLICIT, 1, 2};

  op_code[0xc9] = {0xc9, CMP, IMMEDIATE, 2, 2};
  op_code[0xc5] = {0xc5, CMP, ZERO_PAGE, 2, 3};
  op_code[0xd5] = {0xd5, CMP, ZERO_PAGE_X, 2, 4};
  op_code[0xcd] = {0xcd, CMP, ABSOLUTE, 3, 4};
  op_code[0xdd] = {0xdd, CMP, ABSOLUTE_X, 3, 4};
  op_code[0xd9] = {0xd9, CMP, ABSOLUTE_Y, 3, 4};
  op_code[0xc1] = {0xc1, CMP, INDIRECT_X, 2, 6};
  op_code[0xd1] = {0xd1, CMP, INDIRECT_Y, 2, 5};

  op_code[0xe0] = {0xe0, CPX, IMMEDIATE, 2, 2};
  op_code[0xe4] = {0xe4, CPX, ZERO_PAGE, 2, 3};
  op_code[0xec] = {0xec, CPX, ABSOLUTE, 3, 4};

  op_code[0xc0] = {0xc0, CPY, IMMEDIATE, 2, 2};
  op_code[0xc4] = {0xc4, CPY, ZERO_PAGE, 2, 3};
  op_code[0xcc] = {0xcc, CPY, ABSOLUTE, 3, 4};

  op_code[0xc6] = {0xc6, DEC, ZERO_PAGE, 2, 5, FORCE_OOPS};
  op_code[0xd6] = {0xd6, DEC, ZERO_PAGE_X, 2, 6, FORCE_OOPS};
  op_code[0xce] = {0xce, DEC, ABSOLUTE, 3, 6, FORCE_OOPS};
  op_code[0xde] = {0xde, DEC, ABSOLUTE_X, 3, 6, FORCE_OOPS};

  op_code[0xca] = {0xca, DEX, IMPLICIT, 1, 2};

  op_code[0x88] = {0x88, DEY, IMPLICIT, 1, 2};

  op_code[0x49] = {0x49, EOR, IMMEDIATE, 2, 2};
  op_code[0x45] = {0x45, EOR, ZERO_PAGE, 2, 3};
  op_code[0x55] = {0x55, EOR, ZERO_PAGE_X, 2, 4};
  op_code[0x4d] = {0x4d, EOR, ABSOLUTE, 3, 4};
  op_code[0x5d] = {0x5d, EOR, ABSOLUTE_X, 3, 4};
  op_code[0x59] = {0x59, EOR, ABSOLUTE_Y, 3, 4};
  op_code[0x41] = {0x41, EOR, INDIRECT_X, 2, 6};
  op_code[0x51] = {0x51, EOR, INDIRECT_Y, 2, 5};

  op_code[0xe6] = {0xe6, INC, ZERO_PAGE, 2, 5, FORCE_OOPS};
  op_code[0xf6] = {0xf6, INC, ZERO_PAGE_X, 2, 6, FORCE_OOPS};
  op_code[0xee] = {0xee, INC, ABSOLUTE, 3, 6, FORCE_OOPS};
  op_code[0xfe] = {0xfe, INC, ABSOLUTE_X, 3, 6, FORCE_OOPS};

  op_code[0xe8] = {0xe8, INX, IMPLICIT, 1, 2};

  op_code[0xc8] = {0xc8, INY, IMPLICIT, 1, 2};

  op_code[0x4c] = {0x4c, JMP, ABSOLUTE, 3, 3};
  op_code[0x6c] = {0x6c, JMP, INDIRECT, 3, 5};

  op_code[0x20] = {0x20, JSR, ABSOLUTE, 3, 6};

  op_code[0xa9] = {0xa9, LDA, IMMEDIATE, 2, 2};
  op_code[0xa5] = {0xa5, LDA, ZERO_PAGE, 2, 3};
  op_code[0xb5] = {0xb5, LDA, ZERO_PAGE_X, 2, 4};
  op_code[0xad] = {0xad, LDA, ABSOLUTE, 3, 4};
  op_code[0xbd] = {0xbd, LDA, ABSOLUTE_X, 3, 4};
  op_code[0xb9] = {0xb9, LDA, ABSOLUTE_Y, 3, 4};
  op_code[0xa1] = {0xa1, LDA, INDIRECT_X, 2, 6};
  op_code[0xb1] = {0xb1, LDA, INDIRECT_Y, 2, 5};

  op_code[0xa2] = {0xa2, LDX, IMMEDIATE, 2, 2};
  op_code[0xa6] = {0xa6, LDX, ZERO_PAGE, 2, 3};
  op_code[0xb6] = {0xb6, LDX, ZERO_PAGE_Y, 2, 4};
  op_code[0xae] = {0xae, LDX, ABSOLUTE, 3, 4};
  op_code[0xbe] = {0xbe, LDX, ABSOLUTE_Y, 3, 4};

  op_code[0xa0] = {0xa0, LDY, IMMEDIATE, 2, 2};
  op_code[0xa4] = {0xa4, LDY, ZERO_PAGE, 2, 3};
  op_code[0xb4] = {0xb4, LDY, ZERO_PAGE_X, 2, 4};
  op_code[0xac] = {0xac, LDY, ABSOLUTE, 3, 4};
  op_code[0xbc] = {0xbc, LDY, ABSOLUTE_X, 3, 4};

  op_code[0x4a] = {0x4a, LSR, ACCUMULATOR, 1, 2, FORCE_OOPS};
  op_code[0x46] = {0x46, LSR, ZERO_PAGE, 2, 5, FORCE_OOPS};
  op_code[0x56] = {0x56, LSR, ZERO_PAGE_X, 2, 6, FORCE_OOPS};
  op_code[0x4e] = {0x4e, LSR, ABSOLUTE, 3, 6, FORCE_OOPS};
  op_code[0x5e] = {0x5e, LSR, ABSOLUTE_X, 3, 6, FORCE_OOPS};

  op_code[0xea] = {0xea, NOP, IMPLICIT, 1, 2};

  op_code[0x09] = {0x09, ORA, IMMEDIATE, 2, 2};
  op_code[0x05] = {0x05, ORA, ZERO_PAGE, 2, 3};
  op_code[0x15] = {0x15, ORA, ZERO_PAGE_X, 2, 4};
  op_code[0x0d] = {0x0d, ORA, ABSOLUTE, 3, 4};
  op_code[0x1d] = {0x1d, ORA, ABSOLUTE_X, 3, 4};
  op_code[0x19] = {0x19, ORA, ABSOLUTE_Y, 3, 4};
  op_code[0x01] = {0x01, ORA, INDIRECT_X, 2, 6};
  op_code[0x11] = {0x11, ORA, INDIRECT_Y, 2, 5};

  op_code[0x48] = {0x48, PHA, IMPLICIT, 1, 3};

  op_code[0x08] = {0x08, PHP, IMPLICIT, 1, 3};

  op_code[0x68] = {0x68, PLA, IMPLICIT, 1, 4};

  op_code[0x28] = {0x28, PLP, IMPLICIT, 1, 4};

  op_code[0x2a] = {0x2a, ROL, ACCUMULATOR, 1, 2, FORCE_OOPS};
  op_code[0x26] = {0x26, ROL, ZERO_PAGE, 2, 5, FORCE_OOPS};
  op_code[0x36] = {0x36, ROL, ZERO_PAGE_X, 2, 6, FORCE_OOPS};
  op_code[0x2e] = {0x2e, ROL, ABSOLUTE, 3, 6, FORCE_OOPS};
  op_code[0x3e] = {0x3e, ROL, ABSOLUTE_X, 3, 6, FORCE_OOPS};

  op_code[0x6a] = {0x6a, ROR, ACCUMULATOR, 1, 2, FORCE_OOPS};
  op_code[0x66] = {0x66, ROR, ZERO_PAGE, 2, 5, FORCE_OOPS};
  op_code[0x76] = {0x76, ROR, ZERO_PAGE_X, 2, 6, FORCE_OOPS};
  op_code[0x6e] = {0x6e, ROR, ABSOLUTE, 3, 6, FORCE_OOPS};
  op_code[0x7e] = {0x7e, ROR, ABSOLUTE_X, 3, 6, FORCE_OOPS};

  op_code[0x40] = {0x40, RTI, IMPLICIT, 1, 6};

  op_code[0x60] = {0x60, RTS, IMPLICIT, 1, 6};

  op_code[0xe9] = {0xe9, SBC, IMMEDIATE, 2, 2};
  op_code[0xe5] = {0xe5, SBC, ZERO_PAGE, 2, 3};
  op_code[0xf5] = {0xf5, SBC, ZERO_PAGE_X, 2, 4};
  op_code[0xed] = {0xed, SBC, ABSOLUTE, 3, 4};
  op_code[0xfd] = {0xfd, SBC, ABSOLUTE_X, 3, 4};
  op_code[0xf9] = {0xf9, SBC, ABSOLUTE_Y, 3, 4};
  op_code[0xe1] = {0xe1, SBC, INDIRECT_X, 2, 6};
  op_code[0xf1] = {0xf1, SBC, INDIRECT_Y, 2, 5};

  op_code[0x38] = {0x38, SEC, IMPLICIT, 1, 2};

  op_code[0xf8] = {0xf8, SED, IMPLICIT, 1, 2};

  op_code[0x78] = {0x78, SEI, IMPLICIT, 1, 2};

  op_code[0x85] = {0x85, STA, ZERO_PAGE, 2, 3, FORCE_OOPS};
  op_code[0x95] = {0x95, STA, ZERO_PAGE_X, 2, 4, FORCE_OOPS};
  op_code[0x8d] = {0x8d, STA, ABSOLUTE, 3, 4, FORCE_OOPS};
  op_code[0x9d] = {0x9d, STA, ABSOLUTE_X, 3, 4, FORCE_OOPS};
  op_code[0x99] = {0x99, STA, ABSOLUTE_Y, 3, 4, FORCE_OOPS};
  op_code[0x81] = {0x81, STA, INDIRECT_X, 2, 6, FORCE_OOPS};
  op_code[0x91] = {0x91, STA, INDIRECT_Y, 2, 5, FORCE_OOPS};

  op_code[0x86] = {0x86, STX, ZERO_PAGE, 2, 3, FORCE_OOPS};
  op_code[0x96] = {0x96, STX, ZERO_PAGE_Y, 2, 4, FORCE_OOPS};
  op_code[0x8e] = {0x8e, STX, ABSOLUTE, 3, 4, FORCE_OOPS};

  op_code[0x84] = {0x84, STY, ZERO_PAGE, 2, 3, FORCE_OOPS};
  op_code[0x94] = {0x94, STY, ZERO_PAGE_X, 2, 4, FORCE_OOPS};
  op_code[0x8c] = {0x8c, STY, ABSOLUTE, 3, 4, FORCE_OOPS};

  op_code[0xaa] = {0xaa, TAX, IMPLICIT, 1, 2};

  op_code[0xa8] = {0xa8, TAY, IMPLICIT, 1, 2};

  op_code[0xba] = {0xba, TSX, IMPLICIT, 1, 2};

  op_code[0x8a] = {0x8a, TXA, IMPLICIT, 1, 2};

  op_code[0x9a] = {0x9a, TXS, IMPLICIT, 1, 2};

  op_code[0x98] = {0x98, TYA, IMPLICIT, 1, 2};

  op_code[0x1a] = {0x1a, NOP, IMPLICIT, 1, 2, ILLEGAL};
  op_code[0x3a] = {0x3a, NOP, IMPLICIT, 1, 2, ILLEGAL};
  op_code[0x5a] = {0x5a, NOP, IMPLICIT, 1, 2, ILLEGAL};
  op_code[0x7a] = {0x7a, NOP, IMPLICIT, 1, 2, ILLEGAL};
  op_code[0xda] = {0xda, NOP, IMPLICIT, 1, 2, ILLEGAL};
  op_code[0xfa] = {0xfa, NOP, IMPLICIT, 1, 2, ILLEGAL};

  op_code[0x80] = {0x80, NOP, IMMEDIATE, 2, 2, ILLEGAL};
  op_code[0x82] = {0x82, NOP, IMMEDIATE, 2, 2, ILLEGAL};
  op_code[0x89] = {0x89, NOP, IMMEDIATE, 2, 2, ILLEGAL};
  op_code[0xc2] = {0xc2, NOP, IMMEDIATE, 2, 2, ILLEGAL};
  op_code[0xe2] = {0xe2, NOP, IMMEDIATE, 2, 2, ILLEGAL};

  op_code[0x04] = {0x04, NOP, ZERO_PAGE, 2, 3, ILLEGAL};
  op_code[0x44] = {0x44, NOP, ZERO_PAGE, 2, 3, ILLEGAL};
  op_code[0x64] = {0x64, NOP, ZERO_PAGE, 2, 3, ILLEGAL};

  op_code[0x0c] = {0x0c, NOP, ABSOLUTE, 3, 4, ILLEGAL};

  op_code[0x14] = {0x14, NOP, ZERO_PAGE_X, 2, 4, ILLEGAL};
  op_code[0x34] = {0x34, NOP, ZERO_PAGE_X, 2, 4, ILLEGAL};
  op_code[0x54] = {0x54, NOP, ZERO_PAGE_X, 2, 4, ILLEGAL};
  op_code[0x74] = {0x74, NOP, ZERO_PAGE_X, 2, 4, ILLEGAL};
  op_code[0xd4] = {0xd4, NOP, ZERO_PAGE_X, 2, 4, ILLEGAL};
  op_code[0xf4] = {0xf4, NOP, ZERO_PAGE_X, 2, 4, ILLEGAL};

  op_code[0x1c] = {0x1c, NOP, ABSOLUTE_X, 3, 4, ILLEGAL};
  op_code[0x3c] = {0x3c, NOP, ABSOLUTE_X, 3, 4, ILLEGAL};
  op_code[0x5c] = {0x5c, NOP, ABSOLUTE_X, 3, 4, ILLEGAL};
  op_code[0x7c] = {0x7c, NOP, ABSOLUTE_X, 3, 4, ILLEGAL};
  op_code[0xdc] = {0xdc, NOP, ABSOLUTE_X, 3, 4, ILLEGAL};
  op_code[0xfc] = {0xfc, NOP, ABSOLUTE_X, 3, 4, ILLEGAL};

  op_code[0xa7] = {0xa7, LAX, ZERO_PAGE, 2, 3, ILLEGAL};
  op_code[0xb7] = {0xb7, LAX, ZERO_PAGE_Y, 2, 4, ILLEGAL};
  op_code[0xaf] = {0xaf, LAX, ABSOLUTE, 3, 4, ILLEGAL};
  op_code[0xbf] = {0xbf, LAX, ABSOLUTE_Y, 3, 4, ILLEGAL};
  op_code[0xa3] = {0xa3, LAX, INDIRECT_X, 2, 6, ILLEGAL};
  op_code[0xb3] = {0xb3, LAX, INDIRECT_Y, 2, 5, ILLEGAL};

  op_code[0x87] = {0x87, SAX, ZERO_PAGE, 2, 3, ILLEGAL | FORCE_OOPS};
  op_code[0x97] = {0x97, SAX, ZERO_PAGE_Y, 2, 4, ILLEGAL | FORCE_OOPS};
  op_code[0x8f] = {0x8f, SAX, ABSOLUTE, 3, 4, ILLEGAL | FORCE_OOPS};
  op_code[0x83] = {0x83, SAX, INDIRECT_X, 2, 6, ILLEGAL | FORCE_OOPS};

  op_code[0xeb] = {0xeb, SBC, IMMEDIATE, 2, 2, ILLEGAL};

  op_code[0xc7] = {0xc7, DCP, ZERO_PAGE, 2, 5, ILLEGAL | FORCE_OOPS};
  op_code[0xd7] = {0xd7, DCP, ZERO_PAGE_X, 2, 6, ILLEGAL | FORCE_OOPS};
  op_code[0xcf] = {0xcf, DCP, ABSOLUTE, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0xdf] = {0xdf, DCP, ABSOLUTE_X, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0xdb] = {0xdb, DCP, ABSOLUTE_Y, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0xc3] = {0xc3, DCP, INDIRECT_X, 2, 8, ILLEGAL | FORCE_OOPS};
  op_code[0xd3] = {0xd3, DCP, INDIRECT_Y, 2, 7, ILLEGAL | FORCE_OOPS};

  op_code[0xe7] = {0xe7, ISB, ZERO_PAGE, 2, 5, ILLEGAL | FORCE_OOPS};
  op_code[0xf7] = {0xf7, ISB, ZERO_PAGE_X, 2, 6, ILLEGAL | FORCE_OOPS};
  op_code[0xef] = {0xef, ISB, ABSOLUTE, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0xff] = {0xff, ISB, ABSOLUTE_X, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0xfb] = {0xfb, ISB, ABSOLUTE_Y, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0xe3] = {0xe3, ISB, INDIRECT_X, 2, 8, ILLEGAL | FORCE_OOPS};
  op_code[0xf3] = {0xf3, ISB, INDIRECT_Y, 2, 7, ILLEGAL | FORCE_OOPS};

  op_code[0x07] = {0x07, SLO, ZERO_PAGE, 2, 5, ILLEGAL | FORCE_OOPS};
  op_code[0x17] = {0x17, SLO, ZERO_PAGE_X, 2, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x0f] = {0x0f, SLO, ABSOLUTE, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x1f] = {0x1f, SLO, ABSOLUTE_X, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x1b] = {0x1b, SLO, ABSOLUTE_Y, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x03] = {0x03, SLO, INDIRECT_X, 2, 8, ILLEGAL | FORCE_OOPS};
  op_code[0x13] = {0x13, SLO, INDIRECT_Y, 2, 7, ILLEGAL | FORCE_OOPS};

  op_code[0x27] = {0x27, RLA, ZERO_PAGE, 2, 5, ILLEGAL | FORCE_OOPS};
  op_code[0x37] = {0x37, RLA, ZERO_PAGE_X, 2, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x2f] = {0x2f, RLA, ABSOLUTE, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x3f] = {0x3f, RLA, ABSOLUTE_X, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x3b] = {0x3b, RLA, ABSOLUTE_Y, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x23] = {0x23, RLA, INDIRECT_X, 2, 8, ILLEGAL | FORCE_OOPS};
  op_code[0x33] = {0x33, RLA, INDIRECT_Y, 2, 7, ILLEGAL | FORCE_OOPS};

  op_code[0x47] = {0x47, SRE, ZERO_PAGE, 2, 5, ILLEGAL | FORCE_OOPS};
  op_code[0x57] = {0x57, SRE, ZERO_PAGE_X, 2, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x4f] = {0x4f, SRE, ABSOLUTE, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x5f] = {0x5f, SRE, ABSOLUTE_X, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x5b] = {0x5b, SRE, ABSOLUTE_Y, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x43] = {0x43, SRE, INDIRECT_X, 2, 8, ILLEGAL | FORCE_OOPS};
  op_code[0x53] = {0x53, SRE, INDIRECT_Y, 2, 7, ILLEGAL | FORCE_OOPS};

  op_code[0x67] = {0x67, RRA, ZERO_PAGE, 2, 5, ILLEGAL | FORCE_OOPS};
  op_code[0x77] = {0x77, RRA, ZERO_PAGE_X, 2, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x6f] = {0x6f, RRA, ABSOLUTE, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x7f] = {0x7f, RRA, ABSOLUTE_X, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x7b] = {0x7b, RRA, ABSOLUTE_Y, 3, 6, ILLEGAL | FORCE_OOPS};
  op_code[0x63] = {0x63, RRA, INDIRECT_X, 2, 8, ILLEGAL | FORCE_OOPS};
  op_code[0x73] = {0x73, RRA, INDIRECT_Y, 2, 7, ILLEGAL | FORCE_OOPS};

  return op_code;
}

static constexpr std::array<Cpu::OpCode, 256> OP_CODES = init_op_codes();

static constexpr std::string_view INSTRUCTION_NAMES[] = {
    "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRA",
    "BRK", "BVC", "BVS", "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", "CPY", "DCP",
    "DEC", "DEX", "DEY", "EOR", "INC", "INX", "INY", "ISB", "JMP", "JSR", "LAX",
    "LDA", "LDX", "LDY", "LSR", "NOP", "ORA", "PHA", "PHP", "PHX", "PHY", "PLA",
    "PLP", "PLX", "PLY", "RLA", "ROL", "ROR", "RRA", "RTI", "RTS", "SAX", "SBC",
    "SEC", "SED", "SEI", "SLO", "SRE", "STA", "STX", "STY", "STZ", "TAX", "TAY",
    "TRB", "TSB", "TSX", "TXA", "TXS", "TYA", "INV"
};

static constexpr std::string_view ADDR_MODE_NAMES[] = {
    "ABSOLUTE",
    "ABSOLUTE_X",
    "ABSOLUTE_Y",
    "ACCUMULATOR",
    "IMMEDIATE",
    "IMPLICIT",
    "INDIRECT",
    "INDIRECT_X",
    "INDIRECT_Y",
    "RELATIVE",
    "ZERO_PAGE",
    "ZERO_PAGE_X",
    "ZERO_PAGE_Y",
    "INVALID",
};

Cpu::Cpu()
    : cart_(nullptr),
      ppu_(nullptr),
      apu_(nullptr),
      oops_(false),
      jump_(false),
      test_ram_(nullptr) {}

void Cpu::power_on() {
  regs_.A          = 0;
  regs_.X          = 0;
  regs_.Y          = 0;
  regs_.PC         = peek16(RESET_VECTOR);
  regs_.S          = 0xfd;
  regs_.P          = I_FLAG | DUMMY_FLAG;
  nmi_pending_     = false;
  irq_pending_     = 0;
  oam_dma_pending_ = false;
  cycles_          = RESET_CYCLES;
  std::memset(ram_, 0, sizeof(ram_));
}

void Cpu::reset() {
  regs_.PC = peek16(RESET_VECTOR);
  regs_.S -= 3;
  regs_.P |= I_FLAG;
  nmi_pending_     = false;
  irq_pending_     = 0;
  oam_dma_pending_ = false;
  cycles_          = RESET_CYCLES;
}

static constexpr uint8_t open_bus() { return 0; }

uint8_t Cpu::peek(uint16_t addr) {
  if (test_ram_) {
    return test_ram_[addr];
  } else if (addr < RAM_END) {
    return ram_[addr & RAM_MASK];
  } else if (addr < PPU_REGS_END) {
    switch (addr & 0x2007) {
    case PPU_PPUCTRL: return ppu_->read_PPUCTRL();
    case PPU_PPUMASK: return ppu_->read_PPUMASK();
    case PPU_PPUSTATUS: return ppu_->read_PPUSTATUS();
    case PPU_OAMADDR: return ppu_->read_OAMADDR();
    case PPU_OAMDATA: return ppu_->read_OAMDATA();
    case PPU_PPUSCROLL: return ppu_->read_PPUSCROLL();
    case PPU_PPUADDR: return ppu_->read_PPUADDR();
    case PPU_PPUDATA: return ppu_->read_PPUDATA();
    default: throw std::runtime_error("unreachable");
    }
  } else if (addr >= Cart::CPU_ADDR_START) {
    return cart_->peek_cpu(addr);
  } else {
    switch (addr) {
    case 0x4015: return apu_->read_4015();
    case IO_JOY1: return input_->read_controller(0);
    case IO_JOY2: return input_->read_controller(1);
    case PPU_OAMDMA: return ppu_->read_OAMDMA();
    default: return open_bus();
    }
  }
}

uint16_t Cpu::peek16(uint16_t addr) {
  uint8_t lo = peek(addr);
  uint8_t hi = peek(addr + 1);
  return (uint16_t)(lo + (hi << 8));
}

void Cpu::poke(uint16_t addr, uint8_t x) {
  if (test_ram_) {
    test_ram_[addr] = x;
  } else if (addr < RAM_END) {
    ram_[addr & RAM_MASK] = x;
  } else if (addr < PPU_REGS_END) {
    switch (addr & 0x2007) {
    case PPU_PPUCTRL: ppu_->write_PPUCTRL(x); break;
    case PPU_PPUMASK: ppu_->write_PPUMASK(x); break;
    case PPU_PPUSTATUS: ppu_->write_PPUSTATUS(x); break;
    case PPU_OAMADDR: ppu_->write_OAMADDR(x); break;
    case PPU_OAMDATA: ppu_->write_OAMDATA(x); break;
    case PPU_PPUSCROLL: ppu_->write_PPUSCROLL(x); break;
    case PPU_PPUADDR: ppu_->write_PPUADDR(x); break;
    case PPU_PPUDATA: ppu_->write_PPUDATA(x); break;
    default: throw std::runtime_error("unreachable");
    }
  } else if (addr >= Cart::CPU_ADDR_START) {
    cart_->poke_cpu(addr, x);
  } else {
    switch (addr) {
    case IO_JOY1: input_->write_controller(x); break;
    case 0x4000: apu_->write_4000(x); break;
    case 0x4001: apu_->write_4001(x); break;
    case 0x4002: apu_->write_4002(x); break;
    case 0x4003: apu_->write_4003(x); break;
    case 0x4004: apu_->write_4004(x); break;
    case 0x4005: apu_->write_4005(x); break;
    case 0x4006: apu_->write_4006(x); break;
    case 0x4007: apu_->write_4007(x); break;
    case 0x4008: apu_->write_4008(x); break;
    case 0x400a: apu_->write_400A(x); break;
    case 0x400b: apu_->write_400B(x); break;
    case 0x400c: apu_->write_400C(x); break;
    case 0x400e: apu_->write_400E(x); break;
    case 0x400f: apu_->write_400F(x); break;
    case 0x4015: apu_->write_4015(x); break;
    case 0x4017: apu_->write_4017(x); break;
    case PPU_OAMDMA: {
      ppu_->write_OAMDMA(x);
      oam_dma_pending_ = true;
      break;
    }
    default: break; // unmapped
    }
  }
}

void Cpu::push(uint8_t x) {
  poke(STACK_START + regs_.S, x);
  regs_.S--;
}

uint8_t Cpu::pop() {
  regs_.S++;
  return peek(STACK_START + regs_.S);
}

void Cpu::push16(uint16_t x) {
  uint8_t hi = (uint8_t)(x >> 8);
  uint8_t lo = (uint8_t)x;
  push(hi);
  push(lo);
}

uint16_t Cpu::pop16() {
  uint8_t lo = pop();
  uint8_t hi = pop();
  return (uint16_t)(lo + (hi << 8));
}

void Cpu::step() {
  if (oam_dma_pending_) {
    step_OAM_DMA();
    // nesdev says OAM DMA takes 513 cycles (+1 on odd cpu cycles).
    cycles_ += 513 + (cycles_ & 1);
    oam_dma_pending_ = false;
    return;
  }

  if (nmi_pending_) {
    step_NMI();
    cycles_ += NMI_CYCLES;
    nmi_pending_ = false;
    return;
  }

  if (irq_pending_ && !get_flag(I_FLAG)) {
    // TODO: nesdev says the effect of clearing this flag is delayed 1 cycle for
    // certain instructions (SEI, CLI, or PLP).
    step_IRQ();
    cycles_ += IRQ_CYCLES;
    irq_pending_ = 0;
    return;
  }

  const OpCode &op = OP_CODES[peek(regs_.PC)];

  jump_ = false;
  oops_ = false;

  switch (op.ins) {
  case ADC: step_ADC(op); break;
  case AND: step_AND(op); break;
  case ASL: step_ASL(op); break;
  case BCC: step_BCC(op); break;
  case BCS: step_BCS(op); break;
  case BEQ: step_BEQ(op); break;
  case BIT: step_BIT(op); break;
  case BMI: step_BMI(op); break;
  case BNE: step_BNE(op); break;
  case BPL: step_BPL(op); break;
  case BRK: step_BRK(op); break;
  case BVC: step_BVC(op); break;
  case BVS: step_BVS(op); break;
  case CLC: step_CLC(op); break;
  case CLD: step_CLD(op); break;
  case CLI: step_CLI(op); break;
  case CLV: step_CLV(op); break;
  case CMP: step_CMP(op); break;
  case CPX: step_CPX(op); break;
  case CPY: step_CPY(op); break;
  case DCP: step_DCP(op); break;
  case DEC: step_DEC(op); break;
  case DEX: step_DEX(op); break;
  case DEY: step_DEY(op); break;
  case EOR: step_EOR(op); break;
  case INC: step_INC(op); break;
  case INX: step_INX(op); break;
  case INY: step_INY(op); break;
  case ISB: step_ISB(op); break;
  case JMP: step_JMP(op); break;
  case JSR: step_JSR(op); break;
  case LAX: step_LAX(op); break;
  case LDA: step_LDA(op); break;
  case LDX: step_LDX(op); break;
  case LDY: step_LDY(op); break;
  case LSR: step_LSR(op); break;
  case NOP: step_NOP(op); break;
  case ORA: step_ORA(op); break;
  case PHA: step_PHA(op); break;
  case PHP: step_PHP(op); break;
  case PHX: step_PHX(op); break;
  case PHY: step_PHY(op); break;
  case PLA: step_PLA(op); break;
  case PLP: step_PLP(op); break;
  case PLX: step_PLX(op); break;
  case PLY: step_PLY(op); break;
  case RLA: step_RLA(op); break;
  case ROL: step_ROL(op); break;
  case ROR: step_ROR(op); break;
  case RRA: step_RRA(op); break;
  case RTI: step_RTI(op); break;
  case RTS: step_RTS(op); break;
  case SAX: step_SAX(op); break;
  case SBC: step_SBC(op); break;
  case SEC: step_SEC(op); break;
  case SED: step_SED(op); break;
  case SEI: step_SEI(op); break;
  case SLO: step_SLO(op); break;
  case SRE: step_SRE(op); break;
  case STA: step_STA(op); break;
  case STX: step_STX(op); break;
  case STY: step_STY(op); break;
  case TAX: step_TAX(op); break;
  case TAY: step_TAY(op); break;
  case TSX: step_TSX(op); break;
  case TXA: step_TXA(op); break;
  case TXS: step_TXS(op); break;
  case TYA: step_TYA(op); break;
  default: step_unimplemented(op); break;
  }

  cycles_ += op.base_cycles + oops_;
  if (!jump_) {
    regs_.PC += op.bytes;
  }
}

static bool page_crossed(uint16_t addr1, uint16_t addr2) {
  return (addr1 & 0xff00) != (addr2 & 0xff00);
}

uint16_t Cpu::decode_addr(const OpCode &op) {
  switch (op.mode) {
  case ABSOLUTE: {
    return peek16(regs_.PC + 1);
  }
  case ABSOLUTE_X: {
    uint16_t addr0 = peek16(regs_.PC + 1);
    uint16_t addr1 = addr0 + regs_.X;
    oops_          = (op.flags & FORCE_OOPS) || page_crossed(addr0, addr1);
    return addr1;
  }
  case ABSOLUTE_Y: {
    uint16_t addr0 = peek16(regs_.PC + 1);
    uint16_t addr1 = addr0 + regs_.Y;
    oops_          = (op.flags & FORCE_OOPS) || page_crossed(addr0, addr1);
    return addr1;
  }
  case RELATIVE: {
    uint16_t addr0 = regs_.PC + 2;
    int8_t   off   = (int8_t)peek(regs_.PC + 1);
    uint16_t addr1 = (uint16_t)(addr0 + off);
    oops_          = (op.flags & FORCE_OOPS) || page_crossed(addr0, addr1);
    return addr1;
  }
  case ZERO_PAGE: {
    return peek(regs_.PC + 1);
  }
  case ZERO_PAGE_X: {
    uint8_t addr0 = peek(regs_.PC + 1);
    uint8_t addr1 = addr0 + regs_.X;
    return addr1;
  }
  case ZERO_PAGE_Y: {
    uint8_t addr0 = peek(regs_.PC + 1);
    uint8_t addr1 = addr0 + regs_.Y;
    return addr1;
  }
  case INDIRECT: {
    uint8_t  lo0     = peek(regs_.PC + 1);
    uint8_t  lo1     = lo0 + 1;
    uint8_t  hi      = peek(regs_.PC + 2);
    uint16_t addr0   = (uint16_t)(lo0 + (hi << 8));
    uint16_t addr1   = (uint16_t)(lo1 + (hi << 8));
    uint8_t  addr_lo = peek(addr0);
    uint8_t  addr_hi = peek(addr1);
    return (uint16_t)(addr_lo + (addr_hi << 8));
  }
  case INDIRECT_X: {
    uint8_t a     = peek(regs_.PC + 1);
    uint8_t addr0 = a + regs_.X;
    uint8_t addr1 = addr0 + 1;
    uint8_t lo    = peek(addr0);
    uint8_t hi    = peek(addr1);
    return (uint16_t)(lo + (hi << 8));
  }
  case INDIRECT_Y: {
    uint8_t  a0    = peek(regs_.PC + 1);
    uint8_t  a1    = a0 + 1;
    uint16_t addr0 = (uint16_t)(peek(a0) + (peek(a1) << 8));
    uint16_t addr1 = addr0 + regs_.Y;
    oops_          = (op.flags & FORCE_OOPS) || page_crossed(addr0, addr1);
    return addr1;
  }
  default:
    throw std::runtime_error(std::format(
        "unsupported addressing mode: {} (${:02X})",
        ADDR_MODE_NAMES[op.mode],
        op.code
    ));
  }
}

uint8_t Cpu::decode_mem(const OpCode &op) {
  switch (op.mode) {
  case IMMEDIATE: return peek(regs_.PC + 1);
  case ZERO_PAGE:
  case ZERO_PAGE_X:
  case ZERO_PAGE_Y:
  case ABSOLUTE:
  case ABSOLUTE_X:
  case ABSOLUTE_Y:
  case INDIRECT_X:
  case INDIRECT_Y: return peek(decode_addr(op));
  default:
    throw std::runtime_error(std::format(
        "unsupported addressing mode: {} (${:02X})",
        ADDR_MODE_NAMES[op.mode],
        op.code
    ));
  }
}

void Cpu::step_ADC(const OpCode &op) {
  uint8_t  mem   = decode_mem(op);
  bool     carry = regs_.P & C_FLAG;
  uint16_t res   = regs_.A + mem + carry;
  set_flag(C_FLAG, res > 0xff);
  set_flag(V_FLAG, (res ^ regs_.A) & (res ^ mem) & 0x80);
  step_load((uint8_t)res, regs_.A);
}

void Cpu::step_AND(const OpCode &op) {
  uint8_t mem = decode_mem(op);
  uint8_t res = regs_.A & mem;
  step_load(res, regs_.A);
}

void Cpu::step_ASL(const OpCode &op) { step_shift_left(op, false); }

void Cpu::step_BCC(const OpCode &op) { step_branch(op, !(regs_.P & C_FLAG)); }
void Cpu::step_BCS(const OpCode &op) { step_branch(op, regs_.P & C_FLAG); }
void Cpu::step_BEQ(const OpCode &op) { step_branch(op, regs_.P & Z_FLAG); }

void Cpu::step_BIT(const OpCode &op) {
  uint8_t mem = decode_mem(op);
  uint8_t res = regs_.A & mem;
  set_flag(Z_FLAG, res == 0);
  set_flag(N_FLAG, mem & 0b10000000);
  set_flag(V_FLAG, mem & 0b01000000);
}

void Cpu::step_BMI(const OpCode &op) { step_branch(op, regs_.P & N_FLAG); }
void Cpu::step_BNE(const OpCode &op) { step_branch(op, !(regs_.P & Z_FLAG)); }
void Cpu::step_BPL(const OpCode &op) { step_branch(op, !(regs_.P & N_FLAG)); }

void Cpu::step_BRK([[maybe_unused]] const OpCode &op) {
  push16(regs_.PC + 2);
  push(regs_.P | 0b00110000);
  regs_.PC = peek16(IRQ_VECTOR);
  regs_.P |= I_FLAG;
  jump_ = true;
}

void Cpu::step_BVC(const OpCode &op) { step_branch(op, !(regs_.P & V_FLAG)); }
void Cpu::step_BVS(const OpCode &op) { step_branch(op, regs_.P & V_FLAG); }

void Cpu::step_CLC([[maybe_unused]] const OpCode &op) { regs_.P &= ~C_FLAG; }
void Cpu::step_CLD([[maybe_unused]] const OpCode &op) { regs_.P &= ~D_FLAG; }
void Cpu::step_CLI([[maybe_unused]] const OpCode &op) { regs_.P &= ~I_FLAG; }
void Cpu::step_CLV([[maybe_unused]] const OpCode &op) { regs_.P &= ~V_FLAG; }

void Cpu::step_CMP(const OpCode &op) { step_compare(op, regs_.A); }
void Cpu::step_CPX(const OpCode &op) { step_compare(op, regs_.X); }
void Cpu::step_CPY(const OpCode &op) { step_compare(op, regs_.Y); }

void Cpu::step_DCP(const OpCode &op) {
  step_DEC(op);
  step_CMP(op);
}

void Cpu::step_DEC(const OpCode &op) {
  uint16_t addr = decode_addr(op);
  uint8_t  mem  = peek(addr);
  uint8_t  res  = mem - 1;
  // N.B., read-modify-write instruction, extra write can matter if targeting
  // a hardware register.
  poke(addr, mem);
  poke(addr, res);
  set_flag(Z_FLAG, res == 0);
  set_flag(N_FLAG, res & 0b10000000);
}

void Cpu::step_DEX([[maybe_unused]] const OpCode &op) {
  step_load(regs_.X - 1, regs_.X);
}

void Cpu::step_DEY([[maybe_unused]] const OpCode &op) {
  step_load(regs_.Y - 1, regs_.Y);
}

void Cpu::step_EOR(const OpCode &op) {
  uint8_t mem = decode_mem(op);
  uint8_t res = regs_.A ^ mem;
  step_load(res, regs_.A);
}

void Cpu::step_INC(const OpCode &op) {
  uint16_t addr = decode_addr(op);
  uint8_t  mem  = peek(addr);
  uint8_t  res  = mem + 1;
  // N.B., read-modify-write instruction, extra write can matter if targeting
  // a hardware register.
  poke(addr, mem);
  poke(addr, res);
  set_flag(Z_FLAG, res == 0);
  set_flag(N_FLAG, res & 0b10000000);
}

void Cpu::step_INX([[maybe_unused]] const OpCode &op) {
  step_load(regs_.X + 1, regs_.X);
}

void Cpu::step_INY([[maybe_unused]] const OpCode &op) {
  step_load(regs_.Y + 1, regs_.Y);
}

void Cpu::step_ISB(const OpCode &op) {
  step_INC(op);
  step_SBC(op);
}

void Cpu::step_JMP(const OpCode &op) {
  regs_.PC = decode_addr(op);
  jump_    = true;
}

void Cpu::step_JSR(const OpCode &op) {
  push16(regs_.PC + 2);
  regs_.PC = decode_addr(op);
  jump_    = true;
}

void Cpu::step_LAX(const OpCode &op) {
  uint8_t res = decode_mem(op);
  step_load(res, regs_.A);
  step_load(res, regs_.X);
}

void Cpu::step_LDA(const OpCode &op) { step_load_mem(op, regs_.A); }
void Cpu::step_LDX(const OpCode &op) { step_load_mem(op, regs_.X); }
void Cpu::step_LDY(const OpCode &op) { step_load_mem(op, regs_.Y); }

void Cpu::step_LSR(const OpCode &op) { step_shift_right(op, false); }

void Cpu::step_NOP(const OpCode &op) {
  if (op.flags & ILLEGAL &&
      (op.mode != IMPLICIT && op.mode != ACCUMULATOR && op.mode != IMMEDIATE)) {
    // N.B., some illegal op-codes generate an "oops" cycle.
    decode_addr(op);
  }
}

void Cpu::step_ORA(const OpCode &op) {
  uint8_t mem = decode_mem(op);
  uint8_t res = regs_.A | mem;
  step_load(res, regs_.A);
}

void Cpu::step_PHA([[maybe_unused]] const OpCode &op) { push(regs_.A); }
void Cpu::step_PHX([[maybe_unused]] const OpCode &op) { push(regs_.X); }
void Cpu::step_PHY([[maybe_unused]] const OpCode &op) { push(regs_.Y); }

void Cpu::step_PHP([[maybe_unused]] const OpCode &op) {
  push(regs_.P | 0b00110000);
}

void Cpu::step_PLA([[maybe_unused]] const OpCode &op) {
  step_load_stack(regs_.A);
}

void Cpu::step_PLP([[maybe_unused]] const OpCode &op) {
  constexpr uint8_t mask = 0b11001111;
  uint8_t           mem  = pop();
  // TODO: delay setting I_FLAG by one instruction?
  regs_.P = (mem & mask) | (regs_.P & ~mask);
}

void Cpu::step_PLX([[maybe_unused]] const OpCode &op) {
  step_load_stack(regs_.X);
}

void Cpu::step_PLY([[maybe_unused]] const OpCode &op) {
  step_load_stack(regs_.Y);
}

void Cpu::step_RLA(const OpCode &op) {
  step_ROL(op);
  step_AND(op);
}

void Cpu::step_ROL(const OpCode &op) { step_shift_left(op, true); }
void Cpu::step_ROR(const OpCode &op) { step_shift_right(op, true); }

void Cpu::step_RRA(const OpCode &op) {
  step_ROR(op);
  step_ADC(op);
}

void Cpu::step_RTI([[maybe_unused]] const OpCode &op) {
  constexpr uint8_t mask = 0b11001111;
  uint8_t           mem  = pop();
  regs_.P                = (regs_.P & ~mask) | (mem & mask);
  regs_.PC               = pop16();
  jump_                  = true;
}

void Cpu::step_RTS([[maybe_unused]] const OpCode &op) {
  regs_.PC = pop16() + 1;
  jump_    = true;
}

void Cpu::step_SAX(const OpCode &op) {
  uint16_t addr = decode_addr(op);
  uint8_t  res  = regs_.A & regs_.X;
  poke(addr, res);
}

void Cpu::step_SBC(const OpCode &op) {
  uint8_t mem   = decode_mem(op);
  bool    carry = regs_.P & C_FLAG;
  int16_t res16 = regs_.A - mem - !carry;
  uint8_t res8  = regs_.A + ~mem + carry;
  set_flag(C_FLAG, !(res16 < 0));
  set_flag(V_FLAG, (res8 ^ regs_.A) & (res8 ^ ~mem) & 0x80);
  step_load(res8, regs_.A);
}

void Cpu::step_SEC([[maybe_unused]] const OpCode &op) { regs_.P |= C_FLAG; }
void Cpu::step_SED([[maybe_unused]] const OpCode &op) { regs_.P |= D_FLAG; }
void Cpu::step_SEI([[maybe_unused]] const OpCode &op) { regs_.P |= I_FLAG; }

void Cpu::step_SLO(const OpCode &op) {
  step_ASL(op);
  step_ORA(op);
}

void Cpu::step_SRE(const OpCode &op) {
  step_LSR(op);
  step_EOR(op);
}

void Cpu::step_STA(const OpCode &op) {
  uint16_t addr = decode_addr(op);
  poke(addr, regs_.A);
}

void Cpu::step_STX(const OpCode &op) {
  uint16_t addr = decode_addr(op);
  poke(addr, regs_.X);
}

void Cpu::step_STY(const OpCode &op) {
  uint16_t addr = decode_addr(op);
  poke(addr, regs_.Y);
}

void Cpu::step_TAX([[maybe_unused]] const OpCode &op) {
  step_load(regs_.A, regs_.X);
}

void Cpu::step_TAY([[maybe_unused]] const OpCode &op) {
  step_load(regs_.A, regs_.Y);
}

void Cpu::step_TSX([[maybe_unused]] const OpCode &op) {
  step_load(regs_.S, regs_.X);
}

void Cpu::step_TXA([[maybe_unused]] const OpCode &op) {
  step_load(regs_.X, regs_.A);
}

void Cpu::step_TXS([[maybe_unused]] const OpCode &op) {
  // N.B., does not set flags, so don't use step_load
  regs_.S = regs_.X;
}

void Cpu::step_TYA([[maybe_unused]] const OpCode &op) {
  step_load(regs_.Y, regs_.A);
}

void Cpu::step_NMI() {
  push16(regs_.PC);
  push(regs_.P & ~B_FLAG);
  regs_.P |= I_FLAG;
  regs_.PC = peek16(NMI_VECTOR);
}

void Cpu::step_IRQ() {
  push16(regs_.PC);
  push(regs_.P & ~B_FLAG);
  regs_.P |= I_FLAG;
  regs_.PC = peek16(IRQ_VECTOR);
}

void Cpu::step_OAM_DMA() {
  uint16_t src_addr = (uint16_t)(ppu_->registers().OAMDMA << 8);
  for (int i = 0; i < 256; i++) {
    ppu_->write_OAMDATA(peek(src_addr++));
  }
}

void Cpu::step_load_mem(const OpCode &op, uint8_t &reg) {
  uint8_t res = decode_mem(op);
  step_load(res, reg);
}

void Cpu::step_load_stack(uint8_t &reg) {
  uint8_t res = pop();
  step_load(res, reg);
}

void Cpu::step_load(uint8_t res, uint8_t &reg) {
  reg = res;
  set_flag(Z_FLAG, res == 0);
  set_flag(N_FLAG, res & 0b10000000);
}

void Cpu::step_branch(const OpCode &op, bool test) {
  if (test) {
    regs_.PC = decode_addr(op);
    cycles_ += 1;
    jump_ = true;
  }
}

void Cpu::step_compare(const OpCode &op, uint8_t &reg) {
  uint8_t mem = decode_mem(op);
  uint8_t res = reg - mem;
  set_flag(C_FLAG, reg >= mem);
  set_flag(Z_FLAG, reg == mem);
  set_flag(N_FLAG, res & 0b10000000);
}

void Cpu::step_shift_left(const OpCode &op, bool carry) {
  if (op.mode == ACCUMULATOR) {
    uint8_t res = (uint8_t)(regs_.A << 1);
    if (carry) {
      res |= regs_.P & C_FLAG;
    }
    set_flag(C_FLAG, regs_.A & 0x80);
    step_load(res, regs_.A);
  } else {
    uint16_t addr = decode_addr(op);
    uint8_t  mem  = peek(addr);
    uint8_t  res  = (uint8_t)(mem << 1);
    if (carry) {
      res |= regs_.P & C_FLAG;
    }
    set_flag(C_FLAG, mem & 0x80);
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, res & 0x80);
    // N.B., read-modify-write instruction, extra write can matter if
    // targeting a hardware register.
    poke(addr, mem);
    poke(addr, res);
  }
}

void Cpu::step_shift_right(const OpCode &op, bool carry) {
  if (op.mode == ACCUMULATOR) {
    uint8_t res = regs_.A >> 1;
    if (carry) {
      res |= (regs_.P & C_FLAG) << 7;
    }
    set_flag(C_FLAG, regs_.A & 0x1);
    step_load(res, regs_.A);
  } else {
    uint16_t addr = decode_addr(op);
    uint8_t  mem  = peek(addr);
    uint8_t  res  = mem >> 1;
    if (carry) {
      res |= (regs_.P & C_FLAG) << 7;
    }
    set_flag(C_FLAG, mem & 0x1);
    set_flag(Z_FLAG, res == 0);
    set_flag(N_FLAG, res & 0x80);
    // N.B., read-modify-write instruction, extra write can matter if
    // targeting a hardware register.
    poke(addr, mem);
    poke(addr, res);
  }
}

void Cpu::step_unimplemented(const OpCode &op) {
  throw std::runtime_error(std::format(
      "unimplemented instruction: {} (${:02X})",
      INSTRUCTION_NAMES[op.ins],
      (int)op.code
  ));
}

static void pad_to(std::string &out_str, size_t column) {
  while (out_str.size() < column) {
    out_str.push_back(' ');
  }
}

std::string Cpu::disassemble() {
  std::string   out_str;
  auto          out_it = std::back_inserter(out_str);
  const OpCode &op     = OP_CODES[peek(regs_.PC)];

  std::format_to(out_it, "{:04X}  ", regs_.PC);

  for (uint8_t i = 0; i < 3; i++) {
    if (i < op.bytes) {
      std::format_to(out_it, "{:02X} ", peek(regs_.PC + i));
    } else {
      std::format_to(out_it, "   ");
    }
  }

  pad_to(out_str, 15);
  std::format_to(
      out_it,
      "{}{} ",
      (op.flags & ILLEGAL) ? '*' : ' ',
      INSTRUCTION_NAMES[op.ins]
  );

  auto format_mem = [&](uint16_t addr) {
    bool io_mapped =
        !(test_ram_ || addr < RAM_END || addr >= Cart::CPU_ADDR_START);
    if (!io_mapped) {
      uint8_t mem = peek(addr);
      std::format_to(out_it, " = {:02X}", mem);
    } else {
      // don't peek IO-mapped memory, since this might have side-effects
      std::format_to(out_it, " = ??");
    }
  };

  switch (op.mode) {
  case ACCUMULATOR: std::format_to(out_it, "A"); break;
  case IMPLICIT: break;
  case ABSOLUTE: {
    uint16_t addr = decode_addr(op);
    std::format_to(out_it, "${:04X}", addr);
    if (op.ins != JSR && op.ins != JMP) {
      format_mem(addr);
    }
    break;
  }
  case ABSOLUTE_X: {
    uint16_t addr0 = peek16(regs_.PC + 1);
    uint16_t addr1 = addr0 + regs_.X;
    std::format_to(out_it, "${:04X},X @ {:04X}", addr0, addr1);
    format_mem(addr1);
    break;
  }
  case ABSOLUTE_Y: {
    uint16_t addr0 = peek16(regs_.PC + 1);
    uint16_t addr1 = addr0 + regs_.Y;
    std::format_to(out_it, "${:04X},Y @ {:04X}", addr0, addr1);
    format_mem(addr1);
    break;
  }
  case RELATIVE: {
    uint16_t addr = decode_addr(op);
    std::format_to(out_it, "${:04X}", addr);
    break;
  }
  case IMMEDIATE: {
    uint8_t mem = decode_mem(op);
    std::format_to(out_it, "#${:02X}", mem);
    break;
  }
  case ZERO_PAGE: {
    uint16_t addr = decode_addr(op);
    std::format_to(out_it, "${:02X}", addr);
    format_mem(addr);
    break;
  }
  case ZERO_PAGE_X: {
    uint8_t addr0 = peek(regs_.PC + 1);
    uint8_t addr1 = addr0 + regs_.X;
    std::format_to(out_it, "${:02X},X @ {:02X}", addr0, addr1);
    format_mem(addr1);
    break;
  }
  case ZERO_PAGE_Y: {
    uint8_t addr0 = peek(regs_.PC + 1);
    uint8_t addr1 = addr0 + regs_.Y;
    std::format_to(out_it, "${:02X},Y @ {:02X}", addr0, addr1);
    format_mem(addr1);
    break;
  }
  case INDIRECT: {
    uint16_t addr0 = peek16(regs_.PC + 1);
    uint16_t addr  = decode_addr(op);
    std::format_to(out_it, "(${:04X}) = {:04X}", addr0, addr);
    break;
  }
  case INDIRECT_X: {
    uint8_t  a     = peek(regs_.PC + 1);
    uint8_t  addr0 = a + regs_.X;
    uint16_t addr  = decode_addr(op);
    std::format_to(out_it, "(${:02X},X) @ {:02X} = {:04X}", a, addr0, addr);
    format_mem(addr);
    break;
  }
  case INDIRECT_Y: {
    uint8_t  a0    = peek(regs_.PC + 1);
    uint8_t  a1    = a0 + 1;
    uint16_t addr0 = (uint16_t)(peek(a0) + (peek(a1) << 8));
    uint16_t addr1 = addr0 + regs_.Y;
    std::format_to(out_it, "(${:02X}),Y = {:04X} @ {:04X}", a0, addr0, addr1);
    format_mem(addr1);
    break;
  }
  default:
    throw std::runtime_error(std::format(
        "unsupported addressing mode: {} (${:02X})",
        ADDR_MODE_NAMES[op.mode],
        op.code
    ));
  }

  pad_to(out_str, 48);
  std::format_to(
      out_it,
      "A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X} PPU:???,??? CYC:{}",
      regs_.A,
      regs_.X,
      regs_.Y,
      regs_.P,
      regs_.S,
      cycles_
  );

  return out_str;
}

void Cpu::set_flag(Flags flag, bool value) {
  if (value) {
    regs_.P |= flag;
  } else {
    regs_.P &= ~flag;
  }
}

bool Cpu::get_flag(Flags flag) const { return regs_.P & flag; }

const std::array<Cpu::OpCode, 256> &Cpu::all_op_codes() { return OP_CODES; }