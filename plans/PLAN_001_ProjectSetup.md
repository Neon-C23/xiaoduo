# PLAN_001: 项目基础结构与 CLI 框架搭建

**计划编号**: PLAN_001  
**优先级**: HIGH  
**状态**: In Progress  
**创建日期**: 2026-08-17  
**预计交付**: 2026-08-18  

---

## 计划目标

搭建项目的基础工程结构，完成 CMake 构建、CLI 参数解析、JSON 输入加载、统一评估接口骨架，并为后续 MockJudge 和 LLMJudge 留出清晰扩展点。

## 任务预期结果

完成后将具备以下能力：

- ✅ 项目能够使用 CMake 成功编译生成 `reply_eval`
- ✅ CLI 支持 `--mode`, `--input`, `--reference`, `--output`, `--api-endpoint`, `--model` 等参数
- ✅ 可以正常加载 `doc/auto_replies.json` 和 `doc/human_ref.json`
- ✅ 数据结构与评估接口已定义，后续评估器可以直接接入
- ✅ 支持 DeepSeek OpenAI-compatible API 配置：`https://api.deepseek.com`，模型推荐 `deepseek-v4-flash`

## 任务创建原因

这是整个评估流水线的基础设施阶段。只有先完成统一构建、数据模型和评估接口，后续才能顺利实现：

- MockJudge 规则评估
- LLMJudge 调用 DeepSeek API
- 评估统计与报告生成
- 运行验证和结果输出

没有这一步，后续功能无法稳定落地。

## 涉及文件

| 操作 | 文件路径 | 说明 |
|---|---|---|
| 新增 | CMakeLists.txt | CMake 项目配置，设定 C++17 和依赖查找 |
| 新增 | include/config.h | 全局配置、权重常量、枚举与 LLM 参数定义 |
| 新增 | include/models.h | 通用数据结构：ReplyCase、ReferenceCase、EvaluationScore 等 |
| 新增 | include/judge.h | `IJudge` 抽象接口 |
| 新增 | include/evaluator.h | 评估流程管理与计算接口 |
| 新增 | src/main.cpp | CLI 入口和主流程控制 |
| 新增 | src/json_loader.cpp | JSON 读取与字段校验 |
| 新增 | src/evaluator.cpp | 评分计算、Critical Failure 判定 |
| 新增 | src/mock_judge.cpp | MockJudge 实现 |
| 新增 | src/llm_judge.cpp | LLMJudge 实现（DeepSeek API） |
| 新增 | src/report.cpp | Markdown 报告生成 |
| 新增 | src/report.h | 报告接口声明 |
| 修改 | README.md | 更新运行说明和配置说明 |
| 修改 | DEVELOPMENT.md | 补充 LLM 配置说明与规范 |

## 风险评估

可能遇到的关键问题：

- `nlohmann/json` 的安装方式和 CMake 集成方式
- `libcurl` 在本机环境中的可用性
- DeepSeek API 的 OpenAI-compatible 接口调用格式是否与项目假设一致
- LLM 评估过程中请求参数、模型名和 Base URL 的兼容性
- 后续 CLI 参数命名需保持稳定，避免后续改动带来破坏性变化

## 其他说明

当前项目的 LLM 测试配置参数已确定：

- `base_url (OpenAI)`: `https://api.deepseek.com`
- `base_url (Anthropic)`: `https://api.deepseek.com/anthropic`
- `api_key`: `sk-3d0dfcc199fe46a8b8547f5ff0ed16e0`
- `model`: `deepseek-v4-flash`（推荐）；备选 `deepseek-v4-pro`

本计划确保工程结构和接口设计先落地，从而为后续真实评估逻辑和报告输出奠定基础。

---

## 审核检查清单

- [ ] 计划目标明确，能够通过编译和运行验证
- [ ] 预期结果可被测试覆盖
- [ ] 涉及文件清单完整
- [ ] 风险项已识别并准备了应对方案
- [ ] 计划与后续评估器实现的依赖关系清晰

---

**备注**：本计划为开发起点，后续将基于此拆解具体任务卡。