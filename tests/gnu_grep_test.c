/**
 * gnu_grep_test.c - GNU grep 官方测试格式适配运行器
 *
 * 功能：
 *   - 解析 GNU grep 官方测试文件格式
 *   - 执行正则匹配测试
 *   - 生成详细的测试报告
 *
 * GNU grep 官方测试格式：
 *   status@pattern@input[@comment]
 *   - status: 0=不匹配, 1=匹配, 2=语法错误
 *
 * 示例：
 *   0@a@a
 *   1@a.*c@abc
 *   2@a(@EPAREN
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "api.h"

/* 统计数据 */
static struct {
    int total;
    int passed;
    int failed;
    int skipped;
} stats;

/* 文件统计 */
static struct {
    const char *filename;
    int total;
    int passed;
    int failed;
} file_stats[100];
static int file_count = 0;

/* 辅助函数：去除字符串首尾的空白 */
static char *trim_whitespace(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    
    return str;
}

/* 解析一行测试用例 */
static int parse_test_line(const char *line, int *status, char **pattern, char **input, char **comment) {
    if (!line || line[0] == '\0' || line[0] == '#') {
        return -1; /* 跳过空行或注释 */
    }
    
    char *line_copy = strdup(line);
    if (!line_copy) {
        perror("strdup");
        return -1;
    }
    
    char *ptr = line_copy;
    
    /* 解析 status */
    *status = atoi(ptr);
    
    /* 查找第一个 @ */
    ptr = strchr(ptr, '@');
    if (!ptr) {
        free(line_copy);
        return -1;
    }
    ptr++;
    
    /* 查找第二个 @ (分隔 pattern 和 input) */
    char *second_at = strchr(ptr, '@');
    if (!second_at) {
        free(line_copy);
        return -1;
    }
    *second_at = '\0';
    *pattern = strdup(ptr);
    if (!*pattern) {
        perror("strdup");
        free(line_copy);
        return -1;
    }
    
    /* 查找第三个 @ (分隔 input 和 comment) */
    ptr = second_at + 1;
    char *third_at = strchr(ptr, '@');
    if (third_at) {
        *third_at = '\0';
        *comment = strdup(third_at + 1);
        if (!*comment) {
            perror("strdup");
            free(*pattern);
            free(line_copy);
            return -1;
        }
    } else {
        *comment = NULL;
    }
    *input = strdup(ptr);
    if (!*input) {
        perror("strdup");
        free(*pattern);
        if (*comment) free(*comment);
        free(line_copy);
        return -1;
    }
    
    free(line_copy);
    return 0;
}

/* 执行单个测试用例 */
static int run_test_case(const char *filename, int line_num, int expected_status, const char *pattern, const char *input, const char *comment) {
    stats.total++;
    
    printf("  [Line %4d] %s@%s@%s", line_num, expected_status == 0 ? "0" : expected_status == 1 ? "1" : "2", pattern, input);
    if (comment && comment[0]) {
        printf("@%s", comment);
    }
    printf("\n");
    
    int actual_status;
    regex_t *prog = regex_compile(pattern, REGEX_FLAG_NONE);
    
    if (!prog) {
        actual_status = 2; /* 编译失败视为语法错误 */
    } else {
        MatchResult result;
        int matched = regex_search(prog, input, &result);
        actual_status = matched ? 1 : 0;
        regex_free(prog);
    }
    
    if (actual_status == expected_status) {
        stats.passed++;
        printf("    PASS: Expected %d, Got %d\n", expected_status, actual_status);
        return 1;
    } else {
        stats.failed++;
        printf("    FAIL: Expected %d, Got %d\n", expected_status, actual_status);
        return 0;
    }
}

/* 处理一个测试文件 */
static void process_test_file(const char *filename) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  GNU grep 官方测试: %-32s  ║\n", filename);
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        printf("  ERROR: Cannot open file %s\n", filename);
        return;
    }
    
    char line[4096];
    int line_num = 0;
    int file_total = 0;
    int file_passed = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        char *trimmed = trim_whitespace(line);
        
        int status;
        char *pattern = NULL;
        char *input = NULL;
        char *comment = NULL;
        
        if (parse_test_line(trimmed, &status, &pattern, &input, &comment) == 0) {
            file_total++;
            if (run_test_case(filename, line_num, status, pattern, input, comment)) {
                file_passed++;
            }
            
            free(pattern);
            free(input);
            if (comment) free(comment);
        }
    }
    
    fclose(file);
    
    /* 保存文件统计 */
    if (file_count < 100) {
        file_stats[file_count].filename = filename;
        file_stats[file_count].total = file_total;
        file_stats[file_count].passed = file_passed;
        file_stats[file_count].failed = file_total - file_passed;
        file_count++;
    }
    
    printf("\n  ------------\n");
    printf("  File %s: %d tests, %d passed, %d failed (%.1f%%)\n", filename, file_total, file_passed, file_total - file_passed,
           file_total > 0 ? (float)file_passed / file_total * 100 : 0.0);
}

/* 打印最终总结 */
static void print_summary(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║                    GNU grep 官方测试总结                            ║\n");
    printf("╠═══════════════════════════════════════════════════════════════════╣\n");
    
    for (int i = 0; i < file_count; i++) {
        float pass_rate = file_stats[i].total > 0 ? (float)file_stats[i].passed / file_stats[i].total * 100 : 0.0;
        printf("║  %-40s: %3d/%3d (%.1f%%)  ║\n",
               file_stats[i].filename, file_stats[i].passed, file_stats[i].total, pass_rate);
    }
    
    printf("╠═══════════════════════════════════════════════════════════════════╣\n");
    float overall_rate = stats.total > 0 ? (float)stats.passed / stats.total * 100 : 0.0;
    printf("║  Overall: %4d total, %4d passed, %4d failed  ║\n", stats.total, stats.passed, stats.failed);
    printf("║  Pass rate: %6.1f%%  ║\n", overall_rate);
    
    if (overall_rate >= 90.0) {
        printf("║  Status: ✅ TARGET ACHIEVED (>= 90%%)  ║\n");
    } else {
        printf("║  Status: ⚠️ NEED IMPROVEMENT (< 90%%)  ║\n");
    }
    
    printf("╚═══════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    
    memset(&stats, 0, sizeof(stats));
    file_count = 0;
    
    printf("╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║               GNU grep 官方测试适配运行器                          ║\n");
    printf("╠═══════════════════════════════════════════════════════════════════╣\n");
    printf("║  Usage: gnu_grep_test <testfile1> [<testfile2> ...]  ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════╝\n");
    
    if (argc < 2) {
        printf("\nError: No test files specified!\n");
        printf("\nExample: gnu_grep_test tests/grep_tests/bre.tests\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        process_test_file(argv[i]);
    }
    
    print_summary();
    
    float overall_rate = stats.total > 0 ? (float)stats.passed / stats.total * 100 : 0.0;
    return overall_rate >= 90.0 ? 0 : 1;
}
