# TASK_XXX: <任务名称>

**任务编号**: TASK_XXX  
**关联计划**: [PLAN_XXX_<名称>](../plans/PLAN_XXX_<名称>.md)  
**优先级**: HIGH / MEDIUM / LOW  
**状态**: Not Started / In Progress / Review / Completed  
**创建日期**: YYYY-MM-DD  
**预计完成**: YYYY-MM-DD  

---

## 实现思路

用 2-3 段说明大的实现方向和核心设计决策。不涉及细节代码，但要说明"为什么这样设计"。

例如：
- 使用 CMake 作为跨平台构建系统，便于在 Linux、macOS、Windows 上构建
- 采用接口抽象模式（Strategy Pattern）设计 `IJudge`，支持 MockJudge 和 LLMJudge 的实现互换
- MockJudge 使用规则型启发式方法，无需外部 API 即可快速离线评估
- 数据模型中分离 Accuracy 和 Helpfulness，使评估更加细粒度

## 实现步骤

具体分步实现细节。每一步要清晰、可验证。

1. **编写 CMakeLists.txt 顶级配置**
   - 设置 C++17 标准
   - 配置 include 目录
   - 链接必要的系统库（libcurl, pthread 等）
   - 定义构建目标 `reply_eval`

2. **创建头文件框架**
   - `include/config.h`：配置常量和选项
   - `include/models.h`：所有数据结构定义
   - `include/judge.h`：IJudge 抽象基类
   - `include/evaluator.h`：评估核心逻辑

3. **实现 main.cpp 的 CLI 参数解析**
   - 支持 `--mode mock|llm`
   - 支持 `--input` 指定输入 JSON 文件
   - 支持 `--reference` 指定参考文件
   - 支持 `--output` 指定输出报告路径
   - 实现 `--help` 显示使用说明

4. **实现 JSON 加载器** (`src/json_loader.cpp`)
   - 加载 auto_replies.json，验证必填字段
   - 加载 human_ref.json，验证必填字段
   - 处理缺失的参考文件（给出 warning 但继续）
   - 返回对应的数据结构

5. **创建 IJudge 抽象接口**
   - 定义纯虚函数 `evaluate(const ReplyCase&, const ReferenceCase&)`
   - 返回 `EvaluationScore` 结构体

6. **实现 MockJudge**
   - 基于关键词和规则判断各项分数
   - 记录判断理由和潜在幻觉声明
   - 确保输出稳定、可复现

7. **本地构建和测试**
   - 构建成功，无编译错误
   - 运行 `./reply_eval --help` 验证
   - 加载 JSON 文件验证

## 第三方库

列出需要的外部依赖、最低版本要求、用途和是否可选。

| 库名 | 最低版本 | 用途 | 是否可选 | 系统检查 |
|---|---|---|---|---|
| nlohmann/json | 3.11 | JSON 解析和序列化 | 否 | `find_package()` 或手动指定路径 |
| libcurl | 7.68 | HTTP 请求（仅 LLMJudge 模式） | 是（仅 LLM 模式需要） | `find_package(CURL)` |

**安装说明**（若需）：

```bash
# Ubuntu/Debian
sudo apt-get install nlohmann-json3-dev libcurl4-openssl-dev

# macOS
brew install nlohmann-json curl
```

## 修改代码清单

要修改或创建的源文件及其关键内容。

### 新增文件

#### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.16)
project(reply_evaluator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 查找依赖
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED)

# 定义可执行文件
add_executable(reply_eval
    src/main.cpp
    src/json_loader.cpp
    src/evaluator.cpp
)

