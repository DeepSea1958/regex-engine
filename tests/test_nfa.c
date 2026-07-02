#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <locale.h>
#endif
#include <stdlib.h>
#include "nfa.h"
#include "parser.h"

/* ========================================================================== */
/*  测试框架（与 test_tokenizer.c / test_parser.c 一致）                        */
/* ========================================================================== */

static int g_passes = 0;
static int g_failures = 0;
static int g_module_passes = 0;
static int g_module_failures = 0;

static void module_begin(const char *name) {
    printf("\n══════════════════════════════════════\n");
    printf("  %s\n", name);
    printf("──────────────────────────────────────\n");
    g_module_passes = 0;
    g_module_failures = 0;
}

static void module_end(void) {
    printf("──────────────────────────────────────\n");
    if (g_module_failures == 0) {
        printf("  结果：全部通过 (%d 项)\n", g_module_passes);
    } else {
        printf("  结果：通过 %d 项，失败 %d 项\n", g_module_passes, g_module_failures);
    }
}

static void check_pass(const char *desc) {
    printf("  ✓ %s\n", desc);
    g_passes++;
    g_module_passes++;
}

static void check_fail(const char *desc, const char *expected, const char *actual) {
    printf("  ✗ %s —— 期望「%s」，实际「%s」\n", desc, expected, actual);
    g_failures++;
    g_module_failures++;
}

/* ========================================================================== */
/*  NFA 验证辅助                                                               */
/* ========================================================================== */

/** 解析正则并构造 NFA，失败时返回零初始化的图 */
static NFAGraph do_build(const char *pattern) {
    Parser parser;
    parser_init(&parser, pattern);
    ASTNode *ast = parser_parse(&parser);
    if (!ast) {
        printf("  ! 前置解析失败: %s\n", parser.error_msg);
        NFAGraph nfa = {0};
        return nfa;
    }
    NFAGraph nfa = nfa_from_ast(ast);
    ast_free(ast);
    return nfa;
}

/* ========================================================================== */
/*  断言宏                                                                      */
/* ========================================================================== */

#define CHECK_INT(expected, actual, desc) \
    do { \
        if ((int)(expected) != (int)(actual)) { \
            char exp[32], act[32]; \
            snprintf(exp, sizeof(exp), "%d", (int)(expected)); \
            snprintf(act, sizeof(act), "%d", (int)(actual)); \
            check_fail(desc, exp, act); \
        } else { \
            check_pass(desc); \
        } \
    } while (0)

#define CHECK_NOT_NULL(ptr, desc) \
    do { \
        if ((ptr) != NULL) { \
            check_pass(desc); \
        } else { \
            check_fail(desc, "非 NULL", "NULL"); \
        } \
    } while (0)

#define CHECK_NULL(ptr, desc) \
    do { \
        if ((ptr) == NULL) { \
            check_pass(desc); \
        } else { \
            check_fail(desc, "NULL", "非 NULL"); \
        } \
    } while (0)

#define CHECK_EDGE_TYPE(expected, actual, desc) \
    do { \
        if ((int)(expected) == (int)(actual)) { \
            check_pass(desc); \
        } else { \
            char exp[32], act[32]; \
            snprintf(exp, sizeof(exp), "%d", (int)(expected)); \
            snprintf(act, sizeof(act), "%d", (int)(actual)); \
            check_fail(desc, exp, act); \
        } \
    } while (0)

/* ========================================================================== */
/*  测试用例                                                                    */
/* ========================================================================== */

/* ==================================== */
/*  原子节点                             */
/* ==================================== */

static void test_atom_char(void) {
    NFAGraph nfa = do_build("a");
    CHECK_NOT_NULL(nfa.start, "起始状态非 NULL");
    CHECK_NOT_NULL(nfa.end,   "接受状态非 NULL");
    CHECK_INT(2, nfa.state_count, "单个字符 a — 2 个状态");
    CHECK_EDGE_TYPE(NFA_EDGE_CHAR, nfa.start->edge1_type, "起始边为 CHAR 类型");
    if (nfa.start->edge1_type == NFA_EDGE_CHAR && nfa.start->edge1_char == 'a')
        check_pass("边上的字符为 'a'");
    else
        check_fail("边上的字符为 'a'", "'a'", "?");
    CHECK_NOT_NULL(nfa.start->edge1_next, "边指向接受状态");
    nfa_free(&nfa);
}

