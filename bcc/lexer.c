//
// Created by Bill Evans on 8/20/24.
//

#include <stdlib.h>
#include <ctype.h>
#include "lexer.h"
#include "utils.h"

#define INITIAL_TOKEN_BUFFER_SIZE 128

char  *token_text;
char *tokenBuf;
int tokenBufLength = 0;
int tokenBufSize = 0;

struct Token current_token;

const char *token_names[NUM_TOKEN_TYPES];

char const *sourceFileName;
FILE *sourceFile = NULL;
int atEOF = 1;

void tokens_set_init(void);
const char * tokens_set_insert(const char *str);
int tokens_set_contains(const char *str);


void init_token_names() {
    token_names[TK_UNKNOWN] = "!! unknown !!";
    token_names[TK_INT] = "int";
    token_names[TK_VOID] = "void";
    token_names[TK_RETURN] = "return";
    token_names[TK_SEMI] = ";";
    token_names[TK_L_PAREN] = "(";
    token_names[TK_R_PAREN] = ")";
    token_names[TK_L_BRACE] = "{";
    token_names[TK_R_BRACE] = "}";
    token_names[TK_ID] = "an identifier";
    token_names[TK_CONSTANT] = "a constant";
    token_names[TK_EOF] = "eof";
}

/**
 * Opens a source file for lexing. Initializes the token_text buffer on first call.
 * Closes any previously opened file.
 * @param fname to be opened
 * @return zero if the file could not be opened, non-zero if opened.
 */
int lex_openFile(char const *fname) {
    if (sourceFileName != NULL) {
        free((char*)sourceFileName);
    }
    if (sourceFile != NULL) {
        fclose(sourceFile);
    }
    sourceFile = fopen(fname, "r");
    if (sourceFile == NULL) {
        atEOF = 1;
        return 0;
    }
    sourceFileName = strdup(fname);
    atEOF = 0;
    if (tokenBuf == NULL) {
        init_token_names();
        tokenBufSize = INITIAL_TOKEN_BUFFER_SIZE;
        tokenBuf = malloc(tokenBufSize);
        tokenBufLength = 0;
        *(token_text=tokenBuf) = '\0';
        tokens_set_init();
    }
    return 1;
}

// lex a numeric token_text
enum TK numericToken(int intCh);
// lex a word token_text (keyword or identifier)
enum TK wordToken(int intCh);
// append a character to the token_text buffer.
void storeCh(int intCh);

const char *lex_token_name(enum TK token) {
    return token_names[token];
}

enum TK internal_take_token(void);
struct Token lex_take_token(void) {
    enum TK tk = internal_take_token();
    current_token.token = tk;
    current_token.text = tokens_set_insert(token_text);
    return current_token;
}

/**
 * Gets the next token_text from the input stream.
 * @return the token_text, or TK_EOF when no more.
 */
enum TK internal_take_token(void) {
    if (atEOF) {
        return TK_EOF;
    }
    // reset token_text text
    *(token_text=tokenBuf) = '\0';
    tokenBufLength = 0;
    // skip whitespace
    int intCh;
    while ((intCh=getc(sourceFile)) != EOF && isspace(intCh));
    if (intCh == EOF) {
        atEOF = 1;
        return TK_EOF;
    }
    // starts with alpha or _ ?
    if (isalpha(intCh) || intCh == '_') {
        return wordToken(intCh);
    }
    // starts with digit?
    if (isdigit(intCh)) {
        return numericToken(intCh);
    }

    storeCh(intCh);
    if (intCh == '(') return TK_L_PAREN;
    if (intCh == ')') return TK_R_PAREN;
    if (intCh == '{') return TK_L_BRACE;
    if (intCh == '}') return TK_R_BRACE;
    if (intCh == ';') return TK_SEMI;
    return TK_UNKNOWN;
}

/**
 * Reads a "word" token_text from the stream.
 * @param intCh
 * @return
 */
enum TK wordToken(int intCh) {
    storeCh(intCh);
    while (isalnum(intCh=getc(sourceFile)) || intCh=='_')
        storeCh(intCh);
    // Put back any non-word character
    if (intCh != EOF) {
        ungetc(intCh, sourceFile);
    }
    // Is it a keyword?
    if (strcmp(token_text, "int") == 0) return TK_INT;
    if (strcmp(token_text, "void") == 0) return TK_VOID;
    if (strcmp(token_text, "return") == 0) return TK_RETURN;

