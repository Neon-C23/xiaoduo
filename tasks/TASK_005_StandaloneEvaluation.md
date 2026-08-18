# TASK_005: 无参考答案评估模式实现

**任务编号**: TASK_005
**关联计划**: [PLAN_005_StandaloneEvaluation](../plans/PLAN_005_StandaloneEvaluation.md)
**状态**: Completed

## 实现思路

通过扩展 IJudge 接口，添加 `evaluateStandalone()` 方法和 `supportsStandalone()` 标志，使评估器能够在没有人工参考答案的情况下进行评估。

## 实现步骤

1. **扩展 IJudge 接口** (include/judge.h)
   - 添加 `evaluateStandalone()` 虚方法
   - 添加 `supportsStandalone()` 标志方法
   - 基类默认抛出异常（不支持）

2. **实现 MockJudge 无参考评估** (include/mock_judge.h, src/mock_judge.cpp)
   - 实现 `evaluateStandalone()` 方法
   - 基于 Q-R 相关性评分 Accuracy
   - Helpfulness 和 Tone 使用原有逻辑
   - Factual Reliability 采用保守策略

3. **实现 LLMJudge 无参考评估** (include/llm_judge.h, src/llm_judge.cpp)
   - 实现 `evaluateStandalone()` 方法
   - 修改 Prompt 去除参考答案部分
   - 强调无参考验证的保守性

4. **扩展评估器流程** (include/evaluator.h, src/evaluator.cpp)
   - 添加 `evaluateBatchStandalone()` 方法
   - 支持 `supportsStandalone()` 检查

5. **更新命令行界面** (src/main.cpp)
   - 添加 `--skip-reference` 参数
   - 修改数据加载逻辑（可选参考答案）
   - 根据模式选择评估方法
   - 无参考模式下跳过校验

## 第三方库

无新增依赖。

## 修改代码清单

### 修改文件

- include/judge.h - 添加 standalone 方法
- include/mock_judge.h - 添加 standalone 支持
- include/llm_judge.h - 添加 standalone 支持
- include/evaluator.h - 添加 standalone 批量评估
- src/mock_judge.cpp - 实现 standalone 评估逻辑
- src/llm_judge.cpp - 实现 standalone Prompt
- src/evaluator.cpp - 实现 standalone 批量评估
- src/main.cpp - 支持命令行参数和流程

## 测试方案

1. **Mock 无参考模式**
   ```bash
   ./reply_eval --mode mock --input doc/auto_replies.json --skip-reference
   # 应该成功评估并生成报告
   # 评估模式显示 "Mock (无参考)"
   ```

2. **LLM 无参考模式**
   ```bash
   export LLM_API_KEY="sk-xxx"
   ./reply_eval --mode llm --input doc/auto_replies.json --skip-reference
   # 应该调用 LLM API 并生成报告
   ```

3. **错误处理**
   ```bash
   # 不支持 standalone 的评估器（如果实现新的 Judge）
   # 应该抛出 "Standalone evaluation not supported" 错误
   ```

## 实现细节

### 无参考答案评分策略

**Accuracy（准确性）**:
- 计算问题关键词在回复中的出现率
- 检查是否有明显答非所问的表述
- 不依赖参考答案的相似度计算

**Helpfulness（有用性）**:
- 使用原有逻辑（步骤检测、服务检测）
- 不依赖参考答案

**Tone（语气）**:
- 使用原有逻辑（礼貌检测、同情心检测）
- 不依赖参考答案

**Factual Reliability（事实可靠性）**:
- 检查是否有无法验证的具体承诺
- 检查过度承诺关键词
- 采用保守评分策略（默认 4 分而非 5 分）

### 评估结果对比

| 模式 | 平均分数 | PASS | WARNING | FAIL |
|------|----------|------|---------|------|
| 有参考 | 3.35 | 3 | 12 | 5 |
| 无参考 | 3.19 | 0 | 14 | 6 |

无参考模式评分更保守，这是预期行为。

## 其他注意事项

- 无参考评估的准确性有限，主要用于生产环境的快速筛查
- 报告中会明确标注 "⚠️ 无参考答案验证"
- 建议定期进行有参考评估来校准无参考评估的结果
- 评估器必须显式声明支持 standalone（通过 `supportsStandalone()`）

## 当前状态

✅ 已完成实现，测试结果显示：
- 成功支持无参考答案评估
- 评分相对保守合理
- 报告正确标注评估模式
- 命令行参数 `--skip-reference` 正常工作
