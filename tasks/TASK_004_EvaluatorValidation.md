# TASK_004: 评估器校验功能实现

**任务编号**: TASK_004
**关联计划**: [PLAN_004_EvaluatorValidation](../plans/PLAN_004_EvaluatorValidation.md)
**状态**: Completed

## 实现思路

设计一个可扩展的校验框架，支持：
- 显式人工评分（explicit_scores 字段）
- 隐式评分推断（从 annotator_notes 分析）
- 多维度一致性分析
- 自动评估结论生成

## 实现步骤

1. **设计数据结构** (include/validation.h)
   - HumanScore 结构体：存储人工评分
   - DimensionValidation 结构体：单维度校验结果
   - ValidationResult 结构体：完整校验结果
   - EvaluatorValidator 类：校验逻辑

2. **实现校验器** (src/validation.cpp)
   - loadHumanScores() - 加载人工评分数据
   - validate() - 对比评估器与人工评分
   - inferScoreFromNotes() - 从备注推断隐式评分
   - generateReport() - 生成校验报告

3. **集成到报告生成** (include/report.h, src/report.cpp)
   - 添加包含校验结果的重载函数
   - 在说明章节前插入校验结果

4. **集成到主流程** (src/main.cpp)
   - 添加校验步骤
   - 根据是否有校验数据选择报告生成方式

5. **更新构建配置** (CMakeLists.txt)
   - 添加 validation.cpp 到源文件列表

## 第三方库

无新增依赖。

## 修改代码清单

### 新增文件

- include/validation.h - 校验功能头文件
- src/validation.cpp - 校验功能实现

### 修改文件

- include/report.h - 添加校验结果参数的重载函数
- src/report.cpp - 实现校验报告生成
- src/main.cpp - 集成校验流程
- CMakeLists.txt - 添加 validation.cpp

## 测试方案

1. **无人工评分数据测试**
   ```bash
   ./reply_eval --mode mock --input doc/auto_replies.json --reference doc/human_ref.json
   # 应该显示 "未找到人工评分数据，跳过校验"
   ```

2. **隐式评分推断测试**
   - 当前实现会从 annotator_notes 推断评分
   - 应该显示校验结果和一致性统计

3. **显式评分测试**（需要扩展数据）
   ```json
   {
     "id": "case_01",
     "explicit_scores": {
       "accuracy": 3,
       "helpfulness": 2,
       "factual_reliability": 4,
       "tone": 4
     }
   }
   ```

## 实现细节

### 校验指标

1. **Agreement Rate**: 评估器评分与人工评分完全一致的比例
2. **MAE (Mean Absolute Error)**: 平均绝对误差
3. **Critical Failure Recall**: 关键失败检测召回率

### 隐式评分推断

基于关键词分析：
- 负面关键词（不对、错误、问题）→ 降低评分
- 正面关键词（好、正确、可以）→ 提高评分
- 严重问题（严重、关键）→ 最低评分

### 评估结论逻辑

- 一致率 ≥ 80%：✅ 评估器可信
- 一致率 60-80%：⚠️ 建议调优
- 一致率 < 60%：❌ 需要改进

## 其他注意事项

- 校验功能是可选的，不影响基本评估流程
- 支持部分维度评分（不是所有维度都需要有人工评分）
- 从备注推断的评分比较简单，实际使用建议使用显式评分

## 当前状态

✅ 已完成基本实现，测试结果显示：
- 校验样本数：17
- Accuracy 一致率：70.6%，MAE：0.41
- 总体一致率：70.59%
- 评估结论：一致性中等，建议调优评估规则
