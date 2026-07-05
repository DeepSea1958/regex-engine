#ifndef REGEX_API_H
#define REGEX_API_H

#include "dfa.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    REGEX_OK = 0,
    REGEX_ERR_NULL_ARGUMENT,
    REGEX_ERR_NO_MEMORY,
    REGEX_ERR_PARSE,
    REGEX_ERR_NFA_BUILD,
    REGEX_ERR_DFA_BUILD,
    REGEX_ERR_TOO_MANY_MATCHES
};

enum {
    REGEX_FLAG_NONE = 0
};

typedef struct regex_t {
    DFAMachine dfa;
    int flags;
    int error_code;
    char error_msg[256];
    char *pattern;
} regex_t;

regex_t *regex_compile(const char *pattern, int flags);
int regex_match(regex_t *prog, const char *text, MatchResult *result);
int regex_search(regex_t *prog, const char *text, MatchResult *result);
MatchResult *regex_findall(regex_t *prog, const char *text, int *count);
const char *regex_error(int err_code);
void regex_free(regex_t *prog);
void regex_findall_free(MatchResult *matches);

#ifdef __cplusplus
}
#endif

#endif /* REGEX_API_H */