static void test_atom_dot(void) {
    NFAGraph nfa = do_build(".");
    CHECK_INT(2, nfa.state_count, "点号 — 2 个状态");
    CHECK_EDGE_TYPE(NFA_EDGE_DOT, nfa.start->edge1_type, "起始边为 DOT 类型");
    nfa_free(&nfa);
}

static void test_atom_escape(void) {
    NFAGraph nfa = do_build("\\d");
    CHECK_INT(2, nfa.state_count, "\\d — 2 个状态");
    CHECK_EDGE_TYPE(NFA_EDGE_ESCAPE, nfa.start->edge1_type, "起始边为 ESCAPE 类型");
    if (nfa.start->edge1_esc == ESCAPE_DIGIT)
        check_pass("转义类型为 ESCAPE_DIGIT");
    else
        check_fail("转义类型为 ESCAPE_DIGIT", "ESCAPE_DIGIT", "?");
    nfa_free(&nfa);
}

static void test_atom_bracket(void) {
    NFAGraph nfa = do_build("[abc]");
    CHECK_INT(2, nfa.state_count, "[abc] — 2 个状态");
    CHECK_EDGE_TYPE(NFA_EDGE_BRACKET, nfa.start->edge1_type, "起始边为 BRACKET 类型");
    if (nfa.start->edge1_bracket.len == 3 &&
        strncmp(nfa.start->edge1_bracket.str, "abc", 3) == 0)
        check_pass("字符集合内容为 \"abc\"");
    else
        check_fail("字符集合内容为 \"abc\"", "\"abc\"", "?");
    nfa_free(&nfa);
}

/* ==================================== */
/*  连接 CONCAT                          */
/* ==================================== */

static void test_concat_two(void) {
    /* ab: 原子 a (2) + 原子 b (2) = 4，连接不增加状态 */
    NFAGraph nfa = do_build("ab");
    CHECK_INT(4, nfa.state_count, "ab — 4 个状态（不引入新状态）");
    CHECK_NOT_NULL(nfa.start, "入口为 a 的起始状态");
    CHECK_NOT_NULL(nfa.end,   "出口为 b 的结束状态");

    /* a 的出口 (id=1) 通过 ε 边连到 b 的入口 (id=2) */
    NFAState *a_end = nfa.start->edge1_next;  /* a 的出口 */
    CHECK_NOT_NULL(a_end, "a 的出口存在");
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, a_end->edge1_type, "a.end 的 e1 为 ε 连接边");
    CHECK_NOT_NULL(a_end->edge1_next, "ε 边指向 b 的入口");
    nfa_free(&nfa);
}

static void test_concat_many(void) {
    /* abc: 每个字符 2 状态，3 个字符 = 6 状态 */
    NFAGraph nfa = do_build("abc");
    CHECK_INT(6, nfa.state_count, "abc — 6 个状态");
    CHECK_NOT_NULL(nfa.start, "入口非 NULL");
    CHECK_NOT_NULL(nfa.end,   "出口非 NULL");
    nfa_free(&nfa);
}

/* ==================================== */
/*  并集 ALTER                           */
/* ==================================== */

static void test_alter_simple(void) {
    /* a|b: 原子 a (2) + 原子 b (2) + 新起止 (2) = 6 */
    NFAGraph nfa = do_build("a|b");
    CHECK_INT(6, nfa.state_count, "a|b — 6 个状态");
    CHECK_NOT_NULL(nfa.start, "入口为新增分裂状态");
    CHECK_NOT_NULL(nfa.end,   "出口为新增汇聚状态");

    /* 入口（分裂状态）：两条 ε 出边 */
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, nfa.start->edge1_type, "分裂状态 e1 为 ε");
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, nfa.start->edge2_type, "分裂状态 e2 为 ε");
    CHECK_NOT_NULL(nfa.start->edge1_next, "e1 指向 a 的入口");
    CHECK_NOT_NULL(nfa.start->edge2_next, "e2 指向 b 的入口");
    nfa_free(&nfa);
}

