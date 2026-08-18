#pragma once

#include "judge.h"
#include <string>
#include <memory>

namespace replyx {

/**
 * @brief HTTP 响应结构体
 */
struct HttpResponse {
    long status_code = 0;
    std::string body;
    std::string error;
};

/**
 * @brief 基于 LLM API 的评估器
 * 调用 DeepSeek OpenAI-compatible API 进行评估
 */
class LLMJudge : public IJudge {
public:
    /**
     * @brief 构造函数
     * @param api_endpoint API 基地址（默认 DeepSeek）
     * @param model 模型名称（默认 deepseek-v4-flash）
     * @throws std::runtime_error 如果 LLM_API_KEY 环境变量未设置
     */
    LLMJudge(
        const std::string& api_endpoint,
        const std::string& model
    );

    virtual ~LLMJudge();

    /**
     * @brief 使用 LLM 对回复进行评估
     */
    EvaluationScore evaluate(
        const ReplyCase& reply,
        const ReferenceCase& reference
    ) override;

    /**
     * @brief 使用 LLM 对回复进行评估（无参考答案）
     */
    EvaluationScore evaluateStandalone(
        const ReplyCase& reply
    ) override;

    /**
     * @brief 获取评估器名称
     */
    std::string getName() const override;

    /**
     * @brief 是否支持无参考答案评估
     */
    bool supportsStandalone() const override { return true; }

private:
    std::string api_endpoint_;
    std::string model_;
    std::string api_key_;

    // CURL 句柄（使用 RAII 模式管理）
    void* curl_ = nullptr;

    /**
     * @brief 初始化 CURL
     */
    bool initCurl();

    /**
     * @brief 清理 CURL 资源
     */
    void cleanupCurl();

    /**
     * @brief 根据回复和参考答案构造 LLM Prompt
     */
    std::string buildPrompt(
        const ReplyCase& reply,
        const ReferenceCase& reference
    ) const;

    /**
     * @brief 构造无参考答案的 LLM Prompt
     */
    std::string buildPrompt(
        const ReplyCase& reply
    ) const;

    /**
     * @brief 调用 DeepSeek API
     */
    HttpResponse callDeepSeekAPI(const std::string& prompt);

    /**
     * @brief 解析 LLM 响应 JSON
     */
    EvaluationScore parseLLMResponse(const std::string& json_body);

    /**
     * @brief CURL 写入回调函数
     */
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

}  // namespace replyx
