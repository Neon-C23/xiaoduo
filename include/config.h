#pragma once

#include <string>
#include <vector>

namespace replyx {

/**
 * @brief 全局配置常量和枚举定义
 */

// ===== 权重配置 =====
constexpr double WEIGHT_ACCURACY = 0.35;
constexpr double WEIGHT_HELPFULNESS = 0.30;
constexpr double WEIGHT_FACTUAL_RELIABILITY = 0.25;
constexpr double WEIGHT_TONE = 0.10;

// ===== 评分范围 =====
constexpr int MIN_SCORE = 0;
constexpr int MAX_SCORE = 5;

// ===== 分数阈值 =====
constexpr double PASS_THRESHOLD = 4.0;
constexpr double WARNING_THRESHOLD = 3.0;
constexpr int CRITICAL_FAILURE_ACCURACY_THRESHOLD = 1;
constexpr int CRITICAL_FAILURE_RELIABILITY_THRESHOLD = 1;

// ===== Judge 模式 =====
enum class JudgeMode {
    MOCK,  // 离线规则型评估
    LLM    // LLM API 评估
};

// ===== 结果状态 =====
enum class ResultStatus {
    PASS,
    WARNING,
    FAIL
};

std::string resultStatusToString(ResultStatus status);

// ===== DeepSeek LLM 配置 =====
namespace LLMConfig {
    constexpr const char* DEEPSEEK_BASE_URL_OPENAI = "https://api.deepseek.com";
    constexpr const char* DEEPSEEK_BASE_URL_ANTHROPIC = "https://api.deepseek.com/anthropic";
    constexpr const char* DEFAULT_MODEL = "deepseek-v4-flash";
    constexpr const char* ALTERNATIVE_MODEL = "deepseek-v4-pro";
    constexpr const char* API_KEY_ENV_VAR = "LLM_API_KEY";
    constexpr int REQUEST_TIMEOUT_SECS = 30;
    constexpr int MAX_RETRIES = 3;
}

}  // namespace replyx
