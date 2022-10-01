#ifndef W2N_DRIVERTOOL_DRIVERTOOL_H
#define W2N_DRIVERTOOL_DRIVERTOOL_H

namespace w2n {

/**
 * @brief The driver interface of wasm2native compiler.
 *
 * @param argc
 * @param argv
 * @return int
 */
int mainEntry(int argc, const char * argv[]);
} // namespace w2n

#endif // W2N_DRIVERTOOL_DRIVERTOOL_H