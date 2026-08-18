# PLAN_004: 评估器校验功能

**计划编号**: PLAN_004
**优先级**: MEDIUM
**状态**: Completed

## 计划目标

实现评估器校验功能，通过与人工标注数据对比，计算评估器的准确性和可靠性指标。

## 任务预期结果

完成后用户将看到：
- ✅ 报告中新增"评估器校验"章节
- ✅ 显示 Agreement Rate（与人工标注的一致率）
- ✅ 显示 MAE（Mean Absolute Error，平均绝对误差）
- ✅ 显示各维度的一致性统计
- ✅ 显示 Critical Failure 检测率

## 任务创建原因

- 需要验证 MockJudge 和 LLMJudge 的评估准确性
- 需要量化评估器与人工标注的偏差
- 为后续规则调优提供数据支持
- 增强报告的可信度和价值

## 涉及文件

| 操作 | 文件路径 | 说明 |
|---|---|---|
| 新增 | include/validation.h | 校验功能头文件 |
| 新增 | src/validation.cpp | 校验功能实现 |
| 修改 | include/models.h | 添加人工评分结构（如需要） |
| 修改 | src/report.cpp | 在报告中添加校验结果章节 |
| 修改 | src/evaluator.cpp | 集成校验流程 |

## 风险评估

- 人工标注数据需要标准化格式
- 可能需要扩展 human_ref.json 的字段结构
- 计算逻辑需要与评估指标对齐

## 其他说明

校验指标说明：

1. **Agreement Rate**: 评估器评分与人工评分完全一致的比例
2. **MAE (Mean Absolute Error)**: 评估器与人工评分的平均绝对误差
3. **Correlation**: 评估器与人工评分的相关性（可选）
4. **Critical Failure Recall**: 人工标注的问题中有多少被检测出

校验数据来源：human_ref.json 中的人工评分字段（需要确认字段结构）
