//
// Created by Bill Evans on 8/20/24.
//

#ifndef BCC_STARTUP_H
#define BCC_STARTUP_H

extern int configOptTest;
extern int configOptLex;
extern int configOptParse;
extern int configOptValidate;
extern int configOptTacky;
extern int configOptCodegen;
extern int configOptAsm;
extern int configOptNoLink;

extern int traceAstMem;
extern int traceTokens;
extern int traceResolution;

extern char const *inputFname;
extern char const *ppFname;
extern char const *asmFname;
extern char const *executableFname;

extern int parseConfig(int argc, char **argv);

#endif //BCC_STARTUP_H
