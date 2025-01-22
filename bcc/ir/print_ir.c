//
// Created by Bill Evans on 9/30/24.
//

#include "print_ir.h"

#define inst_fmt "    %-8s"

static void print_ir_function(const struct IrFunction *function, FILE *file);
static void print_ir_instruction(const struct IrInstruction *instruction, FILE *file);
static void print_ir_value(struct IrValue value, FILE *file);

void print_ir(const struct IrProgram *program, FILE *file) {
    fprintf(file, "\n\nIR program\n");
    print_ir_function(program->function, file);
}

void print_ir_function(const struct IrFunction *function, FILE *file) {
    fprintf(file, "Function %s\n", function->name);
    for (int i=0; i<function->body.num_items; ++i) {
        print_ir_instruction(function->body.items[i], file);
    }
}

void print_ir_instruction(const struct IrInstruction *instruction, FILE *file) {
    switch (instruction->inst) {
        case IR_OP_RET:
            fputs("    RET     ", file);
            print_ir_value(instruction->ret.value, file);
            fputc('\n', file);
            break;
        case IR_OP_UNARY:
            fprintf(file, inst_fmt, IR_UNARY_NAMES[instruction->unary.op]);
            print_ir_value(instruction->unary.src, file);
            fputs(", ", file);
            print_ir_value(instruction->unary.dst, file);
            fputc('\n', file);
            break;
        case IR_OP_BINARY:
            fprintf(file, inst_fmt, IR_BINARY_NAMES[instruction->binary.op]);
            print_ir_value(instruction->binary.src1, file);
            fputs(", ", file);
            print_ir_value(instruction->binary.src2, file);
            fputs(", ", file);
            print_ir_value(instruction->binary.dst, file);
            fputc('\n', file);
            break;
        case IR_OP_COPY:
            fprintf(file, inst_fmt, "copy");
            print_ir_value(instruction->copy.src, file);
            fputs(", ", file);
            print_ir_value(instruction->copy.dst, file);
            fputc('\n', file);
            break;
        case IR_OP_JUMP:
            fprintf(file, inst_fmt, "j");
            print_ir_value(instruction->jump.target, file);
            fputs("\n", file);
            break;
        case IR_OP_JUMP_EQ:
            fprintf(file, inst_fmt, "je");
            print_ir_value(instruction->cjump.comparand, file);
            fputs(", ", file);
            print_ir_value(instruction->cjump.value, file);
            fputs(", ", file);
            print_ir_value(instruction->cjump.target, file);
            fputc('\n', file);
            break;
        case IR_OP_JUMP_ZERO:
            fprintf(file, inst_fmt, "jz");
            print_ir_value(instruction->cjump.value, file);
            fputs(", ", file);
            print_ir_value(instruction->cjump.target, file);
            fputc('\n', file);
            break;
        case IR_OP_JUMP_NZERO:
            fprintf(file, inst_fmt, "jnz");
            print_ir_value(instruction->cjump.value, file);
            fputs(", ", file);
            print_ir_value(instruction->cjump.target, file);
            fputc('\n', file);
            break;
        case IR_OP_LABEL:
            print_ir_value(instruction->label.label, file);
            fputs(":\n", file);
            break;
    }
}

void print_ir_value(struct IrValue value, FILE *file) {
    switch (value.kind) {
        case IR_VAL_CONST_INT:
            fprintf(file, "$%d", value.int_val);
            break;
        case IR_VAL_ID:
        case IR_VAL_LABEL:
            fprintf(file, "%s", value.text);
            break;
    }
}
