//
// Created by Bill Evans on 9/2/24.
//

#include <stdlib.h>
#include "compiler.h"

static struct AsmFunction *compile_function(struct CFunction *cFunction);
static int compile_statement(struct AsmFunction *asmFunction, struct CStatement *cStatement);
static int compile_expression(struct AsmFunction *asmFunction, struct CExpression *cExpression);
static struct AsmOperand *make_operand(enum OPND type, const char *name);

struct AsmProgram *c2asm(struct CProgram *cProgram) {
    struct AsmProgram *result = (struct AsmProgram *)malloc(sizeof(struct AsmProgram));
    result->function = compile_function(cProgram->function);
    return result;
}

static struct AsmFunction *compile_function(struct CFunction *cFunction) {
    struct AsmFunction *result = (struct AsmFunction *)malloc(sizeof(struct AsmFunction));
    result->name = cFunction->name;
    compile_statement(result, cFunction->statement);
    return result;
}

static int compile_statement(struct AsmFunction *asmFunction, struct CStatement *cStatement) {
    switch (cStatement->type) {
        case STMT_RETURN:
            compile_expression(asmFunction, cStatement->expression);
            break;
    }
    return 1;
}

static int compile_expression(struct AsmFunction *asmFunction, struct CExpression *cExpression) {
    struct AsmInstruction *inst;
    switch (cExpression->type) {
        case EXP_CONSTANT:
            inst = (struct AsmInstruction *)malloc(sizeof(struct AsmInstruction));
            inst->instruction = INST_MOV;
            inst->src = make_operand(OPND_IMM, cExpression->value);
            inst->dst = make_operand(OPND_REGISTER, "eax");
            asm_function_append_instruction(asmFunction, inst);
            inst = (struct AsmInstruction *)malloc(sizeof(struct AsmInstruction));
            inst->instruction = INST_RET;
            asm_function_append_instruction(asmFunction, inst);
            break;
    }
    return 1;
}

static struct AsmOperand *make_operand(enum OPND type, const char *name) {
    struct AsmOperand *result = (struct AsmOperand *)malloc(sizeof(struct AsmOperand));
    result->operand_type = type;
    result->name = name;
    return result;
}
