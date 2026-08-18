#include "llm_judge.h"
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <cstring>
#include <stdexcept>

// 引入 nlohmann/json（从 third_party 目录）
#include "nlohmann/json.hpp"

#include <curl/curl.h>

using namespace replyx;
using json = nlohmann::json;

/**
 * @brief LLMJudge 构造函数
 */
LLMJudge::LLMJudge(const std::string& api_endpoint, const std::string& model)
    : api_endpoint_(api_endpoint), model_(model) {

    // 从环境变量获取 API Key
    const char* api_key = std::getenv(LLMConfig::API_KEY_ENV_VAR);
    if (!api_key || std::strlen(api_key) == 0) {
        throw std::runtime_error(
            "LLM_API_KEY environment variable not set. "
            "Please set it with: export LLM_API_KEY=\"your_api_key\""
        );
    }
    api_key_ = api_key;

    // 初始化 CURL
    if (!initCurl()) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

/**
 * @brief 析构函数
 */
LLMJudge::~LLMJudge() {
    cleanupCurl();
}

/**
 * @brief 初始化 CURL
 */
bool LLMJudge::initCurl() {
    curl_global_init(CURL_GLOBAL_ALL);
    curl_ = curl_easy_init();
    return (curl_ != nullptr);
}

/**
 * @brief 清理 CURL 资源
 */
void LLMJudge::cleanupCurl() {
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
    curl_global_cleanup();
}

/**
 * @brief CURL 写入回调函数
 */
size_t LLMJudge::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

/**
 * @brief 评估实现
 */
EvaluationScore LLMJudge::evaluate(
    const ReplyCase& reply,
    const ReferenceCase& reference) {

    // 构造 Prompt
    std::string prompt = buildPrompt(reply, reference);

    // 调用 DeepSeek API
    HttpResponse http_response = callDeepSeekAPI(prompt);

    // 检查 HTTP 状态
    if (http_response.status_code != 200) {
        std::ostringstream err;
        err << "API request failed with status " << http_response.status_code;
        if (!http_response.body.empty()) {
            err << ": " << http_response.body;
        }
        throw std::runtime_error(err.str());
    }

    // 解析响应
    try {
        return parseLLMResponse(http_response.body);
    } catch (const std::exception& e) {
        std::ostringstream err;
        err << "Failed to parse LLM response: " << e.what()
            << "\nResponse body: " << http_response.body;
        throw std::runtime_error(err.str());
    }
}

/**
 * @brief 构造无参考答案的 LLM Prompt
 */
std::string LLMJudge::buildPrompt(
    const ReplyCase& reply) const {

    std::ostringstream prompt;

    prompt << "你是一名专业的客服自动回复质量评估专家。\n\n";
    prompt << "请根据用户问题和自动回复，对回复质量进行评分（注意：本次评估没有人工参考答案）。\n\n";
    prompt << "【用户问题】\n" << reply.user_question << "\n\n";
    prompt << "【自动回复】\n" << reply.auto_reply << "\n\n";

    prompt << "请分别对以下四个维度进行评分（每个维度 0-5 分）：\n\n";

    prompt << "1. Accuracy（准确性）- 权重 35%\n";
    prompt << "   5 = 完全正确，完整回答了问题，无事实错误\n";
    prompt << "   4 = 基本正确，仅存在轻微遗漏或表述不完全\n";
    prompt << "   3 = 部分正确，存在明显遗漏但核心不错\n";
    prompt << "   2 = 大部分内容不准确或存在关键错误\n";
    prompt << "   1 = 基本答非所问，方向错误\n";
    prompt << "   0 = 完全错误或与事实相反\n\n";

    prompt << "2. Helpfulness（有用性）- 权重 30%\n";
    prompt << "   5 = 可以直接帮助用户完成操作或解决问题\n";
    prompt << "   4 = 很有帮助，但缺少部分信息\n";
    prompt << "   3 = 提供了一定帮助，但用户可能需要进一步询问\n";
    prompt << "   2 = 帮助有限，用户需要自己做很多工作\n";
    prompt << "   1 = 几乎没有帮助\n";
    prompt << "   0 = 完全无用\n\n";

    prompt << "3. Factual Reliability（事实可靠性）- 权重 25%\n";
    prompt << "   5 = 没有任何未经支持的信息，所有陈述都有根据\n";
    prompt << "   4 = 基本可靠，可能有极轻微的推断\n";
    prompt << "   3 = 存在少量可疑信息，但风险较低\n";
    prompt << "   2 = 存在明显未经支持的信息\n";
    prompt << "   1 = 大量未经支持的信息\n";
    prompt << "   0 = 严重事实编造\n";
    prompt << "   注意：由于没有参考答案，对于无法验证的具体承诺（如时间、金额）应降低评分\n\n";

    prompt << "4. Tone（语气）- 权重 10%\n";
    prompt << "   5 = 礼貌、自然、专业、友好，充分尊重用户\n";
    prompt << "   4 = 基本自然，略显正式但可接受\n";
    prompt << "   3 = 略显机械，不够自然但无不当\n";
    prompt << "   2 = 明显生硬、冷漠或机械感强\n";
    prompt << "   1 = 不礼貌、消极、敷衍\n";
    prompt << "   0 = 冒犯用户、不尊重\n\n";

    prompt << "评分要求：\n";
    prompt << "- 重点关注回复是否真正回答了用户的问题\n";
    prompt << "- 检查回复中是否存在编造或无法支持的具体信息\n";
    prompt << "- 对于没有参考答案验证的情况，Factual Reliability 应保持保守\n\n";

    prompt << "请严格按照以下 JSON 格式输出（不要添加任何其他文字）：\n\n";
    prompt << "{\n";
    prompt << "  \"accuracy\": <0-5的整数>,\n";
    prompt << "  \"helpfulness\": <0-5的整数>,\n";
    prompt << "  \"factual_reliability\": <0-5的整数>,\n";
    prompt << "  \"tone\": <0-5的整数>,\n";
    prompt << "  \"reason\": \"<简要说明评分理由>\",\n";
    prompt << "  \"unsupported_claims\": [\"<列出无法支持的具体声明（如有）>\"]\n";
    prompt << "}";

    return prompt.str();
}

/**
 * @brief 评估实现（无参考答案）
 */
EvaluationScore LLMJudge::evaluateStandalone(
    const ReplyCase& reply) {

    // 构造无参考答案的 Prompt
    std::string prompt = buildPrompt(reply);

    // 调用 DeepSeek API
    HttpResponse http_response = callDeepSeekAPI(prompt);

    // 检查 HTTP 状态
    if (http_response.status_code != 200) {
        std::ostringstream err;
        err << "API request failed with status " << http_response.status_code;
        if (!http_response.body.empty()) {
            err << ": " << http_response.body;
        }
        throw std::runtime_error(err.str());
    }

    // 解析响应
    try {
        return parseLLMResponse(http_response.body);
    } catch (const std::exception& e) {
        std::ostringstream err;
        err << "Failed to parse LLM response: " << e.what()
            << "\nResponse body: " << http_response.body;
        throw std::runtime_error(err.str());
    }
}

/**
 * @brief 构造 LLM Prompt
 */
std::string LLMJudge::buildPrompt(
    const ReplyCase& reply,
    const ReferenceCase& reference) const {

    std::ostringstream prompt;

    prompt << "你是一名专业的客服自动回复质量评估专家。\n\n";
    prompt << "请根据以下信息对自动回复进行多维度评分。\n\n";
    prompt << "【用户问题】\n" << reply.user_question << "\n\n";
    prompt << "【自动回复】\n" << reply.auto_reply << "\n\n";
    prompt << "【人工参考答案】\n" << reference.human_reference << "\n\n";

    if (!reference.annotator_notes.empty()) {
        prompt << "【人工分析备注】\n" << reference.annotator_notes << "\n\n";
    }

    prompt << "请分别对以下四个维度进行评分（每个维度 0-5 分）：\n\n";

    prompt << "1. Accuracy（准确性）- 权重 35%\n";
    prompt << "   5 = 完全正确，完整回答了问题，无事实错误\n";
    prompt << "   4 = 基本正确，仅存在轻微遗漏或表述不完全\n";
    prompt << "   3 = 部分正确，存在明显遗漏但核心不错\n";
    prompt << "   2 = 大部分内容不准确或存在关键错误\n";
    prompt << "   1 = 基本答非所问，方向错误\n";
    prompt << "   0 = 完全错误或与事实相反\n\n";

    prompt << "2. Helpfulness（有用性）- 权重 30%\n";
    prompt << "   5 = 可以直接帮助用户完成操作或解决问题\n";
    prompt << "   4 = 很有帮助，但缺少部分信息\n";
    prompt << "   3 = 提供了一定帮助，但用户可能需要进一步询问\n";
    prompt << "   2 = 帮助有限，用户需要自己做很多工作\n";
    prompt << "   1 = 几乎没有帮助\n";
    prompt << "   0 = 完全无用\n\n";

    prompt << "3. Factual Reliability（事实可靠性）- 权重 25%\n";
    prompt << "   5 = 没有任何未经支持的信息，所有陈述都有根据\n";
    prompt << "   4 = 基本可靠，可能有极轻微的推断\n";
    prompt << "   3 = 存在少量可疑信息，但风险较低\n";
    prompt << "   2 = 存在明显未经支持的信息\n";
    prompt << "   1 = 大量未经支持的信息\n";
    prompt << "   0 = 严重事实编造\n\n";

    prompt << "4. Tone（语气）- 权重 10%\n";
    prompt << "   5 = 礼貌、自然、专业、友好，充分尊重用户\n";
    prompt << "   4 = 基本自然，略显正式但可接受\n";
    prompt << "   3 = 略显机械，不够自然但无不当\n";
    prompt << "   2 = 明显生硬、冷漠或机械感强\n";
    prompt << "   1 = 不礼貌、消极、敷衍\n";
    prompt << "   0 = 冒犯用户、不尊重\n\n";

    prompt << "评分要求：\n";
    prompt << "- Accuracy 和 Helpfulness 必须分开评估\n";
    prompt << "- 准确的回复不一定有用（如仅说'可以退款'但不说如何退款）\n";
    prompt << "- 检查自动回复中是否存在编造的政策、流程、时间、金额等具体信息\n";
    prompt << "- 对比人工参考答案，判断是否引入了无法被参考资料支持的事实\n\n";

    prompt << "请严格按照以下 JSON 格式输出（不要添加任何其他文字）：\n\n";
    prompt << "{\n";
    prompt << "  \"accuracy\": <0-5的整数>,\n";
    prompt << "  \"helpfulness\": <0-5的整数>,\n";
    prompt << "  \"factual_reliability\": <0-5的整数>,\n";
    prompt << "  \"tone\": <0-5的整数>,\n";
    prompt << "  \"reason\": \"<简要说明评分理由>\",\n";
    prompt << "  \"unsupported_claims\": [\"<列出无法支持的具体声明（如有）>\"]\n";
    prompt << "}";

    return prompt.str();
}

/**
 * @brief 调用 DeepSeek API
 */
HttpResponse LLMJudge::callDeepSeekAPI(const std::string& prompt) {
    HttpResponse response;

    if (!curl_) {
        response.error = "CURL not initialized";
        response.status_code = -1;
        return response;
    }

    // 构造 API URL
    std::string url = api_endpoint_ + "/v1/chat/completions";

    // 构造请求 JSON
    json request_json;
    request_json["model"] = model_;
    request_json["messages"] = json::array({
        {{"role", "user"}, {"content", prompt}}
    });
    request_json["temperature"] = 0.3;  // 降低随机性以获得更一致的评分
    request_json["max_tokens"] = 2000;  // 增加输出长度，确保有空间输出 JSON 结果

    std::string request_body = request_json.dump();

    // 设置 CURL 选项
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, request_body.c_str());

    // 设置 HTTP 头
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string auth_header = "Authorization: Bearer " + api_key_;
    headers = curl_slist_append(headers, auth_header.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    // 设置超时
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, LLMConfig::REQUEST_TIMEOUT_SECS);

    // 设置响应写入回调
    std::string response_body;
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body);

    // 执行请求
    CURLcode res = curl_easy_perform(curl_);

    // 清理 headers
    curl_slist_free_all(headers);

    // 处理结果
    if (res != CURLE_OK) {
        response.error = curl_easy_strerror(res);
        response.status_code = -1;
        return response;
    }

    // 获取 HTTP 状态码
    long http_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
    response.status_code = http_code;
    response.body = response_body;

    return response;
}

