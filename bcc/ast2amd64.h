//
// Created by Bill Evans on 9/2/24.
//

#ifndef BCC_AST2AMD64_H
#define BCC_AST2AMD64_H

#include "ast.h"
#include "amd64.h"

extern struct Amd64Program *ast2amd64(struct CProgram *cProgram);

#endif //BCC_AST2AMD64_H