    return TK_ID;
}

enum TK numericToken(int intCh) {
    // Only reads decimal constants.
    storeCh(intCh);
    while (isdigit(intCh=getc(sourceFile)))
        storeCh(intCh);
    // Put back any non-digit character
    if (intCh != EOF) {
        ungetc(intCh, sourceFile);
    }

    if (isalpha(intCh) || intCh=='_') {
        return TK_UNKNOWN;
    }
    return TK_CONSTANT;
}

void storeCh(int intCh) {
    if (tokenBufLength == tokenBufSize-1) {
        char *newBuf = malloc(tokenBufSize*=2);
        strcpy(tokenBuf, newBuf);
        free(tokenBuf);
        token_text = tokenBuf = newBuf;
    }
    tokenBuf[tokenBufLength++] = (char)intCh;
    tokenBuf[tokenBufLength] = '\0';
}

struct SetOfStr {
    int size;
    int population;
    int collisions;
    const char **set;
};
/**
 * Initializes a set to a given size.
 * @param set to be initialized.
 * @param initial_size size to be initialized to.
 */
void set_of_str_init(struct SetOfStr *set, int initial_size) {
    set->population = set->collisions = 0;
    set->size = initial_size;
    set->set = malloc(set->size * sizeof(char*));
}
#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
const char * set_of_str_insert(struct SetOfStr *set, const char *str);
/**
 * Grows a set by 3x.
 * @param set to be grown.
 */
void set_of_str_grow(struct SetOfStr *set) {
    // New larger set to which to move the old set's contents.
    struct SetOfStr newSet;
    set_of_str_init(&newSet, set->size*3);
    // For every active slot in the old set, insert it into the new set.
    for (int ix=0; ix<set->size; ++ix) {
        const char *pStr = set->set[ix];
        if (pStr) set_of_str_insert(&newSet, pStr);
    }
    // Clean up old set's memory.
    free(set->set);
    // And replace with the new set.
    *set = newSet;
}
/**
 * Insert a string into a set. Does nothing if the string is already in the set.
 * @param set into which to insert the string.
 * @param str to be inserted.
 * @return the inserted or already existing string.
 */
const char * set_of_str_insert(struct SetOfStr *set, const char *str) {
    if (set->collisions > set->size/4 || set->population > set->size*3/4) {
        set_of_str_grow(set);
    }
    unsigned long h = hash_str(str);
    int ix = (int)(h % set->size);
    int isCollision = 0;
    // Search until we find a match or there's nothing else to inspect.
    while (set->set[ix]!=NULL && strcmp(str, set->set[ix])!=0) {
        if (++ix >= set->size) ix=0;
        isCollision = 1;
    }
    // ix either indexes an existing match or an empty slot. If not a match, add it.
    if (set->set[ix] == NULL) {
        set->set[ix] = strdup(str);
        set->population++;
        set->collisions += isCollision;
    }
    return set->set[ix];
}
#pragma clang diagnostic pop
/**
 * Queries whether a set contains a given string.
 * @param set to be queried.
 * @param str to be found.
 * @return true if the string is found in the set.
 */
int set_of_str_contains(struct SetOfStr *set, const char *str) {
    unsigned long h = hash_str(str);
    int ix = (int)(h % set->size);
    int cmp=1; // init to 'not equal'
    // Search until we find a match or there's nothing else to inspect.
    while (set->set[ix] && (cmp=strcmp(str, set->set[ix]))) {
        if (++ix >= set->size) ix=0;
    }
    // return true if we found a match.
    return cmp==0;
}

struct SetOfStr token_strings;
/**
 * Initialize the set of token_text strings.
 */
void tokens_set_init(void) {
    set_of_str_init(&token_strings, 101);
}
int tokens_set_contains(const char *str) {
    return set_of_str_contains(&token_strings, str);
}
const char * tokens_set_insert(const char *str) {
    return set_of_str_insert(&token_strings, str);
}
