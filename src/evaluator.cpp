#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <iostream>
#include "evaluator.h"
#include "config.h"

using namespace replyx;

/**
 * @brief 计算加权最终分数
 */
double Evaluator::calculateFinalScore(const EvaluationScore& score) {
    if (!score.isValid()) {
        throw std::invalid_argument("Invalid evaluation score");
    }
    
    double final_score = 
        score.accuracy * WEIGHT_ACCURACY +
        score.helpfulness * WEIGHT_HELPFULNESS +
        score.factual_reliability * WEIGHT_FACTUAL_RELIABILITY +
        score.tone * WEIGHT_TONE;
    
    // 确保分数在 0-5 之间
    if (final_score < 0.0) final_score = 0.0;
    if (final_score > 5.0) final_score = 5.0;
    
    return final_score;
}

/**
 * @brief 判定是否为关键失败
 */
bool Evaluator::isCriticalFailure(const EvaluationScore& score) {
    return score.accuracy <= CRITICAL_FAILURE_ACCURACY_THRESHOLD ||
           score.factual_reliability <= CRITICAL_FAILURE_RELIABILITY_THRESHOLD;
}

/**
 * @brief 根据最终分数确定状态
 */
ResultStatus Evaluator::determineStatus(double final_score, bool critical_failure) {
    if (critical_failure) {
        return ResultStatus::FAIL;
    }
    
    if (final_score >= PASS_THRESHOLD) {
        // 还需要检查准确性和事实可靠性的最低要求（这部分在实际评估中检查）
        return ResultStatus::PASS;
    }
    
    if (final_score >= WARNING_THRESHOLD) {
        return ResultStatus::WARNING;
    }
    
    return ResultStatus::FAIL;
}

/**
 * @brief 批量评估
 */
std::vector<EvaluationResult> Evaluator::evaluateBatch(
    const std::vector<ReplyCase>& replies,
    const std::vector<ReferenceCase>& references,
    IJudge* judge) {

    if (!judge) {
        throw std::invalid_argument("Judge cannot be null");
    }

    std::vector<EvaluationResult> results;

    for (const auto& reply : replies) {
        // 查找对应的参考答案
        auto ref_it = std::find_if(references.begin(), references.end(),
            [&reply](const ReferenceCase& ref) { return ref.id == reply.id; });

        if (ref_it == references.end()) {
            std::cerr << "Warning: No reference found for reply ID: " << reply.id << "\n";
            continue;
        }

        try {
            EvaluationScore score = judge->evaluate(reply, *ref_it);
            double final_score = calculateFinalScore(score);
            bool critical_failure = isCriticalFailure(score);

            EvaluationResult result;
            result.id = reply.id;
            result.score = score;
            result.final_score = final_score;
            result.critical_failure = critical_failure;
            result.status = determineStatus(final_score, critical_failure);
            result.judge_mode = judge->getName();

            results.push_back(result);
        } catch (const std::exception& e) {
            std::cerr << "Error evaluating reply " << reply.id << ": " << e.what() << "\n";
        }
    }

    return results;
}

/**
 * @brief 批量评估（无参考答案）
 */
std::vector<EvaluationResult> Evaluator::evaluateBatchStandalone(
    const std::vector<ReplyCase>& replies,
    IJudge* judge) {

    if (!judge) {
        throw std::invalid_argument("Judge cannot be null");
    }

    // 检查评估器是否支持无参考评估
    if (!judge->supportsStandalone()) {
        throw std::runtime_error("Judge does not support standalone evaluation");
    }

    std::vector<EvaluationResult> results;

    for (const auto& reply : replies) {
        try {
            EvaluationScore score = judge->evaluateStandalone(reply);
            double final_score = calculateFinalScore(score);
            bool critical_failure = isCriticalFailure(score);

            EvaluationResult result;
            result.id = reply.id;
            result.score = score;
            result.final_score = final_score;
            result.critical_failure = critical_failure;
            result.status = determineStatus(final_score, critical_failure);
            result.judge_mode = judge->getName() + " (无参考)";

            results.push_back(result);
        } catch (const std::exception& e) {
            std::cerr << "Error evaluating reply " << reply.id << ": " << e.what() << "\n";
        }
    }

    return results;
}

/**
 * @brief 汇总统计信息
 */
EvaluationSummary Evaluator::summarize(const std::vector<EvaluationResult>& results) {
    EvaluationSummary summary;
    summary.total_cases = results.size();
    
    if (results.empty()) {
        return summary;
    }
    
    double sum_final_score = 0.0;
    double sum_accuracy = 0.0;
    double sum_helpfulness = 0.0;
    double sum_factual_reliability = 0.0;
    double sum_tone = 0.0;
    
    for (const auto& result : results) {
        switch (result.status) {
            case ResultStatus::PASS:
                summary.pass_count++;
                break;
            case ResultStatus::WARNING:
                summary.warning_count++;
                break;
            case ResultStatus::FAIL:
                summary.fail_count++;
                break;
        }
        
        if (result.critical_failure) {
            summary.critical_failure_count++;
        }
        
        sum_final_score += result.final_score;
        sum_accuracy += result.score.accuracy;
        sum_helpfulness += result.score.helpfulness;
        sum_factual_reliability += result.score.factual_reliability;
        sum_tone += result.score.tone;
    }
    
    summary.avg_final_score = sum_final_score / results.size();
    summary.avg_accuracy = sum_accuracy / results.size();
    summary.avg_helpfulness = sum_helpfulness / results.size();
    summary.avg_factual_reliability = sum_factual_reliability / results.size();
    summary.avg_tone = sum_tone / results.size();
    
    return summary;
}
