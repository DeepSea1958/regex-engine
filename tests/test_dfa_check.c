#include <stdio.h>
#include <string.h>
#include <regex.h>

#define regex_t engine_regex_t
#include "api.h"
#undef regex_t
#include "matcher.h"

int main(void) {
    engine_regex_t *eng = regex_compile("hello", REGEX_FLAG_NONE);
    if (!eng) {
        printf("Engine compile failed\n");
        return 1;
    }

    printf("Engine DFA: states=%d start_state=%d\n",
           eng->dfa.state_count, eng->dfa.start_state);

    char text[] = "----------hello";
    MatchResult r;
    int matched = regex_search(eng, text, &r);
    printf("Engine search: matched=%d start=%zu end=%zu\n", matched, r.start, r.end);

    regex_free(eng);
    return 0;
}
