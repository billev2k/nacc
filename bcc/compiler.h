//
// Created by Bill Evans on 9/2/24.
//

#ifndef BCC_COMPILER_H
#define BCC_COMPILER_H

#include "ast.h"
#include "asm.h"

extern struct AsmProgram *c2asm(struct CProgram *cProgram);

#endif //BCC_COMPILER_H
