//
// Created by Bill Evans on 9/2/24.
//

#include <stdlib.h>
#include "ast2amd64.h"

static struct Amd64Function *compile_function(struct CFunction *cFunction);
static int compile_statement(struct Amd64Function *asmFunction, struct CStatement *cStatement);
static int compile_expression(struct Amd64Function *asmFunction, struct CExpression *cExpression);
static struct Amd64Operand *make_operand(enum OPND type, const char *name);

struct Amd64Program *ast2amd64(struct CProgram *cProgram) {
    struct Amd64Program *result = (struct Amd64Program *)malloc(sizeof(struct Amd64Program));
    result->function = compile_function(cProgram->function);
    return result;
}

static struct Amd64Function *compile_function(struct CFunction *cFunction) {
    struct Amd64Function *result = (struct Amd64Function *)malloc(sizeof(struct Amd64Function));
    result->name = cFunction->name;
    compile_statement(result, cFunction->statement);
    return result;
}

static int compile_statement(struct Amd64Function *asmFunction, struct CStatement *cStatement) {
    switch (cStatement->type) {
        case STMT_RETURN:
            compile_expression(asmFunction, cStatement->expression);
            break;
    }
    return 1;
}

static int compile_expression(struct Amd64Function *asmFunction, struct CExpression *cExpression) {
    struct Amd64Instruction *inst;
    switch (cExpression->type) {
        case EXP_CONST_INT:
            inst = (struct Amd64Instruction *)malloc(sizeof(struct Amd64Instruction));
            inst->instruction = INST_MOV;
            inst->src = make_operand(OPND_IMM, cExpression->value);
            inst->dst = make_operand(OPND_REGISTER, "eax");
            amd64_function_append_instruction(asmFunction, inst);
            inst = (struct Amd64Instruction *)malloc(sizeof(struct Amd64Instruction));
            inst->instruction = INST_RET;
            amd64_function_append_instruction(asmFunction, inst);
            break;
        case EXP_NEGATE:
            break;
        case EXP_COMPLEMENT:
            break;
    }
    return 1;
}

static struct Amd64Operand *make_operand(enum OPND type, const char *name) {
    struct Amd64Operand *result = (struct Amd64Operand *)malloc(sizeof(struct Amd64Operand));
    result->operand_type = type;
    result->name = name;
    return result;
}