static void test_alter_three(void) {
    /* a|b|c: 3 个原子各 2 + 2 个 ALTER 各引入 2 个新状态 = 6+4 = 10 */
    NFAGraph nfa = do_build("a|b|c");
    CHECK_INT(10, nfa.state_count, "a|b|c — 10 个状态");
    CHECK_NOT_NULL(nfa.start, "入口非 NULL");
    CHECK_NOT_NULL(nfa.end,   "出口非 NULL");
    nfa_free(&nfa);
}

/* ==================================== */
/*  星号 STAR                            */
/* ==================================== */

static void test_star(void) {
    /* a*: 原子 a (2) + 新起止 (2) = 4 */
    NFAGraph nfa = do_build("a*");
    CHECK_INT(4, nfa.state_count, "a* — 4 个状态");

    /* 入口（分裂状态）：e1 ε→ a.start, e2 ε→ ne（bypass） */
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, nfa.start->edge1_type, "e1 为 ε（进入子图）");
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, nfa.start->edge2_type, "e2 为 ε（绕行）");
    CHECK_NOT_NULL(nfa.start->edge1_next, "e1 指向子图入口");
    CHECK_NOT_NULL(nfa.start->edge2_next, "e2 指向出口（bypass）");

    /* a 的出口：e1 回到 a.start（loop），e2 → ne（退出） */
    nfa_free(&nfa);
}

/* ==================================== */
/*  加号 PLUS                            */
/* ==================================== */

static void test_plus(void) {
    /* a+: 原子 a (2) + 新起止 (2) = 4 */
    NFAGraph nfa = do_build("a+");
    CHECK_INT(4, nfa.state_count, "a+ — 4 个状态");

    /* 入口只有一条 ε 出边（无 bypass） */
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, nfa.start->edge1_type, "e1 为 ε（进入子图）");
    CHECK_NULL(nfa.start->edge2_next, "e2 为空（无 bypass — 至少匹配一次）");
    nfa_free(&nfa);
}

/* ==================================== */
/*  问号 QUESTION                        */
/* ==================================== */

static void test_question(void) {
    /* a?: 原子 a (2) + 新起止 (2) = 4 */
    NFAGraph nfa = do_build("a?");
    CHECK_INT(4, nfa.state_count, "a? — 4 个状态");

    /* 入口有 bypass，出口无循环边 */
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, nfa.start->edge1_type, "e1 为 ε（进入子图）");
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, nfa.start->edge2_type, "e2 为 ε（bypass）");
    nfa_free(&nfa);
}

/* ==================================== */
/*  范围量词 CURLY                        */
/* ==================================== */

static void test_curly_exact(void) {
    /* a{3}: 3 个原子 a 副本 = 6 状态 */
    NFAGraph nfa = do_build("a{3}");
    CHECK_INT(6, nfa.state_count, "a{3} — 6 个状态");
    CHECK_NOT_NULL(nfa.start, "入口非 NULL");
    CHECK_NOT_NULL(nfa.end,   "出口非 NULL");
    nfa_free(&nfa);
}

static void test_curly_range(void) {
    /* a{2,4}: 2 个强制副本 (4) + 2 个可选副本 (4) = 8 */
    NFAGraph nfa = do_build("a{2,4}");
    CHECK_INT(8, nfa.state_count, "a{2,4} — 8 个状态");
    nfa_free(&nfa);
}

static void test_curly_open(void) {
    /* a{1,}: 1 个强制副本 (2) + a* (2+2=4) = 6
       注意 a* 部分：原子 a (2) + 新起止 (2) = 4，但这里 a{1,} 的 a* 还会把
       star 子图包裹在分裂/汇聚状态中，总数 = 2(强制a) + 4(a*的a+起止) = 6 */
    NFAGraph nfa = do_build("a{1,}");
    CHECK_NOT_NULL(nfa.start, "a{1,} — 入口非 NULL");
    CHECK_NOT_NULL(nfa.end,   "a{1,} — 出口非 NULL");
    CHECK_INT(6, nfa.state_count, "a{1,} — 6 个状态");
    nfa_free(&nfa);
}

