# PLAN_002: JSON 数据加载与 MockJudge 规则评分实现

**计划编号**: PLAN_002  
**优先级**: HIGH  
**状态**: In Progress  
**创建日期**: 2026-08-18  
**预计交付**: 2026-08-19  

---

## 计划目标

实现项目的核心评估逻辑：完成 JSON 数据加载模块和 MockJudge 规则型评分器，使程序能够对 20 条自动回复进行完整评估。

## 任务预期结果

完成后将具备以下能力：

- ✅ 能够从 `doc/auto_replies.json` 和 `doc/human_ref.json` 正确加载数据
- ✅ MockJudge 能够对每条回复在四个维度上评分（0-5）
- ✅ 评分规则基于对用户问题、自动回复、人工参考答案的启发式分析
- ✅ 能够计算加权最终分数并判定 Critical Failure
- ✅ 生成初步的 Markdown 评估报告
- ✅ 支持 `./reply_eval --mode mock --input ... --reference ... --output ...` 的完整运行流程

## 任务创建原因

项目框架已搭建，但缺少核心的数据处理和评分逻辑。这一阶段：

- 完成离线 Mock 评估的能力，使程序能独立运行
- 为后续 LLM 评估提供参考和对照
- 通过 MockJudge 验证整个管道（数据加载 → 评估 → 报告）的可行性
- 支持不依赖 API 的完整演示和测试

## 涉及文件

| 操作 | 文件路径 | 说明 |
|---|---|---|
| 修改 | src/json_loader.cpp | 实现 nlohmann/json 数据加载逻辑 |
| 修改 | src/mock_judge.cpp | 实现四维评分规则 |
| 修改 | src/main.cpp | 集成主流程：加载 → 评估 → 统计 → 生成报告 |
| 修改 | src/report.cpp | 完善 Markdown 报告生成逻辑 |
| 新增 | include/mock_judge_rules.h | 规则型评分的具体判断逻辑（可选） |
| 修改 | CMakeLists.txt | 如需确保 nlohmann/json 可用 |

## 风险评估

可能遇到的关键问题：

- nlohmann/json 库的安装和集成方式（系统包 vs 单头文件）
- JSON 数据格式和字段名的不匹配
- 规则型评分的准确性难以保证，需要多次迭代
- 启发式规则可能对某些边界 case 判断不准
- 报告格式需要符合业务期望

## 其他说明

本计划基于实际数据格式：

- `auto_replies.json`: 包含 `id`, `user_question`, `auto_reply`
- `human_ref.json`: 包含 `id`, `human_reference`, `annotator_notes`

MockJudge 的规则评分应考虑：

- Accuracy：用户问题的理解 + 回复的事实正确性 + 与人工参考答案的冲突度
- Helpfulness：是否提供了实际解决方案 + 是否需要用户自己进一步操作
- Tone：礼貌程度 + 同情心表达 + 语言自然度
- Factual Reliability：是否引入了参考资料不支持的信息

---

## 审核检查清单

- [ ] 数据加载逻辑正确，能处理 20 条 Case
- [ ] MockJudge 四维评分均有实现
- [ ] 加权计算正确
- [ ] Critical Failure 判定逻辑完整
- [ ] 主流程能完整运行并生成报告
- [ ] 报告格式清晰、数据准确
- [ ] 支持 Mock 模式的命令行运行

---

**备注**：本计划为离线评估的完整实现，为后续 LLM 评估和项目交付提供基础。