//
// Created by Bill Evans on 9/30/24.
//

#ifndef BCC_PRINT_IR_H
#define BCC_PRINT_IR_H

#include <stdio.h>
#include "ir.h"

extern void print_ir(const struct IrProgram *program, FILE *file);

#endif //BCC_PRINT_IR_H