static void test_curly_zero_min(void) {
    /* a{0,3}: 0 个强制副本 (ε占位 2) + 3 个可选副本 (a×3=6) = 8 */
    NFAGraph nfa = do_build("a{0,3}");
    CHECK_INT(8, nfa.state_count, "a{0,3} — 8 个状态");
    nfa_free(&nfa);
}

static void test_curly_zero_unbounded(void) {
    /* a{0,}: 0 个强制副本 (ε占位 2) + a* (2+2=4) = 6
       这里 a* 是一个完整的星号构造：原子 a (2) + 分裂/汇聚 (2) */
    NFAGraph nfa = do_build("a{0,}");
    CHECK_INT(6, nfa.state_count, "a{0,} — 6 个状态");
    CHECK_NOT_NULL(nfa.start, "入口非 NULL");
    CHECK_NOT_NULL(nfa.end,   "出口非 NULL");
    nfa_free(&nfa);
}

/* ==================================== */
/*  捕获组 GROUP                         */
/* ==================================== */

static void test_group_passthrough(void) {
    /* (a): 组不增加状态 — 透传内部原子 a 的 2 个状态 */
    NFAGraph nfa = do_build("(a)");
    CHECK_INT(2, nfa.state_count, "(a) — 2 个状态（组不增加开销）");
    CHECK_EDGE_TYPE(NFA_EDGE_CHAR, nfa.start->edge1_type, "内部为 CHAR 边");
    nfa_free(&nfa);
}

static void test_group_with_alter(void) {
    /* (a|b): 组 (0) + ALTER a|b (6) = 6 */
    NFAGraph nfa = do_build("(a|b)");
    CHECK_INT(6, nfa.state_count, "(a|b) — 6 个状态");
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, nfa.start->edge1_type, "入口为 ALTER 的分裂状态");
    nfa_free(&nfa);
}

/* ==================================== */
/*  复杂组合                              */
/* ==================================== */

static void test_complex_concat_star(void) {
    /* ab*: a(2) + b*(2+2=4) = 6 */
    NFAGraph nfa = do_build("ab*");
    CHECK_INT(6, nfa.state_count, "ab* — 6 个状态");
    CHECK_NOT_NULL(nfa.start, "入口非 NULL");
    CHECK_NOT_NULL(nfa.end,   "出口非 NULL");
    nfa_free(&nfa);
}

static void test_complex_alter_concat(void) {
    /* ab|cd: a(2)+b(2)=4 + c(2)+d(2)=4 + ALTER起止(2) = 10 */
    NFAGraph nfa = do_build("ab|cd");
    CHECK_INT(10, nfa.state_count, "ab|cd — 10 个状态");
    nfa_free(&nfa);
}

static void test_complex_group_star(void) {
    /* (ab)*: ab(4) + STAR起止(2) = 6 */
    NFAGraph nfa = do_build("(ab)*");
    CHECK_INT(6, nfa.state_count, "(ab)* — 6 个状态");
    /* 入口应该是星号的分裂状态（e1→ab, e2→出口 bypass） */
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, nfa.start->edge1_type, "入口 e1 为 ε");
    CHECK_EDGE_TYPE(NFA_EDGE_EPSILON, nfa.start->edge2_type, "入口 e2 为 ε (bypass)");
    nfa_free(&nfa);
}

static void test_complex_nested_quant(void) {
    /* a(b|c)*d — 经典场景
        a(2) + [b(2)+c(2)+ALTER起止(2)=6 + STAR起止(2)=8] + d(2)
        连接: a + (b|c)* + d = 2+8+2 - 不引入新状态 = 12 */
    NFAGraph nfa = do_build("a(b|c)*d");
    CHECK_INT(12, nfa.state_count, "a(b|c)*d — 12 个状态");
    CHECK_NOT_NULL(nfa.start, "入口非 NULL");
    CHECK_NOT_NULL(nfa.end,   "出口非 NULL");
    nfa_free(&nfa);
}

