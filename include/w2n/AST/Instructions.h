#ifndef W2N_AST_INSTRUCTIONS_H
#define W2N_AST_INSTRUCTIONS_H

namespace w2n {

enum class Instruction {
#define INST(Id, Opcode0, ...) Id = Opcode0,
#include <w2n/ASt/Instructions.def>
};

} // namespace w2n

#endif // W2N_AST_INSTRUCTIONS_H