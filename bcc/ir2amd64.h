//
// Created by Bill Evans on 9/2/24.
//

#ifndef BCC_IR2AMD64_H
#define BCC_IR2AMD64_H

#include "IR.h"
#include "amd64.h"

extern struct Amd64Program *ir2amd64(struct IrProgram* irProgram);

#endif //BCC_IR2AMD64_H