static void test_complex_escape_curly(void) {
    /* \d{2,4}[a-z]?
        \d(2) + \d(2) + \d可选(2) + \d可选(2) + [a-z]?(2+2=4)
        用 {2,4} → 2强制+2可选 = 4+4=8, [a-z]? = 4 → 总计 12 */
    NFAGraph nfa = do_build("\\d{2,4}[a-z]?");
    CHECK_INT(12, nfa.state_count, "\\d{2,4}[a-z]? — 12 个状态");
    nfa_free(&nfa);
}

/* ==================================== */
/*  边界情况                              */
/* ==================================== */

static void test_null_input(void) {
    NFAGraph nfa = nfa_from_ast(NULL);
    CHECK_NULL(nfa.start,   "NULL AST — start 为 NULL");
    CHECK_NULL(nfa.end,     "NULL AST — end 为 NULL");
    CHECK_NULL(nfa.states,  "NULL AST — states 为 NULL");
    CHECK_INT(0, nfa.state_count, "NULL AST — state_count 为 0");
    nfa_free(&nfa);
}

static void test_double_free_safe(void) {
    /* 验证 double-free 不会崩溃 */
    NFAGraph nfa = do_build("a");
    nfa_free(&nfa);
    nfa_free(&nfa);  /* 第二次 free 应是安全的空操作 */
    check_pass("double-free — 安全无崩溃");
}

static void test_bracket_content_copy(void) {
    /* 验证 bracket 内容被正确拷贝（不是浅拷贝） */
    NFAGraph nfa = do_build("[xyz]");
    CHECK_NOT_NULL(nfa.start->edge1_bracket.str, "bracket.str 非 NULL");
    if (nfa.start->edge1_bracket.str &&
        strncmp(nfa.start->edge1_bracket.str, "xyz", 3) == 0)
        check_pass("bracket 内容为 \"xyz\"（独立拷贝）");
    else
        check_fail("bracket 内容为 \"xyz\"", "\"xyz\"", "?");
    nfa_free(&nfa);
}

static void test_escapes_all(void) {
    /* 验证全部 6 种转义序列 */
    static const char *patterns[] = {"\\d","\\D","\\w","\\W","\\s","\\S"};
    static EscapeSeq expected[] = {
        ESCAPE_DIGIT, ESCAPE_NON_DIGIT,
        ESCAPE_WORD,  ESCAPE_NON_WORD,
        ESCAPE_SPACE, ESCAPE_NON_SPACE,
    };

    for (int i = 0; i < 6; i++) {
        NFAGraph nfa = do_build(patterns[i]);
        char desc[64];
        snprintf(desc, sizeof(desc), "%s — 转义类型正确", patterns[i]);
        if (nfa.start->edge1_esc == expected[i])
            check_pass(desc);
        else {
            char act[8]; snprintf(act, sizeof(act), "%d", nfa.start->edge1_esc);
            check_fail(desc, patterns[i], act);
        }
        nfa_free(&nfa);
    }
}

static void test_state_ids_unique(void) {
    /* 验证所有状态 id 唯一 */
    NFAGraph nfa = do_build("a|b*");
    int *seen = calloc(nfa.state_count, sizeof(int));
    int all_unique = 1;

    for (int i = 0; i < nfa.state_count; i++) {
        int id = nfa.states[i]->id;
        if (id < 0 || id >= nfa.state_count || seen[id]) {
            all_unique = 0;
            break;
        }
        seen[id] = 1;
    }

    if (all_unique)
        check_pass("所有状态 id 唯一");
    else
        check_fail("所有状态 id 唯一", "唯一", "重复");

    free(seen);
    nfa_free(&nfa);
}

static void test_start_not_end_for_nonempty(void) {
    /* 验证非空 NFA 的 start ≠ end（保证有实际匹配内容） */
    NFAGraph nfa = do_build("a");
    if (nfa.start != nfa.end)
        check_pass("单字符 — start ≠ end（有实际匹配边）");
    else
        check_fail("单字符 — start ≠ end", "start ≠ end", "start == end");
    nfa_free(&nfa);
}

static void test_nfa_dump_null(void) {
    /* 验证 nfa_dump 对 NULL 安全 */
    nfa_dump(NULL);    /* 不应崩溃 */
    check_pass("nfa_dump(NULL) — 安全");
}

/* ==================================== */
/*  量词状态数一致性验证                    */
/* ==================================== */

