#ifndef W2N_AST_INSTRUCTIONS_H
#define W2N_AST_INSTRUCTIONS_H

#include <cstdint>

namespace w2n {

enum class Instruction : uint8_t {
#define INST(Id, Opcode0, ...) Id = Opcode0,
#include <w2n/ASt/Instructions.def>
};

} // namespace w2n

#endif // W2N_AST_INSTRUCTIONS_H