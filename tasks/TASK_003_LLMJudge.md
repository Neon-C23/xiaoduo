# TASK_003: LLM Judge 实现

**任务编号**: TASK_003
**关联计划**: [PLAN_003_LLMJudge](../plans/PLAN_003_LLMJudge.md)
**状态**: Completed

## 实现思路

使用 libcurl 发送 HTTP 请求到 DeepSeek 的 OpenAI-compatible API：
- 构造符合评估标准的详细 Prompt
- 使用 chat/completions 端点发送请求
- 解析 LLM 返回的 JSON 格式评分
- 处理各种错误情况（网络、API、解析）

## 实现步骤

1. **更新头文件** (include/llm_judge.h)
   - 添加 `HttpResponse` 结构体
   - 添加 CURL 句柄管理
   - 添加新的私有方法声明

2. **实现 CURL 管理**
   - `initCurl()` - 初始化 CURL 全局环境和 easy handle
   - `cleanupCurl()` - 清理资源
   - 使用 RAII 模式管理 CURL 生命周期

3. **实现 Prompt 构造** (buildPrompt)
   - 包含用户问题、自动回复、人工参考答案、人工分析
   - 详细说明各维度评分标准
   - 要求 LLM 输出 JSON 格式结果

4. **实现 API 调用** (callDeepSeekAPI)
   - 构造 OpenAI-compatible 请求 JSON
   - 设置 HTTP headers（Authorization、Content-Type）
   - 发送 POST 请求到 /v1/chat/completions
   - 处理响应和错误

5. **实现响应解析** (parseLLMResponse)
   - 解析 API 响应 JSON
   - 提取 choices[0].message.content
   - 处理 markdown 代码块包裹的 JSON
   - 提取各维度评分和理由

6. **错误处理**
   - API Key 未设置时抛出异常
   - HTTP 非 200 状态码处理
   - JSON 解析错误处理
   - 超时设置（30秒）

## 第三方库

| 库 | 版本 | 用途 | 可选 |
|---|---|---|---|
| libcurl | 7.68+ | HTTP 客户端 | 否（LLM模式必须） |
| nlohmann/json | 3.11+ | JSON 解析 | 否 |

## 修改代码清单

### 修改文件

- include/llm_judge.h - 添加新的方法声明和 HttpResponse 结构体
- src/llm_judge.cpp - 完整实现所有功能

## 测试方案

1. **无 API Key 测试**
   ```bash
   ./reply_eval --mode llm --input doc/auto_replies.json --reference doc/human_ref.json
   # 应该输出 "未设置 LLM_API_KEY 环境变量" 错误
   ```

2. **Mock 模式验证**
   ```bash
   ./reply_eval --mode mock --input doc/auto_replies.json --reference doc/human_ref.json
   # 应该成功生成报告
   ```

3. **LLM 模式验证**（需要真实 API Key）
   ```bash
   export LLM_API_KEY="sk-xxx"
   ./reply_eval --mode llm --input doc/auto_replies.json --reference doc/human_ref.json
   # 应该成功调用 API 并生成报告
   ```

## 实现细节

### Prompt 设计要点

- 明确四个维度的评分标准（0-5分）
- 说明各维度权重
- 要求严格 JSON 输出
- 提供人工参考答案和备注作为评估依据

### API 请求参数

- `temperature`: 0.3（降低随机性）
- `max_tokens`: 1000（足够输出 JSON 结果）
- `model`: deepseek-v4-flash（快速模型）

### 错误处理策略

- 网络错误：返回 CURL 错误信息
- API 错误：解析响应中的 error 字段
- 解析错误：捕获 JSON 异常并提示

## 其他注意事项

- 确保 CURL 全局初始化只执行一次
- 正确清理 CURL 资源避免内存泄漏
- 支持从环境变量读取 API Key
- 支持自定义 API 端点和模型名称
