#include "matcher.h"
#include <stdio.h>
#include <string.h>

/* ========================================================================== */
/*  DFA 匹配器实现                                                              */
/*                                                                            */
/*  核心思路：DFA 就是一个状态机 —— 从起始状态出发，                            */
/*  逐字符查 transitions[] 表，走到接受状态即匹配成功。                          */
/*  每个字符只读一次，永不回溯，时间复杂度 O(n)。                               */
/* ========================================================================== */

/* -------------------------------------------------------------------------- */
/*  内部辅助：将 char 映射为 0..255 的索引                                      */
/* -------------------------------------------------------------------------- */

/**
 * 将输入字符转换为 0-255 范围内的索引。
 * 显式 cast 为 unsigned char 避免负数索引（signed char 扩展ASCII 场景）。
 */
static int char_to_index(char c) {
    return (unsigned char)c;
}

/* ========================================================================== */
/*  dfa_match — 精确匹配（整个输入字符串必须走完并接受）                         */
/* ========================================================================== */

MatchResult dfa_match(const DFAMachine *dfa, const char *input) {
    MatchResult result;
    memset(&result, 0, sizeof(result));

    /* 空输入：如果起始状态就是接受状态，也算匹配 */
    if (!input || *input == '\0') {
        result.start = 0;
        result.end = 0;
        result.length = 0;
        if (dfa->states[dfa->start_state].is_accept) {
            result.matched = 1;
        }
        return result;
    }

    int current_state = dfa->start_state;
    size_t start_pos = 0;
    size_t input_len = strlen(input);

    /* 逐字符在 DFA 上行走 */
    for (size_t i = 0; i < input_len; i++) {
        int idx = char_to_index(input[i]);

        /* 查转移表：-1 表示没有对应转移，匹配中断 */
        int next_state = dfa->states[current_state].transitions[idx];
        if (next_state == -1) {
            break;  /* 提前终止 */
        }

        current_state = next_state;
    }

    /* 填充结果 */
    result.start = start_pos;
    result.end = input_len;
    result.matched = dfa->states[current_state].is_accept;

    if (result.matched) {
        result.length = input_len;
    }

    return result;
}

/* ========================================================================== */
/*  dfa_dump — 打印 DFA 状态转移表（调试用）                                   */
/* ========================================================================== */

void dfa_dump(const DFAMachine *dfa) {
    if (!dfa) {
        printf("(null DFA)\n");
        return;
    }

    printf("========== DFA 状态转移表 ==========\n");
    printf("起始状态: %d\n", dfa->start_state);
    printf("状态总数: %d\n", dfa->state_count);
    printf("------------------------------------\n");

    for (int i = 0; i < dfa->state_count; i++) {
        const DFAState *state = &dfa->states[i];
        printf("状态 %d %s\n",
               state->id,
               state->is_accept ? "(接受)" : "");

        /* 仅打印有意义的转移 */
        int has_any = 0;
        for (int c = 0; c < 256; c++) {
            if (state->transitions[c] != -1) {
                if (!has_any) {
                    printf("  转移: ");
                    has_any = 1;
                }
                printf("[%c(0x%02x)->%d] ",
                       (c >= 32 && c < 127) ? c : '.',
                       c,
                       state->transitions[c]);
            }
        }
        if (has_any) {
            printf("\n");
        } else {
            printf("  (无转移)\n");
        }
    }

    printf("====================================\n");
}
