#pragma once

#include <string>
#include <vector>
#include <optional>
#include "config.h"

namespace replyx {

/**
 * @brief 评估得分结构体
 */
struct EvaluationScore {
    int accuracy = 0;                               // 准确性：0-5
    int helpfulness = 0;                            // 有用性：0-5
    int factual_reliability = 0;                    // 事实可靠性：0-5
    int tone = 0;                                   // 语气：0-5
    
    std::string reason;                             // Judge 的判断理由
    std::vector<std::string> unsupported_claims;    // 无法支持的声明列表
    
    /**
     * @brief 验证分数是否在有效范围内
     */
    bool isValid() const;
};

/**
 * @brief 自动回复 Case 结构体
 */
struct ReplyCase {
    std::string id;                 // 唯一标识
    std::string user_question;      // 用户问题
    std::string auto_reply;         // 自动生成的回复
};

/**
 * @brief 人工参考 Case 结构体
 */
struct ReferenceCase {
    std::string id;                 // 对应的 Reply ID
    std::string human_reference;    // 人工参考答案
    std::string annotator_notes;    // 人工标注备注
};

/**
 * @brief 评估结果结构体
 */
struct EvaluationResult {
    std::string id;                                 // 对应的 Reply ID
    EvaluationScore score;                          // 各维度评分
    double final_score = 0.0;                       // 加权后的最终分数 (0-5)
    bool critical_failure = false;                  // 是否判定为关键失败
    ResultStatus status = ResultStatus::FAIL;       // PASS / WARNING / FAIL
    std::string judge_reason;                       // Judge 的详细理由
    std::string judge_mode;                         // 使用的评估器 (Mock / LLM)
    
    /**
     * @brief 验证评估结果的完整性
     */
    bool isValid() const;
};

/**
 * @brief 评估汇总统计信息
 */
struct EvaluationSummary {
    int total_cases = 0;
    int pass_count = 0;
    int warning_count = 0;
    int fail_count = 0;
    int critical_failure_count = 0;
    
    double avg_final_score = 0.0;
    double avg_accuracy = 0.0;
    double avg_helpfulness = 0.0;
    double avg_factual_reliability = 0.0;
    double avg_tone = 0.0;
};

}  // namespace replyx
