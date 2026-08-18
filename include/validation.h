#pragma once

#include "models.h"
#include <vector>
#include <string>
#include <map>

namespace replyx {

/**
 * @brief 人工评分结构体
 * 用于与评估器结果对比
 */
struct HumanScore {
    int accuracy = -1;           // -1 表示未评分
    int helpfulness = -1;
    int factual_reliability = -1;
    int tone = -1;

    // 是否包含该维度的评分
    bool hasAccuracy() const { return accuracy >= 0; }
    bool hasHelpfulness() const { return helpfulness >= 0; }
    bool hasFactualReliability() const { return factual_reliability >= 0; }
    bool hasTone() const { return tone >= 0; }

    // 是否有任何评分
    bool hasAnyScore() const;
};

/**
 * @brief 单个维度校验结果
 */
struct DimensionValidation {
    std::string dimension_name;     // 维度名称
    int agreement_count = 0;        // 完全一致的数量
    int total_count = 0;             // 总数量
    double agreement_rate = 0.0;    // 一致率
    double mae = 0.0;               // Mean Absolute Error

    void calculate();
};

/**
 * @brief 评估器校验结果
 */
struct ValidationResult {
    // 各维度校验
    DimensionValidation accuracy;
    DimensionValidation helpfulness;
    DimensionValidation factual_reliability;
    DimensionValidation tone;

    // Critical Failure 检测统计
    int human_critical_failures = 0;     // 人工标注的关键失败数
    int detected_critical_failures = 0; // 评估器检测到的关键失败数
    double critical_failure_recall = 0.0;  // 检测召回率

    // 总体统计
    int total_samples = 0;
    int agreement_count = 0;              // 完全一致（所有维度）的数量
    double overall_agreement_rate = 0.0;  // 所有维度都一致的比例

    void calculate();
    std::string generateReport() const;
};

/**
 * @brief 评估器校验器
 *
 * 功能：
 * 1. 加载人工评分数据
 * 2. 与评估器结果对比
 * 3. 计算各项指标
 * 4. 生成校验报告
 */
class EvaluatorValidator {
public:
    /**
     * @brief 构造函数
     */
    EvaluatorValidator() = default;

    /**
     * @brief 从 JSON 文件加载人工评分
     * 如果 JSON 中包含 explicit_scores 字段则直接使用
     * 否则返回空映射（表示无人工评分数据）
     */
    bool loadHumanScores(const std::string& filepath);

    /**
     * @brief 对比评估器结果与人工评分
     * @param evaluator_results 评估器的评估结果
     * @return 校验统计结果
     */
    ValidationResult validate(
        const std::vector<EvaluationResult>& evaluator_results
    ) const;

    /**
     * @brief 获取人工评分数据
     */
    const std::map<std::string, HumanScore>& getHumanScores() const {
        return human_scores_;
    }

private:
    std::map<std::string, HumanScore> human_scores_;

    /**
     * @brief 从备注中推断隐式评分（可选功能）
     * 分析 annotator_notes 的情感倾向
     */
    HumanScore inferScoreFromNotes(const std::string& notes) const;

    /**
     * @brief 检测人工标注中的关键失败
     * 分析备注中是否明确指出严重问题
     */
    bool detectHumanCriticalFailure(const std::string& notes) const;
};

}  // namespace replyx
