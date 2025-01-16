//
// Created by Bill Evans on 8/20/24.
//

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "lexer.h"

#include <stdio.h>

#include "../parser/ast.h"
#include "../utils/startup.h"

// This is a pretty long line, but generated code can be longer.
#define INITIAL_LINE_BUFFER_SIZE 512

char *lineBuffer = NULL;
int lineBufferSize = 0;
int lineNumber = 0;

#define MAX_READAHEAD 3

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

/**
 * The name of the given token, like "+" or ">=". Note that the name of an identifier token is not the identifier,
 * but rather "an identifier".
 * @param token for which the name is desired.
 * @return a pointer to the name.
 */
const char *lex_token_name(enum TK token) {
    return token_names[token];
}

static struct Token internal_take_token(void);
static enum TK tokenizer(void);

/**
 * When it is necessary to look ahead without affecting the "current_token", the "peeked" token is stored here.
 */
struct Token readahead_list[MAX_READAHEAD];
int readahead_count = 0;

/**
 * Peek at the next token without affecting the "current_token".
 * @return the "next" token.
 */
struct Token lex_peek_token(void) {
    return lex_peek_ahead(1);
}

/**
 * Peek at the nth token. n==1 means peek at the token that will be returned by lex_take_token()
 * @param n the token at which to peek.
 * @return The token.
 */
struct Token lex_peek_ahead(int n) {
    assert(n <= MAX_READAHEAD);
    int x = n - readahead_count;
    while ((n - readahead_count) > 0) {
        readahead_list[readahead_count++] = internal_take_token();
    }
    return readahead_list[n-1];
}


/**
 * Return the next token from the input stream, and advance the token pointer. If the next token
 * has already been "peeked", returns the peeked token, which becomes the "current_token".
 * @return the next token.
 */
struct Token lex_take_token(void) {
    if (readahead_count) {
        current_token = readahead_list[0];
        for (int i=0; i<readahead_count-1; ++i) {
            readahead_list[i] = readahead_list[i+1];
        }
        --readahead_count;
        if (traceTokens) printf("Take ra token (%d) %s\n", current_token.token, current_token.text);
        return current_token;
    }
    current_token = internal_take_token();
    if (traceTokens) printf("Take token (%d) %s\n", current_token.token, current_token.text);
    return current_token;
}

/**
 * Internal (to the lexer) function to read the next token. May not immediately become the "current_token"
 * if the parser is looking ahead.
 * @return the next parsable token.
 */
static struct Token internal_take_token(void) {
    enum TK tk = tokenizer();
    struct Token token = {.token = tk};
    if (tk == TK_ID || tk == TK_LITERAL) {
        char saved = *token_end;
        *(char *) token_end = '\0';  // const_cast<char*>()
        token.text = tokens_set_insert(token_begin);
        *(char *) token_end = saved;
    } else {
        token.text = lex_token_name(tk);
    }
    return token;
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

    // Assume a one-character token, and adjust if two- or three-character token.
    ++token_end;
    // See what we really have.
    switch (*pBuffer++) {
        case '(':
            return TK_L_PAREN;
        case ')':
            return TK_R_PAREN;
        case '{':
            return TK_L_BRACE;
        case '}':
            return TK_R_BRACE;
        case ';':
            return TK_SEMI;
        case '+':
            if (*pBuffer == '+') {
                ++pBuffer;
                ++token_end;
                return TK_INCREMENT;
            } else if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_ASSIGN_PLUS;
            }
            return TK_PLUS;
        case '-': {
            if (*pBuffer == '-') {
                ++pBuffer;
                ++token_end;
                return TK_DECREMENT;
            } else if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_ASSIGN_MINUS;
            }
            return TK_HYPHEN;
        }
        case '*':
            if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_ASSIGN_MULT;
            }
            return TK_ASTERISK;
        case '/':
            if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_ASSIGN_DIV;
            }
            return TK_SLASH;
        case '%':
            if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_ASSIGN_MOD;
            }
            return TK_PERCENT;
        case '&':
            if (*pBuffer == '&') {
                ++pBuffer;
                ++token_end;
                return TK_L_AND;
            } else if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_ASSIGN_AND;
            }
            return TK_AMPERSAND;
        case '|':
            if (*pBuffer == '|') {
                ++pBuffer;
                ++token_end;
                return TK_L_OR;
            } else if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_ASSIGN_OR;
            }
            return TK_OR;
        case '^':
            if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_ASSIGN_XOR;
            }return TK_CARET;
        case '!':
            if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_NE;
            }
            return TK_L_NOT;
        case '=':
            if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_EQ;
            }
            return TK_ASSIGN;
        case '<':
            if (*pBuffer == '<') {
                ++pBuffer;
                ++token_end;
                if (*pBuffer == '=') {
                    ++pBuffer;
                    ++token_end;
                    return TK_ASSIGN_LSHIFT;
                }
                return TK_LSHIFT;
            } else if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_LE;
            }
            return TK_LT;
        case '>':
            if (*pBuffer == '>') {
                ++pBuffer;
                ++token_end;
                if (*pBuffer == '=') {
                    ++pBuffer;
                    ++token_end;
                    return TK_ASSIGN_RSHIFT;        // >>=
                }
                return TK_RSHIFT;                   // >>
            } else if (*pBuffer == '=') {
                ++pBuffer;
                ++token_end;
                return TK_GE;                       // >=
            }
            return TK_GT;                           // >
        case '~':
            return TK_TILDE;
        case '?':
            return TK_QUESTION;
        case ':':
            return TK_COLON;
        default:
            return TK_UNKNOWN;
    }
}

/**
 * Reads a "word" token_text from the stream.
 *
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

/**
 * Parse a run of decimal digits.
 * @return TK_LITERAL if a well-formed number (currently integer), and TK_UNKNOWN if the digit(s) are
 * prefix to an alpha character or '_'.
 */
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
    return set_of_str_find(&token_strings, str, NULL);
}
const char * tokens_set_insert(const char *str) {
    return set_of_str_insert(&token_strings, str);
}

/**
 * Reads the next non-blank line from the input stream. Lines terminate with '\n' or at EOF.
 * @return non-zero if a line was read, 0 if no more lines.
 */
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