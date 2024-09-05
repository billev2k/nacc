cmake_minimum_required(VERSION 3.29)
project(bcc C)

set(CMAKE_C_STANDARD 17)

add_executable(bcc main.c
        startup.c
        startup.h
        lexer.c
        lexer.h
        tokens.h
        utils.c
        utils.h
        parser.c
        parser.h
        c_ast.c
        c_ast.h
        asm_ast.c
        asm_ast.h
        compiler.c
        compiler.h
)
