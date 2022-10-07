#ifndef W2N_SEMA_SEMA_H
#define W2N_SEMA_SEMA_H

#include <w2n/Sema/TypeCheck.h>

namespace w2n {

class SourceFile;

void performTypeChecking(SourceFile& File);

}

#endif // W2N_SEMA_SEMA_H