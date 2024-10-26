//
// Created by Bill Evans on 9/30/24.
//

#include "print_ir.h"

static void print_ir_function(struct IrFunction *function, FILE *file);
static void print_ir_instruction(struct IrInstruction *instruction, FILE *file);
static void print_ir_value(struct IrValue *value, FILE *file);

void print_ir(struct IrProgram *program, FILE *file) {
    fprintf(file, "IR program\n");
    print_ir_function(program->function, file);
}

void print_ir_function(struct IrFunction *function, FILE *file) {
    fprintf(file, "Function %s\n", function->name);
    for (int i=0; i<function->body.list_count; ++i) {
        print_ir_instruction(function->body.items[i], file);
    }
}

void print_ir_instruction(struct IrInstruction *instruction, FILE *file) {
    switch (instruction->inst) {
        case IR_OP_RET:
            fputs("    RET     ", file);
            print_ir_value(instruction->a, file);
            fputc('\n', file);
            break;
        case IR_OP_UNARY:
            switch (instruction->unary_op) {
                case IR_UNARY_COMPLEMENT:
                    fputs("    CMPL    ", file);
                    print_ir_value(instruction->a, file);
                    fputs(", ", file);
                    print_ir_value(instruction->b, file);
                    fputc('\n', file);
                    break;
                case IR_UNARY_NEGATE:
                    fputs("    NEG     ", file);
                    print_ir_value(instruction->a, file);
                    fputs(", ", file);
                    print_ir_value(instruction->b, file);
                    fputc('\n', file);
                    break;
            }
            break;
    }
}

void print_ir_value(struct IrValue *value, FILE *file) {
    switch (value->type) {
        case IR_VAL_CONST_INT:
            fprintf(file, "Constant(%s)", value->text);
            break;
        case IR_VAL_ID:
            fprintf(file, "Var(%s)", value->text);
            break;
    }
}
