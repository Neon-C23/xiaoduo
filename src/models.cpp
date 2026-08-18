#include "models.h"
#include "config.h"

using namespace replyx;

/**
 * @brief 验证评估分数是否有效
 */
bool EvaluationScore::isValid() const {
    return accuracy >= MIN_SCORE && accuracy <= MAX_SCORE &&
           helpfulness >= MIN_SCORE && helpfulness <= MAX_SCORE &&
           factual_reliability >= MIN_SCORE && factual_reliability <= MAX_SCORE &&
           tone >= MIN_SCORE && tone <= MAX_SCORE;
}

/**
 * @brief 验证评估结果的完整性
 */
bool EvaluationResult::isValid() const {
    return !id.empty() && score.isValid() &&
           final_score >= 0.0 && final_score <= 5.0;
}

/**
 * @brief 将结果状态转换为字符串
 */
std::string replyx::resultStatusToString(ResultStatus status) {
    switch (status) {
        case ResultStatus::PASS:
            return "PASS";
        case ResultStatus::WARNING:
            return "WARNING";
        case ResultStatus::FAIL:
            return "FAIL";
        default:
            return "UNKNOWN";
    }
}
