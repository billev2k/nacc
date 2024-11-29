//
// Created by Bill Evans on 9/25/24.
//

#ifndef BCC_AST2IR_H
#define BCC_AST2IR_H

#include "ast.h"
#include "../ir/ir.h"

extern struct IrProgram *ast2ir(struct CProgram *cProgram);

#endif //BCC_AST2IR_H
