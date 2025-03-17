cmake_minimum_required(VERSION 3.29)
project(bcc C)

set(CMAKE_C_STANDARD 17)


add_executable(bcc main.c
        utils/startup.c
        utils/startup.h
        lexer/lexer.c
        lexer/lexer.h
        lexer/tokens.h
        utils/utils.c
        inc/utils.h
        parser/parser.c
        parser/parser.h
        parser/ast.c
        parser/ast.h
        parser/print_ast.c
        parser/print_ast.h
        amd64/amd64.c
        amd64/amd64.h
        amd64/ir2amd64.c
        amd64/ir2amd64.h
        ir/ir.c
        ir/ir.h
        parser/ast2ir.c
        parser/ast2ir.h
        ir/print_ir.c
        ir/print_ir.h
        amd64/emit_amd64.h
        amd64/emit_amd64.c
        parser/semantics.c
        parser/semantics.h
        parser/idtable.c
        parser/idtable.h
        parser/symtable.c
        parser/symtable.h
        parser/symtable.c
        utils/constant.c
        inc/constant.h
)
target_include_directories(bcc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(bcc_test SetOfItemTest.c
        utils/utils.c
        inc/utils.h
)
target_compile_definitions(bcc_test PRIVATE TESTING_SET_IMPL=1)

