#include <stdio.h>
#include <string.h>
#include <regex.h>

int main(void) {
    regex_t prog;
    memset(&prog, 0, sizeof(prog));

    if (regcomp(&prog, "hello", REG_EXTENDED) != 0) {
        printf("regcomp failed\n");
        return 1;
    }

    char text[] = "----------hello";
    printf("sizeof(regmatch_t) = %zu\n", sizeof(regmatch_t));
    printf("sizeof(regoff_t)   = %zu\n", sizeof(regoff_t));

    /* Test with array of 1 */
    regmatch_t pm[1];
    memset(pm, 0xFF, sizeof(pm));  /* Fill with known pattern */
    int ret = regexec(&prog, text, 1, pm, 0);
    printf("regexec=%d pm[0]={rm_so=%d, rm_eo=%d}\n", ret, pm[0].rm_so, pm[0].rm_eo);

    /* Test with array of 2 */
    regmatch_t pm2[2];
    memset(pm2, 0xFF, sizeof(pm2));
    ret = regexec(&prog, text, 2, pm2, 0);
    printf("regexec=%d pm2[0]={rm_so=%d, rm_eo=%d}\n", ret, pm2[0].rm_so, pm2[0].rm_eo);
    printf("             pm2[1]={rm_so=%d, rm_eo=%d}\n", pm2[1].rm_so, pm2[1].rm_eo);

    /* Raw bytes */
    printf("Raw bytes of pm2[0]: ");
    unsigned char *bytes = (unsigned char *)&pm2[0];
    for (int i = 0; i < (int)sizeof(pm2[0]); i++) {
        printf("%02x ", bytes[i]);
    }
    printf("\n");

    regfree(&prog);
    return 0;
}