/**
 * @brief 解析 LLM 响应 JSON
 */
EvaluationScore LLMJudge::parseLLMResponse(const std::string& json_body) {
    EvaluationScore score;

    try {
        json response_json = json::parse(json_body);

        // 检查是否有错误
        if (response_json.contains("error")) {
            std::string error_msg = response_json["error"].dump();
            throw std::runtime_error("API returned error: " + error_msg);
        }

        // 提取 choices[0].message.content
        if (!response_json.contains("choices") ||
            response_json["choices"].empty() ||
            !response_json["choices"][0].contains("message")) {
            throw std::runtime_error("Invalid response structure");
        }

        auto& message = response_json["choices"][0]["message"];
        std::string content = message.value("content", "");

        // DeepSeek 推理模型可能将推理过程放在 reasoning_content 中
        // 如果 content 为空，尝试从 reasoning_content 提取最终答案
        if (content.empty() && message.contains("reasoning_content")) {
            std::string reasoning = message["reasoning_content"];
            // 尝试从推理过程的末尾提取 JSON
            size_t json_start = reasoning.find("{");
            if (json_start != std::string::npos) {
                content = reasoning.substr(json_start);
            } else {
                // 如果没有找到 JSON，使用整个 reasoning_content
                content = reasoning;
            }
        }

        if (content.empty()) {
            throw std::runtime_error("Empty response content from LLM");
        }

        // 解析 LLM 返回的 JSON（可能包含 markdown 代码块）
        std::string json_str = content;

        // 尝试去除 markdown 代码块标记
        size_t code_start = content.find("```json");
        if (code_start != std::string::npos) {
            code_start += 7;  // 跳过 "```json"
            size_t code_end = content.find("```", code_start);
            if (code_end != std::string::npos) {
                json_str = content.substr(code_start, code_end - code_start);
            }
        } else {
            // 尝试查找 ``` (没有 json 标记)
            code_start = content.find("```");
            if (code_start != std::string::npos) {
                code_start += 3;
                size_t code_end = content.find("```", code_start);
                if (code_end != std::string::npos) {
                    json_str = content.substr(code_start, code_end - code_start);
                }
            }
        }

        // 去除前后空白
        json_str.erase(0, json_str.find_first_not_of(" \t\n\r"));
        json_str.erase(json_str.find_last_not_of(" \t\n\r") + 1);

        // 解析评分 JSON
        json score_json = json::parse(json_str);

        // 提取各维度评分
        score.accuracy = score_json.value("accuracy", 3);
        score.helpfulness = score_json.value("helpfulness", 3);
        score.factual_reliability = score_json.value("factual_reliability", 3);
        score.tone = score_json.value("tone", 3);

        // 提取理由和无法支持的声明
        score.reason = score_json.value("reason", "");
        if (score_json.contains("unsupported_claims") &&
            score_json["unsupported_claims"].is_array()) {
            for (const auto& claim : score_json["unsupported_claims"]) {
                if (claim.is_string()) {
                    score.unsupported_claims.push_back(claim.get<std::string>());
                }
            }
        }

        // 验证分数范围
        if (!score.isValid()) {
            throw std::runtime_error("Invalid score values (out of range)");
        }

    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    } catch (const json::type_error& e) {
        throw std::runtime_error(std::string("JSON type error: ") + e.what());
    }

    return score;
}

/**
 * @brief 获取评估器名称
 */
std::string LLMJudge::getName() const {
    return "LLMJudge (" + model_ + " @ " + api_endpoint_ + ")";
}
