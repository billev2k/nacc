//
// Created by Bill Evans on 11/19/24.
//

#ifndef BCC_SEMANTICS_H
#define BCC_SEMANTICS_H

/**
 * Returns a monotonically increasing number, starting with 0.
 * @return the number.
 */
extern int next_uniquifier(void);

/**
 * Make a unique name by combining a valid symbol name with a '.' and a unique number.
 *
 * @param fmt The format to use to create the unique symbol. Should contain a "%s",
 *          a "%d", and a '.'. May also contain fixed text, eg.: "%s.tmp.%d".
 * @param context A string to help make the resulting string human readable (and understandable).
 * @return A pointer to the generated string. NOTE: the generated string is in a global buffer,
*           and will ony be valid until the next call to make_unique().
*/
extern const char* make_unique(const char* fmt, const char* context);


extern void semantic_analysis(struct CProgram* program);

#endif //BCC_SEMANTICS_H
