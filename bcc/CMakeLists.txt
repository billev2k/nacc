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
        ast.c
        ast.h
        amd64.c
        amd64.h
        ir2amd64.c
        ir2amd64.h
        IR.c
        IR.h
        ast2ir.c
        ast2ir.h
        print_ir.c
        print_ir.h
        SetOfItemTest.c
        SetOfItemTest.h
)

add_executable(bcc_test SetOfItemTest.c
        utils.c
        utils.h
)
