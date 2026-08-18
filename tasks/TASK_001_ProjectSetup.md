# TASK_001: 项目基础结构与 CLI 框架搭建

**任务编号**: TASK_001  
**关联计划**: [PLAN_001_ProjectSetup](../plans/PLAN_001_ProjectSetup.md)  
**优先级**: HIGH  
**状态**: In Progress  
**创建日期**: 2026-08-17  
**预计完成**: 2026-08-18  

---

## 实现思路

本任务的核心目标是先搭建一个可以编译和运行的工程骨架，再在此基础上接入评估逻辑。设计上采用：

- CMake 作为跨平台构建系统，统一编译入口；
- `IJudge` 抽象接口作为评估器扩展点，支持 Mock 和 LLM 两种实现；
- 统一的数据结构层管理输入、参考和评分结果，避免后续实现耦合；
- CLI 参数分层处理，确保评估流程从命令行启动时即可运行。

这样可以在不牺牲扩展性的前提下，尽快实现最小可运行版本。

## 实现步骤

1. **创建顶层 CMake 配置**
   - 设置 C++17
   - 配置 `include/` 头文件目录
   - 查找 `nlohmann/json` 和 `libcurl`
   - 定义可执行目标 `reply_eval`

2. **创建基础头文件**
   - `include/config.h`：定义权重、状态、枚举和 LLM 参数常量
   - `include/models.h`：定义 `ReplyCase`, `ReferenceCase`, `EvaluationScore`, `EvaluationResult`
   - `include/judge.h`：声明抽象基类 `IJudge`
   - `include/evaluator.h`：声明评估器上下文和评分函数

3. **实现 JSON 数据加载**
   - 加载 `doc/auto_replies.json`
   - 加载 `doc/human_ref.json`
   - 校验字段存在性和基本结构
   - 处理缺失字段和解析异常

4. **实现 CLI 入口**
   - `--mode`：支持 `mock` / `llm`
   - `--input`：输入 JSON 文件路径
   - `--reference`：参考文件路径
   - `--output`：输出报告路径
   - `--api-endpoint`：LLM API 基地址
   - `--model`：例如 `deepseek-v4-flash`
   - `--help`：输出使用方法

5. **实现评估计算核心**
   - 计算加权 score
   - 判定 `Critical Failure`
   - 维持 0~5 分的评分范围

6. **搭建 MockJudge 与 LLMJudge 接口骨架**
   - MockJudge：规则型评分体骨架
   - LLMJudge：准备 HTTP 调用逻辑与 DeepSeek 配置结构

7. **构建并验证最小运行流**
   - `cmake .. && make`
   - `./reply_eval --help`
   - `./reply_eval --mode mock ...`

## 第三方库

| 库名 | 最低版本 | 用途 | 是否可选 | 系统检查 |
|---|---|---|---|---|
| nlohmann/json | 3.11 | JSON 解析和序列化 | 否 | `find_package(nlohmann_json REQUIRED)` |
| libcurl | 7.68 | HTTP 请求（LLM 模式） | 是（仅 LLM 模式需要） | `find_package(CURL REQUIRED)` |

### DeepSeek LLM 配置

```bash
export LLM_API_KEY="sk-3d0dfcc199fe46a8b8547f5ff0ed16e0"
```

```text
base_url (OpenAI): https://api.deepseek.com
base_url (Anthropic): https://api.deepseek.com/anthropic
model: deepseek-v4-flash
```

## 修改代码清单

### 新增文件

#### CMakeLists.txt
- 配置 `cmake_minimum_required`, `project()`, `CMAKE_CXX_STANDARD 17`
- 包含 `include/` 目录
- 链接 `nlohmann_json` 与 `CURL`
- 生成目标 `reply_eval`

#### include/config.h
- 全局常量：权重、分数上限、状态字符串等
- 枚举 `JudgeMode { MOCK, LLM }`
- DeepSeek 配置常量（Base URL、默认模型）

#### include/models.h
- `ReplyCase`
- `ReferenceCase`
- `EvaluationScore`
- `EvaluationResult`

#### include/judge.h
- 抽象类 `IJudge`
- `virtual ~IJudge() = default;`
- `evaluate()` 纯虚函数

#### include/evaluator.h
- `calculateFinalScore()`
- `isCriticalFailure()`
- `Evaluator` 调度器框架

#### src/main.cpp
- 参数解析逻辑
- 主流程入口
- 错误处理和帮助输出

#### src/json_loader.cpp
- `loadReplies()`
- `loadReferences()`
- JSON 解析与校验

#### src/evaluator.cpp
- 加权公式实现
- Critical Failure 判定
- 评估循环控制

### 修改文件

- README.md：更新运行说明
- DEVELOPMENT.md：补充 LLM 配置说明

## 创建文件清单

### CMakeLists.txt
```text
- 项目工程入口
- C++17 编译标准
- 依赖管理
- 生成 reply_eval 可执行文件
```

### include/models.h
```text
- 数据结构定义
- 所有字段默认值
- 不包含业务逻辑
```

### include/judge.h
```text
- 决策器抽象接口
- 统一返回类型
- 支持 Mock / LLM 两种实现互换
```

### src/main.cpp
```text
- CLI 参数解析
- 启动评估流程
- 输出基础诊断信息
```

## 测试方案

### 1. 编译测试

```bash
cd /home/chen/code/xiaoduo
mkdir -p build && cd build
cmake ..
make
```

预期：构建成功，生成 `reply_eval` 二进制文件。

### 2. 帮助信息测试

```bash
./reply_eval --help
```

预期：输出参数说明，至少包括：

- `--mode`
- `--input`
- `--reference`
- `--output`
- `--api-endpoint`
- `--model`

### 3. Mock 模式最小验证

```bash
./reply_eval \
  --input ../doc/auto_replies.json \
  --reference ../doc/human_ref.json \
  --mode mock \
  --output ../reports/report.md
```

预期：程序能正常读取 JSON，并生成报告文件。

### 4. LLM 模式配置验证

```bash
export LLM_API_KEY="sk-3d0dfcc199fe46a8b8547f5ff0ed16e0"

./reply_eval \
  --input ../doc/auto_replies.json \
  --reference ../doc/human_ref.json \
  --mode llm \
  --api-endpoint https://api.deepseek.com \
  --model deepseek-v4-flash \
  --output ../reports/report.md
```

预期：程序能构造 DeepSeek 请求参数并发起 HTTP 调用，返回评估结果。

---

## 验收标准

任务完成标准：

- [ ] CMake 编译成功
- [ ] 可执行文件 `reply_eval` 正常运行
- [ ] CLI 参数和 JSON 文件路径可用
- [ ] Mock 模式可生成报告
- [ ] LLM 模式可配置 DeepSeek API 连接
- [ ] 结构满足后续评估器扩展要求

---

**备注**：本任务是开发起点，后续所有功能都应基于此工程骨架展开。