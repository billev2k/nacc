//
// Created by Bill Evans on 8/28/24.
//

#ifndef BCC_PARSER_H
#define BCC_PARSER_H

extern struct CProgram *c_program_parse(void);
extern void analyze_program(const struct CProgram* program);

#endif //BCC_PARSER_H
