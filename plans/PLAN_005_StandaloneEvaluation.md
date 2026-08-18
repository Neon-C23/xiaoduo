# PLAN_005: 无参考答案评估模式

**计划编号**: PLAN_005
**优先级**: HIGH
**状态**: Completed

## 计划目标

实现无需人工参考答案的评估模式，使评估器能够仅基于用户问题和自动回复进行质量评估。

## 任务预期结果

完成后用户将看到：
- ✅ 支持无 reference 参数的评估
- ✅ MockJudge 可在无参考答案时给出合理评分
- ✅ LLMJudge 可在无参考答案时给出合理评分
- ✅ 报告中标注评估模式（有参考/无参考）

## 任务创建原因

- 生产环境中可能没有人工参考答案
- 自动评估应该能够独立工作
- 需要支持大规模实时评估场景
- 降低评估流程对人工标注的依赖

## 涉及文件

| 操作 | 文件路径 | 说明 |
|---|---|---|---|
| 修改 | include/judge.h | 添加可选参考答案的评估接口 |
| 修改 | src/mock_judge.cpp | 支持无参考答案的评分逻辑 |
| 修改 | src/llm_judge.cpp | 支持无参考答案的 Prompt |
| 修改 | src/evaluator.cpp | 支持无参考答案的评估流程 |
| 修改 | src/main.cpp | 支持 --skip-reference 参数 |
| 修改 | src/json_loader.cpp | ReferenceCase 可为空 |

## 风险评估

- 无参考答案时评分准确性会降低
- 需要明确区分有参考和无参考两种模式的可靠性
- Factual Reliability 指标在无参考答案时更难评估

## 设计方案

### 方案一：修改现有接口（推荐）

在 IJudge 接口中添加重载方法：

```cpp
class IJudge {
public:
    // 原有方法（有参考答案）
    virtual EvaluationScore evaluate(
        const ReplyCase& reply,
        const ReferenceCase& reference
    ) = 0;

    // 新方法（无参考答案）
    virtual EvaluationScore evaluate(
        const ReplyCase& reply
    ) {
        // 默认实现：抛出异常或返回默认评分
        throw std::runtime_error("Standalone evaluation not supported");
    }
};
```

### 方案二：使用空 ReferenceCase

让 ReferenceCase 支持空值，在评估时检查：

```cpp
EvaluationScore evaluate(
    const ReplyCase& reply,
    const ReferenceCase& reference  // 可为空
);
```

## 评分策略调整

### 无参考答案时的评分逻辑

**Accuracy**:
- 检查回复是否回答了用户问题（问题-回复相关性）
- 检查回复内容是否完整
- 检查是否有明显的事实错误

**Helpfulness**:
- 与有参考答案时相同（主要基于回复本身）
- 检查是否提供解决方案
- 检查是否包含具体步骤

**Factual Reliability**:
- 检查是否有明显的编造信息
- 降低评分标准（更保守）
- 在报告中标注"无参考验证"

**Tone**:
- 与有参考答案时相同

## 其他说明

- 无参考答案模式的评估结果会标注"⚠️ 无参考答案验证"
- 报告中会区分有参考和无参考两种模式的统计
- 用户应明确知道无参考答案评估的局限性