target_include_directories(reply_eval PRIVATE include)
target_link_libraries(reply_eval PRIVATE nlohmann_json::nlohmann_json)
```

#### include/models.h
- 结构体 `EvaluationScore`（accuracy, helpfulness, tone, hallucination, reason, hallucinated_claims）
- 结构体 `ReplyCase`（id, user_question, auto_reply）
- 结构体 `ReferenceCase`（id, human_reference, annotator_notes）
- 结构体 `EvaluationResult`（id, score, final_score, critical_failure, judge_reason）

#### include/judge.h
- 抽象基类 `IJudge`
- 纯虚方法 `evaluate()`

#### include/evaluator.h
- 函数 `calculateFinalScore()`
- 函数 `isCriticalFailure()`
- 类 `Evaluator`（管理评估流程）

#### include/config.h
- 常量定义（权重值、分数范围等）
- 枚举 `JudgeMode`（MOCK, LLM）

#### src/main.cpp
- 命令行参数解析
- 程序入口和流程控制
- 基础的错误处理

#### src/json_loader.cpp
- 函数 `loadReplies()`
- 函数 `loadReferences()`
- JSON 解析和验证逻辑

#### src/evaluator.cpp
- 函数实现 `calculateFinalScore()`
- 函数实现 `isCriticalFailure()`
- 主流程编排

### 修改文件

（暂无）

## 创建文件清单

新建文件的详细设计说明。

### CMakeLists.txt
```text
- 顶级 CMake 配置
- 包含 C++17 标准设置
- 链接 nlohmann_json 和 libcurl（可选）
- 生成二进制文件 reply_eval 到 build/ 目录
```

### include/models.h
```text
- 定义 4 个核心结构体
- 所有字段都应该有默认值或初始化
- 无业务逻辑，仅用于数据传递
```

### include/judge.h
```text
- 抽象基类 IJudge，定义评估接口
- 纯虚函数 evaluate() 的签名
- 虚析构函数
```

### include/evaluator.h
```text
- 权重常量定义
- 加权计算公式实现
- 关键失败判定逻辑
```

### src/main.cpp
```text
- 函数 parseArguments() 处理命令行参数
- 函数 main() 主程序入口
- 验证输入文件和参数有效性
- 输出帮助信息和错误提示
```

### src/json_loader.cpp
```text
- 函数 loadReplies()：读取 auto_replies.json
- 函数 loadReferences()：读取 human_ref.json
- 异常处理：缺失必填字段、文件不存在等
```

## 测试方案

如何验证这个任务的功能是否正确？必须包括测试命令和预期输出。

### 单元测试

#### 1. 编译测试
```bash
cd /home/chen/code/xiaoduo
rm -rf build && mkdir build && cd build
cmake .. && make

# 预期：编译成功，无错误，输出 reply_eval 可执行文件
```

#### 2. 帮助信息测试
```bash
./reply_eval --help

# 预期输出应包含：
# - Usage 说明
# - --mode 选项
# - --input 选项
# - --reference 选项
# - --output 选项
```

#### 3. JSON 加载测试
```bash
./reply_eval \
    --input ../doc/auto_replies.json \
    --reference ../doc/human_ref.json \
    --mode mock

# 预期：
# - 成功加载 20 条回复
# - 成功加载 20 条参考答案
# - 无 JSON 解析错误
# - 输出加载统计
```

### 集成测试

#### 1. 完整流程测试
```bash
./reply_eval \
    --input ../doc/auto_replies.json \
    --reference ../doc/human_ref.json \
    --mode mock \
    --output ../reports/report.md

# 预期：
# - 程序正常退出（exit code 0）
# - 生成 ../reports/report.md 文件
# - 报告包含"总体得分"、"通过率"等基本统计
```

### 验证要点

- [ ] 无编译警告（-Wall -Wextra）
- [ ] 加载的 JSON 数据与原文件数据一致
- [ ] 命令行参数解析正确
- [ ] 错误消息清晰有用

## 代码质量检查清单

完成后确保以下项都满足：

- [ ] 代码符合编码规范（DEVELOPMENT.md § 8）
- [ ] 所有函数都有文档注释（Doxygen 格式）
- [ ] 没有编译错误和警告
- [ ] 所有异常情况都有处理（文件不存在、格式错误等）
- [ ] 输出消息对用户友好、明确
- [ ] 代码已通过本地构建和测试

## 其他注意事项

- 在提交前确保代码通过所有测试
- 如有第三方库依赖问题，请在 issue 中详细描述
- Mock Judge 的第一版本不需要完全精确，但应该能演示评估流程
- 保持代码简洁，复杂逻辑延后到 Task_002

---

## 审核检查清单

实现者完成后可用此检查；审核者用此验证任务完成质量。

- [ ] 所有实现步骤都已完成
- [ ] 所有新增文件都已创建
- [ ] 编译成功，无错误和警告
- [ ] 所有测试用例都通过
- [ ] 代码注释完整、清晰
- [ ] 符合编码规范

---

**备注**：删除本说明，按照上述格式填充实际内容。
