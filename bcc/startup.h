//
// Created by Bill Evans on 8/20/24.
//

#ifndef BCC_STARTUP_H
#define BCC_STARTUP_H

extern int configOptLex;
extern int configOptParse;
extern int configOptCodegen;
extern int configOptAsm;

extern char const *inputFname;
extern char const *ppFname;
extern char const *asmFname;
extern char const *executableFname;

extern int parseConfig(int argc, char **argv);

#endif //BCC_STARTUP_H
