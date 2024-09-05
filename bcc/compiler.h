//
// Created by Bill Evans on 9/2/24.
//

#ifndef BCC_COMPILER_H
#define BCC_COMPILER_H

#include "c_ast.h"
#include "asm_ast.h"

extern struct AsmProgram *c2asm(struct CProgram *cProgram);

#endif //BCC_COMPILER_H
