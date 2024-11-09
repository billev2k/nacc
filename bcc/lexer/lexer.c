//
// Created by Bill Evans on 8/20/24.
//

#include <stdlib.h>
#include <ctype.h>
#include "lexer.h"
#include "../parser/ast.h"
#include "../utils/startup.h"

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
#define X(a,b,c) b
    TOKENS__
#undef X
};
enum TOKEN_FLAGS TOKEN_FLAGS[] = {
#define X(a,b,c) c
        TOKENS__
#undef X
};

char const *sourceFileName;
FILE *sourceFile = NULL;
int atEOF = 1;

// Forwards

static void tokens_set_init(void);
static const char * tokens_set_insert(const char *str);
int tokens_set_contains(const char *str);
static int read_next_line(void);
// lex a numeric token_text
static enum TK numericToken(void);
// lex a word token_text (keyword or identifier)
static enum TK wordToken(void);


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

static struct Token internal_take_token(void);
static enum TK tokenizer(void);

struct Token readahead;
int have_readahead = 0;

struct Token lex_peek_token(void) {
    if (!have_readahead) {
        readahead = internal_take_token();
        have_readahead = 1;
    }
    if (traceTokens) printf("Peek token (%d) %s\n", readahead.token, readahead.text);
    return readahead;
}

struct Token lex_take_token(void) {
    if (have_readahead) {
        have_readahead = 0;
        if (traceTokens) printf("Take ra token (%d) %s\n", readahead.token, readahead.text);
        return readahead;
    }
    struct Token token = internal_take_token();
    if (traceTokens) printf("Take token (%d) %s\n", token.token, token.text);
    return token;
}

static struct Token internal_take_token(void) {
    enum TK tk = tokenizer();
    current_token.token = tk;
    if (tk == TK_ID || tk == TK_LITERAL) {
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
static enum TK tokenizer(void) {
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
        case '+':
            ++pBuffer;
            if (*pBuffer == '+') {
                ++pBuffer;
                ++token_end;
                return TK_INCREMENT;
            }
            return TK_PLUS;
        case '-': {
            ++pBuffer;
            if (*pBuffer == '-') {
                ++pBuffer;
                ++token_end;
                return TK_DECREMENT;
            }
            return TK_HYPHEN;
        }
        case '*':
            ++pBuffer;
            return TK_ASTERISK;
        case '/':
            ++pBuffer;
            return TK_SLASH;
        case '%':
            ++pBuffer;
            return TK_PERCENT;
        case '&':
            ++pBuffer;
            if (*pBuffer == '&') {
                ++pBuffer;
                ++token_end;
                return TK_L_AND;
            }
            return TK_AMPERSAND;
        case '|':
            ++pBuffer;
            if (*pBuffer == '|') {
                ++pBuffer;
                ++token_end;
                return TK_L_OR;
            }
            return TK_OR;
        case '^':
            ++pBuffer;
            return TK_CARET;
        case '!':
            ++pBuffer;
            if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_NE;
            }
            return TK_L_NOT;
        case '=':
            ++pBuffer;
            if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_EQ;
            }
            return TK_NE; // TODO: Shoudl be "assign"
        case '<':
            ++pBuffer;
            if (*pBuffer == '<') {
                ++pBuffer;
                ++token_end;
                return TK_LSHIFT;
            } else if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_LE;
            }
            return TK_LT;
        case '>':
            ++pBuffer;
            if (*pBuffer == '>') {
                ++pBuffer;
                ++token_end;
                return TK_RSHIFT;
            } else if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_GE;
            }
            return TK_GT;
        case '~':
            ++pBuffer;
            return TK_TILDE;
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
    return TK_LITERAL;
}

struct set_of_str token_strings;
/**
 * Initialize the set of token_text strings.
 */
void tokens_set_init(void) {
    set_of_str_init(&token_strings, 101);
}
int tokens_set_contains(const char *str) {
    return set_of_str_find(&token_strings, str) != NULL;
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