# 自动回复质量评估流水线 · 开发规范文档

**版本**: 1.0  
**日期**: 2026-08-17  
**作者**: Development Team

---

## 目录

1. [项目概述](#1-项目概述)
2. [项目结构](#2-项目结构)
3. [指标定义](#3-指标定义)
4. [架构设计](#4-架构设计)
5. [工作流规范](#5-工作流规范)
6. [Plan Card 规范](#6-plan-card-规范)
7. [Task Card 规范](#7-task-card-规范)
8. [编码规范](#8-编码规范)
9. [构建与测试](#9-构建与测试)
10. [常见问题](#10-常见问题)

---

## 1. 项目概述

### 1.1 需求背景

团队上线了"客服自动回复"功能，需要评估回复质量。业务方需求明确但描述模糊：

- 回复要**准确**
- 回复要**有用**
- **语气好**
- **不能瞎编**

### 1.2 项目目标

构建一个可重复运行的命令行自动回复质量评估流水线，对任意输入数据集中的自动回复逐条进行多维度评分，并生成整体质量报告、指标分布和低质量 Case 分析。当前提供的 20 条数据仅作为验证样本，不代表系统设计永远固定为 20 条记录。

**关键交付物**：

- ✅ 四维评估指标（准确、有用、语气、事实可靠性）
- ✅ 评估流水线（支持 Mock 模式和真实 LLM 模式）
- ✅ 评估报告（Markdown 格式）
- ✅ 局限性讨论和改进建议
- ✅ 开发过程截图和运行结果演示

### 1.3 评估设计原则

本项目不假设存在完整的业务 Knowledge Base，也不要求所有评估都依赖真实产品数据库。当前题目更适合采用 Reference-based Evaluation：

- 以用户问题、自动回复、人工参考答案和人工分析为主要评估依据；
- 关注“是否理解了用户意图、是否回答正确、是否帮助用户、是否引入了无法支持的事实”；
- 采样数据集只是当前验证样本，系统设计应支持 20、100、1000 甚至更多样本量，而无需修改核心评估逻辑；
- 若未来需要更强的事实核验能力，应接入真实业务知识源或官方政策资料，但这不属于当前离线评估的前提条件。

### 1.4 技术栈

| 组件 | 选型 | 理由 |
|---|---|---|
| 语言 | C++17 | 工程化、性能、跨平台 |
| 构建 | CMake | 标准化、便于交付 |
| JSON | nlohmann/json | 单头文件、易集成 |
| HTTP | libcurl | 轻量、稳定 |
| 输出 | Markdown + JSON | 易读、易处理 |

---

## 2. 项目结构

```text
xiaoduo/
├── DEVELOPMENT.md              # 本开发规范文档
├── README.md                   # 项目说明（最终交付）
├── CMakeLists.txt             # 构建配置
│
├── include/                    # 头文件
│   ├── models.h               # 数据结构定义
│   ├── judge.h                # 评估器接口（抽象）
│   ├── evaluator.h            # 评估流程
│   ├── report.h               # 报告生成
│   ├── config.h               # 配置管理
│   └── json_utils.h           # JSON 工具函数
│
├── src/                        # 源文件
│   ├── main.cpp               # 命令行入口
│   ├── json_loader.cpp        # JSON 读取
│   ├── mock_judge.cpp         # Mock 评估器（规则型）
│   ├── llm_judge.cpp          # LLM 评估器（API 调用）
│   ├── evaluator.cpp          # 核心评估逻辑
│   └── report.cpp             # 报告生成
│
├── third_party/               # 第三方库
│   └── nlohmann/              # JSON 库（可选，也可用系统包）
│
├── doc/                        # 原始数据和参考文档
│   ├── auto_replies.json      # 20 条自动回复（输入）
│   ├── human_ref.json         # 人工参考答案和标注（输入）
│   ├── eval_criteria.md       # 业务评估标准
│   └── 0109_自动回复质量评估流水线_C++实现方案.md  # 策略文档
│
├── data/                       # 运行时数据（如有）
│   ├── auto_replies.json      # 符号链接到 doc/
│   └── human_ref.json         # 符号链接到 doc/
│
├── plans/                      # 计划卡片（Plan Cards）
│   ├── PLAN_00_TEMPLATE.md    # 模板
│   └── PLAN_*.md              # 各个计划卡片
│
├── tasks/                      # 任务卡片（Task Cards）
│   ├── TASK_00_TEMPLATE.md    # 模板
│   └── TASK_*.md              # 各个任务卡片
│
├── build/                      # 编译输出目录（生成）
│   └── reply_eval             # 最终二进制
│
├── reports/                    # 评估报告（生成）
│   └── report.md              # 输出报告
│
└── screenshots/               # 开发过程截图（可选）
    ├── 01-dev-ide.png
    ├── 02-build.png
    ├── 03-run-mock.png
    └── 04-report.png
```

---

## 3. 指标定义

### 3.1 核心评估指标

四个指标的综合加权模式，每项分值 0~5 分。

| # | 指标 | 权重 | 说明 |
|---|---|---|---|
| 1 | **Accuracy**<br>准确性 | 35% | 是否正确理解用户意图并给出事实正确的回答 |
| 2 | **Helpfulness**<br>有用性 | 30% | 是否真正帮助用户解决问题（给步骤、给方案） |
| 3 | **Factual Reliability**<br>事实可靠性 | 25% | 是否编造政策、流程、时间、价格、承诺等，或引入无法被参考信息支持的事实 |
| 4 | **Tone**<br>语气 | 10% | 是否礼貌、自然、专业、适度安抚 |

**最终得分公式**：

$$\text{Final Score} = \text{Accuracy} \times 0.35 + \text{Helpfulness} \times 0.30 + \text{Factual Reliability} \times 0.25 + \text{Tone} \times 0.10$$

> 说明：`Factual Reliability` 是一个“正向指标”，分数越高表示事实越可靠，越低表示越可能存在编造或未经支持的信息。

### 3.2 各指标评分标准

#### Accuracy（准确性） | 权重 35%

| 分值 | 定义 |
|---|---|
| **5** | 完全正确，完整回答了问题，无事实错误 |
| **4** | 基本正确，仅存在轻微遗漏或表述不完全 |
| **3** | 部分正确，存在明显遗漏但核心不错 |
| **2** | 大部分内容不准确或存在关键错误 |
| **1** | 基本答非所问，方向错误 |
| **0** | 完全错误或与事实相反 |

**Judge 判断重点**：

- 是否理解了用户的核心意图？
- 回复中的关键事实是否正确？
- 是否与人工参考答案严重冲突？

---

#### Helpfulness（有用性） | 权重 30%

| 分值 | 定义 |
|---|---|
| **5** | 可以直接帮助用户完成操作或解决问题 |
| **4** | 很有帮助，但缺少部分信息 |
| **3** | 提供了一定帮助，但用户可能需要进一步询问 |
| **2** | 帮助有限，用户需要自己做很多工作 |
| **1** | 几乎没有帮助 |
| **0** | 完全无用 |

**Judge 判断重点**：

- 有没有给出实际解决方案？（不只是概念说明）
- 有没有给出具体步骤？（而不是"自己去看"）
- 用户是否可能还需要继续追问？

**注意**：Accuracy 和 Helpfulness 必须分开。例如：

```
回复："可以退款。"

分值：Accuracy = 5（这句话确实正确）
     Helpfulness = 1（但对用户帮助很小，没说怎么退）
```

---

#### Factual Reliability（事实可靠性） | 权重 25%

| 分值 | 定义 |
|---|---|
| **5** | 没有任何未经支持的信息，所有陈述都有根据 |
| **4** | 基本可靠，可能有极轻微的推断 |
| **3** | 存在少量可疑信息，但风险较低 |
| **2** | 存在明显未经支持的信息 |
| **1** | 大量未经支持的信息 |
| **0** | 严重事实编造 |

**Judge 判断重点**：

- 是否编造了政策或流程？
- 是否编造了具体的时间、金额、时效？
- 是否夸大了产品功能或承诺？
- 人工参考资料是否不存在该信息？
- 该说法是否与已有问题、参考答案或人工分析相冲突？

**例子**：

```
人工参考："退款需要联系客服处理。"

自动回复："退款将在3个工作日内到账。"

判定：Factual Reliability 较低分，因为"3个工作日"未在参考资料中，也无法从已知上下文得到支持。
```

---

#### Tone（语气） | 权重 10%

| 分值 | 定义 |
|---|---|
| **5** | 礼貌、自然、专业、友好，充分尊重用户 |
| **4** | 基本自然，略显正式但可接受 |
| **3** | 略显机械，不够自然但无不当 |
| **2** | 明显生硬、冷漠或机械感强 |
| **1** | 不礼貌、消极、敷衍 |
| **0** | 冒犯用户、不尊重 |

**Judge 判断重点**：

- 有没有称呼和问候？
- 语气是否体现了同情心或安抚？（尤其是处理投诉场景）
- 表述是否自然流畅？

---

### 3.3 关键失败规则

即使综合得分看起来还可以，以下情况仍属于 **Critical Failure**：

```
IF (Accuracy <= 1) OR (Factual Reliability <= 1) THEN
    Critical Failure ← True
END IF
```

**业务含义**：

- 这条回复严重错误或编造事实
- 不能直接上线，需要人工审核
- 应该被单独标记和报告

### 3.4 结果状态：PASS / WARNING / FAIL

为更直观地展示评估结论，可为每条回复或整体数据集输出状态：

```text
PASS:
    Final Score >= 4.0
    且 Accuracy >= 3
    且 Factual Reliability >= 4

WARNING:
    3.0 <= Final Score < 4.0
    且不存在 Critical Failure

FAIL:
    Final Score < 3.0
    或存在 Critical Failure
```

> 说明：这些阈值是本评估方案自行定义的示例，不等同于业务方正式上线标准。

---

## 4. 架构设计

### 4.1 设计原则：任意数据集、参考式评估

本项目的评估器应当以“数据集驱动”的方式工作，而非与当前 20 条样本强绑定：

```text
输入数据集 = 任意数量的 Reply Case
        ↓
统一评估流程
        ↓
聚合统计
        ↓
报告输出
```

核心原则：

- 不写死 Case 数量；
- 不写死 Case ID；
- 不针对当前样本增加特殊逻辑；
- 评估逻辑基于统一规则，而非硬编码的 20 条数据；
- 评估依据是 `Question + Auto Reply + Human Reference + Human Analysis`，而不是假设存在完整知识库。

### 4.2 整体流程

```
┌─────────────────────────┐
│   auto_replies.json     │
│   任意数量的自动回复    │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│   C++ Evaluator         │
│                         │
│  1. Load JSON           │
│  2. For each reply:     │
│     - Judge evaluate    │
│     - Calculate score   │
│     - Check failure     │
│  3. Aggregate stats     │
│  4. Generate report     │
└────────────┬────────────┘
             │
    ┌────────┴────────┐
    │                 │
    ▼                 ▼
┌──────────────┐  ┌──────────────┐
│ Mock Judge   │  │ LLM Judge    │
│ (Offline)    │  │ (API Call)   │
└──────┬───────┘  └──────┬───────┘
       │                 │
       └────────┬────────┘
                ▼
       ┌─────────────────┐
       │ EvaluationScore │
       │ (4 dimensions)  │
       └────────┬────────┘
                ▼
       ┌─────────────────────┐
       │ Report Generator    │
       │ - Summary stats     │
       │ - Worst 3 cases     │
       │ - Full details      │
       └────────┬────────────┘
                ▼
       ┌──────────────────────┐
       │ reports/report.md    │
       │ (Final Output)       │
       └──────────────────────┘
```

### 4.3 核心抽象：Judge 接口

**目的**：支持多种评估方式（Mock / LLM），主程序无需改动。

```cpp
// include/judge.h
class IJudge {
public:
    virtual ~IJudge() = default;
    
    virtual EvaluationScore evaluate(
        const ReplyCase& reply,
        const ReferenceCase& reference
    ) = 0;
};
```

**实现**：

| 类 | 说明 |
|---|---|
| `MockJudge` | 基于规则的离线评估器 |
| `LLMJudge` | 调用 OpenAI-compatible API 的评估器 |

### 4.4 核心数据结构

```cpp
// include/models.h

struct EvaluationScore {
    int accuracy;
    int helpfulness;
    int tone;
    int factual_reliability;
    
    std::string reason;
    std::vector<std::string> unsupported_claims;
};

struct ReplyCase {
    std::string id;
    std::string user_question;
    std::string auto_reply;
};

struct ReferenceCase {
    std::string id;
    std::string human_reference;
    std::string annotator_notes;
};

struct EvaluationResult {
    std::string id;
    EvaluationScore score;
    double final_score;          // 加权后的最终分数
    bool critical_failure;       // 是否是关键失败
    
    // 额外信息
    std::string judge_reason;    // Judge 的详细理由
};
```

### 4.5 评分计算逻辑

```cpp
// src/evaluator.cpp

double calculateFinalScore(const EvaluationScore& score) {
    return
        score.accuracy * 0.35 +
        score.helpfulness * 0.30 +
        score.factual_reliability * 0.25 +
        score.tone * 0.10;
}

bool isCriticalFailure(const EvaluationScore& score) {
    return score.accuracy <= 1 || score.factual_reliability <= 1;
}
```

### 4.6 评估器验证（Evaluator Validation）

仅靠某个 Judge 输出分数并不代表方法本身合理，需要用人工标注来验证评估器的可靠性：

- **Agreement Rate**：判断人工标注和模型判断是否一致；
- **MAE（Mean Absolute Error）**：计算模型分数与人工分数之间的平均绝对误差；
- **Critical Failure Detection**：检查严重错误是否能被自动评估识别出来。

推荐在报告中新增一节：

```text
Evaluator Validation
- Agreement Rate
- MAE by metric
- Critical Failure recall
```

这类检测有助于理解评估方法的偏差，并为后续调优提供依据。

---

## 5. 工作流规范

### 5.1 三段工作流

任何代码改动或功能开发都遵循以下流程：

```
1. Plan Card（计划阶段）
   ↓ [开发者/审核者确认]
2. Task Card（任务分解）
   ↓ [开发者/审核者确认]
3. Execute（执行开发）
   ↓ [完成、测试、提交]
4. Verification（验证完成）
```

### 5.2 何时创建 Plan Card

每当你需要进行以下操作时：

- ✅ 新增功能模块（如新的 Judge 实现）
- ✅ 修改现有代码结构
- ✅ 修复重要 bug
- ✅ 添加第三方依赖
- ✅ 修改数据结构或接口

**不需要创建 Plan Card 的情况**：

- ❌ 修复小的拼写错误
- ❌ 添加注释
- ❌ 一行代码的小改动

### 5.3 工作流示例

**场景**：实现 `MockJudge` 类

```
Step 1: 创建 plans/PLAN_001_MockJudge.md
        └─ 说明：要实现什么、为什么、预期结果

Step 2: 等待确认（用户审阅）
        
Step 3: 根据反馈创建 tasks/TASK_001_MockJudge.md
        └─ 说明：具体怎么实现、代码清单、测试方案

Step 4: 等待确认（用户审阅）

Step 5: 执行编码和测试

Step 6: 报告完成
```

---

## 6. Plan Card 规范

### 6.1 文件命名

```text
plans/PLAN_<序号>_<简短标题>.md

例如：
  plans/PLAN_001_ProjectSetup.md
  plans/PLAN_002_MockJudge.md
  plans/PLAN_003_LLMJudge.md
```

序号从 001 开始，逐次递增。

### 6.2 必填字段

| 字段 | 说明 | 示例 |
|---|---|---|
| **计划编号** | 唯一标识 | PLAN_001 |
| **计划名称** | 简洁的功能描述 | 项目基础结构搭建 |
| **任务目标** | 这个计划要达成什么 | 搭建 CMakeLists.txt、头文件骨架、CLI 入口 |
| **任务预期结果** | 完成后会有什么输出 | 可以成功构建，./reply_eval --help 可以运行 |
| **任务创建原因** | 为什么要做这件事 | 基础工程框架是后续所有功能的前提 |
| **涉及文件** | 哪些现有文件需要修改/创建 | 新增：CMakeLists.txt, include/config.h, src/main.cpp；修改：无 |
| **风险评估** | 可能的问题或依赖 | 需要确认 nlohmann/json 的集成方式 |
| **优先级** | 相对优先级 | HIGH / MEDIUM / LOW |

### 6.3 模板

见 [plans/PLAN_00_TEMPLATE.md](#plan-card-模板)

---

## 7. Task Card 规范

### 7.1 文件命名

```text
tasks/TASK_<序号>_<简短标题>.md

例如：
  tasks/TASK_001_ProjectSetup.md
  tasks/TASK_002_MockJudge.md
```

序号与对应的 Plan Card 编号一致。

### 7.2 必填字段

| 字段 | 说明 | 示例 |
|---|---|---|
| **任务编号** | 对应的 Plan Card 编号 | TASK_001 |
| **关联计划** | 链接到对应的 PLAN | [PLAN_001_ProjectSetup](../plans/PLAN_001_ProjectSetup.md) |
| **实现思路** | 大的实现方向 | 使用 CMake 作为构建系统；采用接口抽象设计 Judge |
| **实现步骤** | 具体分步实现细节，可以是编号列表 | 1. 编写 CMakeLists.txt；2. 创建头文件框架；3. 实现 main.cpp CLI 参数解析；... |
| **第三方库** | 需要的外部依赖 | nlohmann/json, libcurl (仅 LLMJudge 模式需要) |
| **修改代码清单** | 要修改/新增的源文件 | 新增：CMakeLists.txt, include/config.h, include/models.h, src/main.cpp；修改：无 |
| **创建文件清单** | 要创建的新文件 | include/config.h, include/models.h, include/judge.h, src/main.cpp, src/json_loader.cpp |
| **测试方案** | 如何验证功能正确 | 构建成功；运行 `./reply_eval --help` 正常输出；加载 auto_replies.json 无错误 |

### 7.3 模板

见 [tasks/TASK_00_TEMPLATE.md](#task-card-模板)

---

## 8. 编码规范

### 8.1 C++ 标准和风格

- **标准**：C++17
- **命名**：
  - 类名：`PascalCase`（如 `MockJudge`）
  - 函数名：`camelCase`（如 `evaluateReply`）
  - 变量名：`snake_case`（如 `auto_reply`）
  - 常量：`UPPER_SNAKE_CASE`（如 `MAX_SCORE`）

### 8.2 头文件规范

```cpp
#pragma once

#include <string>
#include <vector>

/**
 * @brief 简洁的类或函数说明
 */
class MyClass {
public:
    /**
     * @param param1 参数说明
     * @return 返回值说明
     */
    void myMethod(const std::string& param1);

private:
    std::string data_;
};
```

### 8.3 包含顺序

```cpp
// 系统头文件
#include <iostream>
#include <string>
#include <vector>

// 第三方库
#include "nlohmann/json.hpp"

// 本项目头文件
#include "models.h"
#include "judge.h"
```

### 8.4 错误处理

- 优先使用异常而非返回码
- 对用户输入进行有效性检查
- 提供清晰的错误信息

```cpp
if (id.empty()) {
    throw std::invalid_argument("Reply ID cannot be empty");
}
```

### 8.5 代码注释

- 复杂逻辑必须有注释
- 公共接口必须有文档注释
- 避免冗余注释

---

## 9. 构建与测试

### 9.1 构建流程

```bash
# 1. 创建构建目录
cd /home/chen/code/xiaoduo
mkdir -p build && cd build

# 2. 配置（CMake）
cmake ..

# 3. 编译
make

# 4. 运行
./reply_eval --mode mock
```

### 9.2 Mock 模式（推荐开发用）

```bash
./reply_eval \
    --input ../doc/auto_replies.json \
    --reference ../doc/human_ref.json \
    --mode mock \
    --output ../reports/report.md
```

**优点**：

- ✅ 不需要 API Key
- ✅ 离线运行
- ✅ 快速反馈
- ✅ 便于 CI 集成

### 9.3 LLM 模式（生产用）

当前项目默认使用 DeepSeek 的 OpenAI-compatible API，推荐配置如下：

- `base_url (OpenAI)`: `https://api.deepseek.com`
- `base_url (Anthropic)`: `https://api.deepseek.com/anthropic`
- `api_key`: 从 DeepSeek 平台申请，并写入环境变量 `LLM_API_KEY`
- `model`: `deepseek-v4-flash`（推荐）；备选 `deepseek-v4-pro`

```bash
export LLM_API_KEY="your_deepseek_api_key"

./reply_eval \
    --input ../doc/auto_replies.json \
    --reference ../doc/human_ref.json \
    --mode llm \
    --api-endpoint https://api.deepseek.com \
    --model deepseek-v4-flash \
    --output ../reports/report.md
```

> 说明：如果未来需要按照 Anthropic 风格的兼容接口访问，可将 `base_url` 切换为 `https://api.deepseek.com/anthropic`；本项目默认采用 OpenAI-compatible 端点以保持兼容性和实现简洁性。

### 9.4 测试策略

| 阶段 | 内容 | 方式 |
|---|---|---|
| 单元测试 | 各个 Judge 的评分逻辑 | 可选：GoogleTest |
| 集成测试 | 完整流水线 | 手动运行 mock 模式 |
| 验证测试 | 对比人工标注 | 对比 human_ref.json 的分数一致性，计算 Agreement Rate 与 MAE |

### 9.5 MockJudge 的定位

`MockJudge` 适合作为开发和验证流水线的基础实现，用于：

- 验证输入解析是否正确；
- 验证评分流程是否能跑通；
- 验证报告生成逻辑是否正常；
- 做离线开发和 CI 确认。

但它不是可信的业务质量结论来源，不能被当作最后的生产级“事实判定器”。如果需要更可靠的自动评估，应在真实业务背景和数据支持下进一步扩展 LLM 评估逻辑，或接入业务知识源。

---

## 10. 常见问题

### Q1: 我什么时候需要创建 Plan Card？

**A**: 任何会改动代码结构、添加新功能、修改数据模型的工作都应该创建 Plan Card。如果只是修复一个小 bug 或改注释，就不必要。

### Q2: Plan Card 和 Task Card 有什么区别？

**A**:
- **Plan Card** 回答"做什么"和"为什么"：目标、预期结果、涉及文件
- **Task Card** 回答"怎么做"：具体步骤、代码清单、测试方案

### Q3: 为什么用 Mock Judge 而不是直接用 LLM？

**A**: Mock Judge 的优势：
- 不依赖 API Key，离线可用
- 构建和测试快
- 确定性输出，便于调试
- 适合演示和作业提交
- 生成规则的过程本身就是在理解评估标准

### Q4: 如何验证评估方法是否合理？

**A**: 用 `human_ref.json` 中的人工标注来验证：
- 比较 Judge 给出的分数与人工分数的偏差（MAE）
- 计算分数一致率（Agreement Rate）
- 如果偏差大，则调整 Judge 的规则或 LLM Prompt

### Q5: 可以直接修改 doc 文件夹里的 JSON 吗？

**A**: 不建议。`doc/` 下的文件是原始输入数据，应该保持不变。如果需要调整评估数据，可以在 `data/` 文件夹下创建符号链接或副本。

### Q6: 如何添加新的评估指标？

**A**: 这是一个重要的设计变更，应该：
1. 创建 Plan Card 说明为什么要新增指标
2. 更新本文档的"指标定义"部分
3. 修改 `EvaluationScore` 结构体
4. 更新 `calculateFinalScore()` 的加权公式
5. 修改所有 Judge 的实现
6. 创建 Task Card 并执行

---

## 附录 A: Plan Card 模板

```markdown
# PLAN_XXX: <计划名称>

**计划编号**: PLAN_XXX  
**优先级**: HIGH / MEDIUM / LOW  
**状态**: Not Started / In Progress / Approved / Completed  

## 计划目标

用简洁的一句话说明这个计划要做什么。

例如：实现基础的项目结构和 CMake 构建配置。

## 任务预期结果

完成后用户将看到什么。

例如：
- ✅ 项目成功构建
- ✅ `./reply_eval --help` 正常输出
- ✅ 可以加载 auto_replies.json 而不出错

## 任务创建原因

为什么需要做这件事？背景是什么？

例如：
- 基础工程框架是所有后续功能的前提
- 需要标准化的构建流程便于交付
- 需要 CLI 接口便于运行和演示

## 涉及文件

这个计划会涉及哪些文件（新增、修改、删除）。

| 操作 | 文件路径 | 说明 |
|---|---|---|
| 新增 | CMakeLists.txt | 项目构建配置 |
| 新增 | include/config.h | 配置管理头文件 |
| 新增 | src/main.cpp | CLI 入口 |
| 修改 | （无） | - |

## 风险评估

可能的问题或需要确认的事项。

例如：
- 需要确认 nlohmann/json 是否用单头文件还是系统包
- libcurl 在本机是否已安装

## 其他说明

任何额外的背景信息。
```

---

## 附录 B: Task Card 模板

```markdown
# TASK_XXX: <任务名称>

**任务编号**: TASK_XXX  
**关联计划**: [PLAN_XXX_XXX](../plans/PLAN_XXX_XXX.md)  
**状态**: Not Started / In Progress / Review / Completed  

## 实现思路

用几句话说明大的实现方向和设计决策。

例如：
- 使用 CMake 作为跨平台构建系统
- 采用接口抽象模式设计 `IJudge`，支持多种评估器实现
- Mock Judge 使用规则型启发式方法

## 实现步骤

具体分步实现，每一步要清楚。

1. 编写 CMakeLists.txt 顶级配置
2. 创建头文件框架（models.h, judge.h, etc.）
3. 实现 main.cpp 的 CLI 参数解析
4. 实现 JSON 加载器
5. 创建 IJudge 接口
6. 实现 MockJudge（第一版）
7. 本地构建和测试

## 第三方库

列出需要的外部依赖及版本要求。

| 库 | 版本 | 用途 | 可选 |
|---|---|---|---|
| nlohmann/json | 3.11+ | JSON 解析 | 否 |
| libcurl | 7.68+ | HTTP 请求（仅 LLMJudge） | 是 |

## 修改代码清单

要修改或创建的源文件。

### 新增文件

- include/config.h
- include/models.h
- include/judge.h
- include/evaluator.h
- src/main.cpp
- src/json_loader.cpp

### 修改文件

（暂无）

## 创建文件清单

新建文件的详细说明。

### include/models.h
- 定义 `EvaluationScore` 结构体
- 定义 `ReplyCase` 结构体
- 定义 `ReferenceCase` 结构体
- 定义 `EvaluationResult` 结构体

### include/judge.h
- 定义 `IJudge` 抽象基类
- 定义接口方法 `evaluate()`

### src/main.cpp
- 命令行参数解析（使用 getopt 或手动解析）
- 验证输入文件存在性
- 简单的使用说明

## 测试方案

如何验证这个任务的功能是否正确？

1. 构建验证
   ```bash
   cd build
   cmake ..
   make
   ```
   
2. 功能验证
   ```bash
   ./reply_eval --help
   # 应该输出帮助信息
   ```
   
3. 加载验证
   ```bash
   ./reply_eval --input ../doc/auto_replies.json
   # 应该成功加载 20 条回复，无错误
   ```

## 其他注意事项

- 确保编译时没有 warning（-Wall -Wextra）
- 代码需要符合编码规范（见 DEVELOPMENT.md § 8）
- 在 PR/提交前运行构建和测试

```

---

## 版本历史

| 版本 | 日期 | 改动 |
|---|---|---|
| 1.0 | 2026-08-17 | 初稿：完整的工作流规范、指标定义、架构设计、两个卡片模板 |

---

**最后更新**：2026-08-17
