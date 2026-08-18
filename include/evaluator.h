#pragma once

#include <vector>
#include <memory>
#include "models.h"
#include "judge.h"

namespace replyx {

/**
 * @brief 评估器主类，管理评估流程和计算
 */
class Evaluator {
public:
    /**
     * @brief 计算加权最终分数
     * @param score 各维度分数
     * @return 加权后的 0-5 分数
     */
    static double calculateFinalScore(const EvaluationScore& score);
    
    /**
     * @brief 判定是否为关键失败
     * @param score 各维度分数
     * @return true 如果 Accuracy <= 1 或 FactualReliability <= 1
     */
    static bool isCriticalFailure(const EvaluationScore& score);
    
    /**
     * @brief 根据最终分数和关键失败判定结果状态
     * @param final_score 加权后的分数
     * @param critical_failure 是否为关键失败
     * @return PASS / WARNING / FAIL
     */
    static ResultStatus determineStatus(double final_score, bool critical_failure);
    
    /**
     * @brief 对整个 Reply 列表进行评估
     * @param replies 待评估的回复列表
     * @param references 参考答案列表
     * @param judge 评估器实现
     * @return 评估结果列表
     */
    static std::vector<EvaluationResult> evaluateBatch(
        const std::vector<ReplyCase>& replies,
        const std::vector<ReferenceCase>& references,
        IJudge* judge
    );

    /**
     * @brief 对整个 Reply 列表进行评估（无参考答案）
     * @param replies 待评估的回复列表
     * @param judge 评估器实现
     * @return 评估结果列表
     */
    static std::vector<EvaluationResult> evaluateBatchStandalone(
        const std::vector<ReplyCase>& replies,
        IJudge* judge
    );
    
    /**
     * @brief 汇总评估统计信息
     * @param results 评估结果列表
     * @return 汇总统计
     */
    static EvaluationSummary summarize(const std::vector<EvaluationResult>& results);
};

}  // namespace replyx
