#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>

static char *generate_text_with_suffix(size_t len, char fill, const char *suffix) {
    size_t suffix_len = strlen(suffix);
    if (len < suffix_len) len = suffix_len;
    char *text = (char *)malloc(len + 1);
    if (text) {
        memset(text, fill, len);
        text[len] = '\0';
        memcpy(text + len - suffix_len, suffix, suffix_len);
    }
    return text;
}

int main(void) {
    typedef struct { const char *label; const char *pattern; char fill; const char *needle; } TestCase;

    const TestCase tests[] = {
        { "literal",     "hello",                                              '-', "hello" },
        { "plus",        "a+",                                                  'a', "a" },
        { "star",        "a*b",                                                 'a', "b" },
        { "digit-class", "[0-9]+",                                              '0', "123" },
        { NULL, NULL, 0, NULL },
    };

    size_t sizes[] = { 100, 1000 };
    const char *size_labels[] = { "100B", "1KB" };

    for (int ti = 0; tests[ti].pattern; ti++) {
        for (int si = 0; si < 2; si++) {
            size_t len = sizes[si];
            char *text = generate_text_with_suffix(len, tests[ti].fill, tests[ti].needle);
            if (!text) continue;

            regex_t prog;
            int rc = regcomp(&prog, tests[ti].pattern, REG_EXTENDED);
            printf("[%s] %-12s pattern='%s' size=%s text_tail='%s...%s'\n",
                   rc == 0 ? "OK" : "FAIL",
                   tests[ti].label, tests[ti].pattern, size_labels[si],
                   text, text + len - strlen(tests[ti].needle));

            if (rc == 0) {
                regmatch_t pm[1];
                rc = regexec(&prog, text, 1, pm, 0);
                printf("       regexec=%d rm_so=%d rm_eo=%d\n", rc, pm[0].rm_so, pm[0].rm_eo);
                regfree(&prog);
            }

            free(text);
        }
    }
    return 0;
}
