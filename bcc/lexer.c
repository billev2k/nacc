//
// Created by Bill Evans on 8/20/24.
//

#include <stdlib.h>
#include <ctype.h>
#include "lexer.h"
#include "utils.h"

#define INITIAL_TOKEN_BUFFER_SIZE 128
// This is a pretty long line, but generated code can be longer.
#define INITIAL_LINE_BUFFER_SIZE 512

char *lineBuffer = NULL;
int lineBufferSize = 0;
int lineNumber = 0;

// Pointer to next character in line buffer.
const char *pBuffer;
// Pointer to beginning of current token, in line buffer
const char *token_begin;
// Pointer to end of current token, in line buffer
const char *token_end;

struct Token current_token;

const char *token_names[NUM_TOKEN_TYPES] = {
#define X(a,b) b
    TOKENS__
#undef X
};

char const *sourceFileName;
FILE *sourceFile = NULL;
int atEOF = 1;

// Forwards

void tokens_set_init(void);
const char * tokens_set_insert(const char *str);
int tokens_set_contains(const char *str);
static int read_next_line(void);
// lex a numeric token_text
enum TK numericToken(void);
// lex a word token_text (keyword or identifier)
enum TK wordToken(void);
// append a character to the token_text buffer.
void storeCh(int intCh);


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
    if (lineBuffer == NULL) {
        lineBufferSize = INITIAL_LINE_BUFFER_SIZE;
        lineBuffer = malloc(lineBufferSize);
        lineBuffer[0] = '\0';
        pBuffer = lineBuffer;

        tokens_set_init();
    }
    return 1;
}

const char *lex_token_name(enum TK token) {
    return token_names[token];
}

enum TK internal_take_token(void);
struct Token lex_take_token(void) {
    enum TK tk = internal_take_token();
    current_token.token = tk;
    if (tk == TK_ID || tk == TK_CONSTANT) {
        char saved = *token_end;
        *(char *) token_end = '\0';  // const_cast<char*>()
        current_token.text = tokens_set_insert(token_begin);
        *(char *) token_end = saved;
    } else {
        current_token.text = lex_token_name(tk);
    }
    return current_token;
}

/**
 * Gets the next token_text from the input stream.
 * @return the token_text, or TK_EOF when no more.
 */
enum TK internal_take_token(void) {
    // Skip whitespace. At the end of line, read another line.
    while (isspace(*pBuffer) || *pBuffer=='\0') {
        if (*pBuffer == '\0') {
            if (read_next_line())
                pBuffer = lineBuffer;
            else
                return TK_EOF;
        } else {
            ++pBuffer;
        }
    }

    // pBuffer points to a non-whitespace character. Look there for a token; none yet.
    token_end = token_begin = pBuffer;

    // Starts with alpha or '_'? keyword or identifier
    if (isalpha(*pBuffer) || *pBuffer=='_') {
        return wordToken();
    }

    // Starts with digit (todo: or . followed by digit)? Numeric literal
    if (isdigit(*pBuffer) /* || *pBuffer=='.' && isdigit(*pBuffer+1) */ ) {
        return numericToken();
    }

    // Assume a one-character token.
    ++token_end;
    // See what we really have.
    switch (*pBuffer) {
        case '(':
            ++pBuffer;
            return TK_L_PAREN;
        case ')':
            ++pBuffer;
            return TK_R_PAREN;
        case '{':
            ++pBuffer;
            return TK_L_BRACE;
        case '}':
            ++pBuffer;
            return TK_R_BRACE;
        case ';':
            ++pBuffer;
            return TK_SEMI;
        case '~':
            ++pBuffer;
            return TK_COMPLEMENT;
        case '-': {
            ++pBuffer;
            if (*pBuffer+1 == '-') {
                ++pBuffer;
                ++token_end;
                return TK_DECREMENT;
            }
            return TK_MINUS;
        }
        default:
            ++pBuffer;
            return TK_UNKNOWN;
    }
}

/**
 * Reads a "word" token_text from the stream.
 * @return the TK type.
 */
enum TK wordToken(/*int intCh*/) {
    while (isalnum(*pBuffer) || *pBuffer=='_') {
        ++pBuffer;
    }
    token_end = pBuffer; // next character after token

    // Is it a keyword?
    int token_length = (int)(token_end - token_begin);
    for (int kwix=TK_KEYWORDS_BEGIN; kwix < TK_KEYWORDS_END; ++kwix) {
        if (token_end-token_begin == strlen(token_names[kwix]) &&
                memcmp(token_begin, token_names[kwix], token_length) == 0)
            return kwix;
    }

    return TK_ID;
}

enum TK numericToken(void) {
    // Only reads decimal constants.
    while (isdigit(*pBuffer))
        ++pBuffer;
    token_end = pBuffer;

    if (isalpha(*pBuffer) || *pBuffer=='_') {
        return TK_UNKNOWN;
    }
    return TK_CONSTANT;
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

static int read_next_line(void) {
    if (atEOF) return 0;
    char *pBuf = lineBuffer;
    char *pEnd = lineBuffer + lineBufferSize;
    int intCh;
    int readAnything = 0;
    while ((intCh=fgetc(sourceFile)) != EOF) {
        readAnything = 1;
        // Checking for end of line. First check for LF
        if (intCh == '\n') {
            ++lineNumber;
            break;
        } else if (intCh == '\r') {
            ++lineNumber;
            // If CR, look at next character.
            intCh = fgetc(sourceFile);
            // If not an LF, put it back. If LF, consume it as well.
            if (intCh != '\n' && intCh != EOF) {
                ungetc(intCh, sourceFile);
            }
            break;
        }
        // Keep the character.
        *pBuf++ = (char)intCh;
        // Is buffer full now?
        if (pBuf == pEnd) {
            // Alocate a new buffer 2x size of old, copy data.
            char *newBuf = malloc(lineBufferSize * 2);
            memcpy(newBuf, lineBuffer, lineBufferSize);
            newBuf[lineBufferSize] = '\0';
            free(lineBuffer);
            lineBuffer = newBuf;
            // pBuf should point just past the old data.
            pBuf = lineBuffer + lineBufferSize;
            lineBufferSize += 2;
            pEnd = lineBuffer + lineBufferSize;
        }
    }
    // Null terminate the line.
    *pBuf = '\0';
    if (intCh == EOF) {
        atEOF = 1;
        // If all we read was EOF, return 0. If we read anything, even an empty line, return 1.
        return readAnything;
    }
    return 1;
}