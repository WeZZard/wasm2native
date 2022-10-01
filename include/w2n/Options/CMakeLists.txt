# LLVM_TARGET_DEFINITIONS must contain tablegen files to process
set(LLVM_TARGET_DEFINITIONS Options.td)
w2n_tablegen(Options.inc -gen-opt-parser-defs)
w2n_add_public_tablegen_target(W2NOptionsTbd)