static void test_quant_state_counts(void) {
    /* 对比量词的状态数：a* (4) == a+ (4) == a? (4) — 三者都引入 2 个新状态 */
    NFAGraph star_nfa = do_build("a*");
    NFAGraph plus_nfa = do_build("a+");
    NFAGraph q_nfa    = do_build("a?");

    CHECK_INT(4, star_nfa.state_count, "a* — 4 个状态");
    CHECK_INT(4, plus_nfa.state_count, "a+ — 4 个状态");
    CHECK_INT(4, q_nfa.state_count,    "a? — 4 个状态");

    nfa_free(&star_nfa);
    nfa_free(&plus_nfa);
    nfa_free(&q_nfa);
}

/* ==================================== */
/*  量词叠加                              */
/* ==================================== */

static void test_double_quant(void) {
    /* (a*)* : a(2) + *起止(2) + *起止(2) = 6 */
    NFAGraph nfa = do_build("(a*)*");
    CHECK_INT(6, nfa.state_count, "(a*)* — 6 个状态");
    nfa_free(&nfa);
}

static void test_star_plus_question_chain(void) {
    /* (a*)?+  — parser 不支持量词直接相邻，改用 (a*)? 验证量词叠加：
       a(2) + *(2) + ?(2) = 6 */
    NFAGraph nfa = do_build("(a*)?");
    CHECK_INT(6, nfa.state_count, "(a*)? — 6 个状态（量词链式叠加）");
    nfa_free(&nfa);
}

/* ========================================================================== */
/*  主函数                                                                      */
/* ========================================================================== */

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF-8");
#endif

    printf("╔══════════════════════════════════════╗\n");
    printf("║   NFA Thompson 构造 单元测试          ║\n");
    printf("╚══════════════════════════════════════╝\n");

    /* ---- 原子 ---- */
    module_begin("原子节点");
    test_atom_char();
    test_atom_dot();
    test_atom_escape();
    test_atom_bracket();
    module_end();

    /* ---- 连接 ---- */
    module_begin("连接 (CONCAT)");
    test_concat_two();
    test_concat_many();
    module_end();

    /* ---- 并集 ---- */
    module_begin("并集 (ALTER)");
    test_alter_simple();
    test_alter_three();
    module_end();

    /* ---- 量词：单个 ---- */
    module_begin("量词 — 星号 / 加号 / 问号");
    test_star();
    test_plus();
    test_question();
    test_quant_state_counts();
    module_end();

    /* ---- 量词：范围 ---- */
    module_begin("量词 — 范围 {m,n}");
    test_curly_exact();
    test_curly_range();
    test_curly_open();
    test_curly_zero_min();
    test_curly_zero_unbounded();
    module_end();

    /* ---- 捕获组 ---- */
    module_begin("捕获组（括号）");
    test_group_passthrough();
    test_group_with_alter();
    module_end();

    /* ---- 复杂组合 ---- */
    module_begin("复杂组合");
    test_complex_concat_star();
    test_complex_alter_concat();
    test_complex_group_star();
    test_complex_nested_quant();
    test_complex_escape_curly();
    module_end();

    /* ---- 量词叠加 ---- */
    module_begin("量词叠加");
    test_double_quant();
    test_star_plus_question_chain();
    module_end();

    /* ---- 边界与安全 ---- */
    module_begin("边界与安全");
    test_null_input();
    test_double_free_safe();
    test_bracket_content_copy();
    test_escapes_all();
    test_state_ids_unique();
    test_start_not_end_for_nonempty();
    test_nfa_dump_null();
    module_end();

    /* ---- 总结果 ---- */
    printf("\n╔══════════════════════════════════════╗\n");
    if (g_failures == 0) {
        printf("║  测试全部通过！                      ║\n");
    } else {
        printf("║  测试存在失败                        ║\n");
    }
    printf("║  总计：%3d 项                        ║\n", g_passes + g_failures);
    printf("║  通过：%3d 项                        ║\n", g_passes);
    if (g_failures > 0) {
        printf("║  失败：%3d 项                        ║\n", g_failures);
    }
    printf("╚══════════════════════════════════════╝\n");

    return g_failures > 0 ? 1 : 0;
}
