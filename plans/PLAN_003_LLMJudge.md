# PLAN_003: LLM Judge 实现

**计划编号**: PLAN_003
**优先级**: HIGH
**状态**: Completed

## 计划目标

实现基于 DeepSeek API 的 LLM 评估器，提供比 MockJudge 更准确、更可靠的自动回复质量评估。

## 任务预期结果

完成后用户将看到：
- ✅ 项目支持 `--mode llm` 参数，可调用 DeepSeek API 进行评估
- ✅ LLMJudge 能正确构造符合评估标准的 Prompt
- ✅ 成功调用 DeepSeek API 并解析响应
- ✅ 生成与人工标注相近的评分结果
- ✅ 正确处理 API 错误和超时情况

## 任务创建原因

- MockJudge 基于规则，无法理解复杂的语义和上下文
- LLM-as-a-Judge 是业界主流的自动评估方法
- DeepSeek 提供 OpenAI-compatible API，易于集成
- 可与人工标注对比验证评估方法的可靠性

## 涉及文件

| 操作 | 文件路径 | 说明 |
|---|---|---|
| 修改 | include/llm_judge.h | 添加 CURL 管理和新的方法声明 |
| 修改 | src/llm_judge.cpp | 实现完整的 LLM API 调用逻辑 |
| 修改 | CMakeLists.txt | 确保正确链接 libcurl |
| 新增 | third_party/nlohmann/json.hpp | 下载 JSON 解析库 |

## 风险评估

- 需要安装 libcurl 开发库 ✅ 已解决
- API 调用可能超时或失败 - 已实现超时和错误处理
- LLM 返回的 JSON 格式可能不一致 - 已实现解析容错
- API Key 管理问题 - 使用环境变量，程序启动时验证

## 其他说明

- LLMJudge 使用 DeepSeek 的 OpenAI-compatible 端点
- 默认使用 `deepseek-v4-flash` 模型
- Prompt 设计遵循项目评估标准文档
- 支持解析 markdown 代码块中的 JSON 响应
