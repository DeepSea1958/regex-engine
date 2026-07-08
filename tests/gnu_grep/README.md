# GNU grep 官方测试套件

此目录包含来自 GNU grep 官方仓库的测试文件。

## 测试文件说明

| 文件名 | 说明 |
|--------|------|
| `bre.tests` | 基础正则表达式 (BRE) 测试 |
| `ere.tests` | 扩展正则表达式 (ERE) 测试 |
| `spencer1.tests` | Henry Spencer 的经典测试套件 |

## 测试文件格式

```
status@pattern@input[@comment]
```

- **status**: 
  - `0` = 不匹配
  - `1` = 匹配
  - `2` = 语法错误

## 构建和运行

### 1. 构建项目

```bash
cd regex-engine
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

### 2. 运行官方测试

#### 使用 CMake 目标（推荐）

```bash
cmake --build . --target run_gnu_grep_tests
```

#### 直接运行

```bash
# 运行单个测试文件
./bin/gnu_grep_test ../tests/gnu_grep/bre.tests

# 运行多个测试文件
./bin/gnu_grep_test ../tests/gnu_grep/bre.tests ../tests/gnu_grep/ere.tests
```

## 项目目标

通过 **90% 以上** 的官方测试用例！

## 测试来源

所有测试文件来自 GNU grep 官方仓库：
- 仓库: https://git.savannah.gnu.org/cgit/grep.git/
- GitHub 镜像: https://github.com/GitMirroring/grep

## 许可证

GNU GPL v3，与 GNU grep 相同。
