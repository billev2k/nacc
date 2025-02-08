//
// Created by Bill Evans on 8/20/24.
//

#ifndef BCC_STARTUP_H
#define BCC_STARTUP_H

extern int configOptTest;
extern int configOptPpOnly;
extern int configOptLexOnly;
extern int configOptParseOnly;
extern int configOptValidateOnly;
extern int configOptTackyOnly;
extern int configOptCodegenOnly;
extern int configOptNoAssemble;
extern int configOptNoLink;

extern int traceAstMem;
extern int traceTokens;
extern int traceResolution;

extern char const *inputFname;
extern int inputFileIsC;
extern int numInputFileNames;
// Any provided output file name.
extern char const* oFname;
extern char const *ppFname;
extern char const *asmFname;
extern char const *executableFname;

extern char const* assembleAndLinkCommand;

extern int parseConfig(int argc, char **argv);
extern int parseInputFilename(int ix);
extern const char * buildAssembleAndLinkCommand();


#endif //BCC_STARTUP_H
