#include "validation.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <iostream>

#ifdef USE_SYSTEM_NLOHMANN
#include <nlohmann/json.hpp>
#else
#include "nlohmann/json.hpp"
#endif

using namespace replyx;
using json = nlohmann::json;

// ===== HumanScore 实现 =====

bool HumanScore::hasAnyScore() const {
    return hasAccuracy() || hasHelpfulness() ||
           hasFactualReliability() || hasTone();
}

// ===== DimensionValidation 实现 =====

void DimensionValidation::calculate() {
    if (total_count > 0) {
        agreement_rate = static_cast<double>(agreement_count) / total_count;
    } else {
        agreement_rate = 0.0;
    }
}

// ===== ValidationResult 实现 =====

void ValidationResult::calculate() {
    // 计算各维度指标
    accuracy.calculate();
    helpfulness.calculate();
    factual_reliability.calculate();
    tone.calculate();

    // 计算 Critical Failure 召回率
    if (human_critical_failures > 0) {
        critical_failure_recall =
            static_cast<double>(detected_critical_failures) / human_critical_failures;
    }

    // 计算总体一致率（四个维度都一致才算完全一致）
    if (accuracy.total_count > 0) {
        overall_agreement_rate =
            static_cast<double>(agreement_count) / total_samples;
    }
}

std::string ValidationResult::generateReport() const {
    std::stringstream report;

    report << "## 评估器校验结果\n\n";

    if (total_samples == 0) {
        report << "⚠️ 暂无人工评分数据可用于校验。\n\n";
        report << "要启用此功能，请在 `human_ref.json` 中为每个 case 添加 `explicit_scores` 字段：\n\n";
        report << "```json\n";
        report << "{\n";
        report << "  \"id\": \"case_01\",\n";
        report << "  \"human_reference\": \"...\",\n";
        report << "  \"annotator_notes\": \"...\",\n";
        report << "  \"explicit_scores\": {\n";
        report << "    \"accuracy\": 3,\n";
        report << "    \"helpfulness\": 2,\n";
        report << "    \"factual_reliability\": 4,\n";
        report << "    \"tone\": 4\n";
        report << "  }\n";
        report << "}\n";
        report << "```\n\n";
        return report.str();
    }

    report << "**校验样本数**: " << total_samples << "\n\n";

    // 各维度一致性表格
    report << "### 各维度一致性\n\n";
    report << "| 维度 | 一致率 | MAE |\n";
    report << "|:---|---:|---:|\n";
    report << "| Accuracy | " << std::fixed << std::setprecision(1)
           << (accuracy.agreement_rate * 100) << "% | "
           << std::setprecision(2) << accuracy.mae << " |\n";
    report << "| Helpfulness | " << (helpfulness.agreement_rate * 100) << "% | "
           << helpfulness.mae << " |\n";
    report << "| Factual Reliability | " << (factual_reliability.agreement_rate * 100) << "% | "
           << factual_reliability.mae << " |\n";
    report << "| Tone | " << (tone.agreement_rate * 100) << "% | "
           << tone.mae << " |\n";
    report << "| **总体** | **" << (overall_agreement_rate * 100) << "%** | - |\n\n";

    // Critical Failure 检测
    report << "### Critical Failure 检测\n\n";
    report << "- 人工标注的关键失败: " << human_critical_failures << "\n";
    report << "- 评估器检测到的: " << detected_critical_failures << "\n";
    if (human_critical_failures > 0) {
        report << "- 召回率: " << std::setprecision(1)
               << (critical_failure_recall * 100) << "%\n\n";
    } else {
        report << "\n";
    }

    // 评估结论
    report << "### 评估结论\n\n";

    if (overall_agreement_rate >= 0.8) {
        report << "✅ 评估器与人工标注一致性较高（≥80%），评估方法可信。\n";
    } else if (overall_agreement_rate >= 0.6) {
        report << "⚠️ 评估器与人工标注一致性中等（60-80%），建议调优评估规则。\n";
    } else {
        report << "❌ 评估器与人工标注一致性较低（<60%），需要改进评估方法。\n";
    }

    if (accuracy.mae <= 0.5 && helpfulness.mae <= 0.5) {
        report << "- 核心维度（Accuracy、Helpfulness）误差较小。\n";
    } else if (accuracy.mae > 1.0) {
        report << "- ⚠️ Accuracy 误差较大，需要调整评分规则。\n";
    }

    if (human_critical_failures > 0 && critical_failure_recall < 0.5) {
        report << "- ⚠️ Critical Failure 检测率较低，需要加强检测规则。\n";
    }

    report << "\n";

    return report.str();
}

// ===== EvaluatorValidator 实现 =====

bool EvaluatorValidator::loadHumanScores(const std::string& filepath) {
    human_scores_.clear();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Warning: Cannot open human scores file: " << filepath << "\n";
        return false;
    }

    try {
        json data = json::parse(file);

        for (const auto& item : data) {
            if (!item.contains("id")) {
                continue;
            }

            std::string id = item["id"].get<std::string>();
            HumanScore score;

            // 尝试读取显式评分
            if (item.contains("explicit_scores") && item["explicit_scores"].is_object()) {
                auto& scores = item["explicit_scores"];
                if (scores.contains("accuracy")) {
                    score.accuracy = scores["accuracy"].get<int>();
                }
                if (scores.contains("helpfulness")) {
                    score.helpfulness = scores["helpfulness"].get<int>();
                }
                if (scores.contains("factual_reliability")) {
                    score.factual_reliability = scores["factual_reliability"].get<int>();
                }
                if (scores.contains("tone")) {
                    score.tone = scores["tone"].get<int>();
                }
            }

            // 如果没有显式评分，尝试从备注推断
            if (!score.hasAnyScore() && item.contains("annotator_notes")) {
                std::string notes = item["annotator_notes"].get<std::string>();
                score = inferScoreFromNotes(notes);
            }

            human_scores_[id] = score;
        }

        return true;

    } catch (const json::parse_error& e) {
        std::cerr << "Error parsing human scores JSON: " << e.what() << "\n";
        return false;
    }
}

ValidationResult EvaluatorValidator::validate(
    const std::vector<EvaluationResult>& evaluator_results) const {

    ValidationResult result;
    result.accuracy.dimension_name = "Accuracy";
    result.helpfulness.dimension_name = "Helpfulness";
    result.factual_reliability.dimension_name = "Factual Reliability";
    result.tone.dimension_name = "Tone";

    for (const auto& eval_result : evaluator_results) {
        auto it = human_scores_.find(eval_result.id);
        if (it == human_scores_.end()) {
            continue;
        }

        const HumanScore& human = it->second;
        const EvaluationScore& eval = eval_result.score;

        // 只有当有人工评分时才统计
        if (!human.hasAnyScore()) {
            continue;
        }

        result.total_samples++;

        // Accuracy
        if (human.hasAccuracy()) {
            result.accuracy.total_count++;
            if (human.accuracy == eval.accuracy) {
                result.accuracy.agreement_count++;
            }
            result.accuracy.mae += std::abs(human.accuracy - eval.accuracy);
        }

        // Helpfulness
        if (human.hasHelpfulness()) {
            result.helpfulness.total_count++;
            if (human.helpfulness == eval.helpfulness) {
                result.helpfulness.agreement_count++;
            }
            result.helpfulness.mae += std::abs(human.helpfulness - eval.helpfulness);
        }

        // Factual Reliability
        if (human.hasFactualReliability()) {
            result.factual_reliability.total_count++;
            if (human.factual_reliability == eval.factual_reliability) {
                result.factual_reliability.agreement_count++;
            }
            result.factual_reliability.mae += std::abs(human.factual_reliability - eval.factual_reliability);
        }

        // Tone
        if (human.hasTone()) {
            result.tone.total_count++;
            if (human.tone == eval.tone) {
                result.tone.agreement_count++;
            }
            result.tone.mae += std::abs(human.tone - eval.tone);
        }

        // 完全一致统计
        bool all_match = true;
        if (human.hasAccuracy() && human.accuracy != eval.accuracy) all_match = false;
        if (human.hasHelpfulness() && human.helpfulness != eval.helpfulness) all_match = false;
        if (human.hasFactualReliability() && human.factual_reliability != eval.factual_reliability) all_match = false;
        if (human.hasTone() && human.tone != eval.tone) all_match = false;

        if (all_match && human.hasAnyScore()) {
            result.agreement_count++;
        }
    }

    // 计算 MAE（除以样本数）
    if (result.accuracy.total_count > 0) {
        result.accuracy.mae /= result.accuracy.total_count;
    }
    if (result.helpfulness.total_count > 0) {
        result.helpfulness.mae /= result.helpfulness.total_count;
    }
    if (result.factual_reliability.total_count > 0) {
        result.factual_reliability.mae /= result.factual_reliability.total_count;
    }
    if (result.tone.total_count > 0) {
        result.tone.mae /= result.tone.total_count;
    }

    result.calculate();

    return result;
}

HumanScore EvaluatorValidator::inferScoreFromNotes(const std::string& notes) const {
    HumanScore score;

    // 简单的关键词分析（可作为基础实现）
    std::string lower_notes = notes;
    std::transform(lower_notes.begin(), lower_notes.end(), lower_notes.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // 检测负面关键词
    bool has_negative = (lower_notes.find("不对") != std::string::npos ||
                        lower_notes.find("错误") != std::string::npos ||
                        lower_notes.find("问题") != std::string::npos ||
                        lower_notes.find("没有") != std::string::npos);

    // 检测正面关键词
    bool has_positive = (lower_notes.find("好") != std::string::npos ||
                        lower_notes.find("正确") != std::string::npos ||
                        lower_notes.find("可以") != std::string::npos);

    // 基础推断（这个实现比较简单，实际使用时可以更复杂）
    if (has_negative) {
        // 有问题，根据严重程度给分
        if (lower_notes.find("严重") != std::string::npos ||
            lower_notes.find("关键") != std::string::npos) {
            score.accuracy = 1;
        } else {
            score.accuracy = 2;
        }
    } else if (has_positive) {
        score.accuracy = 4;
    }

    return score;
}

bool EvaluatorValidator::detectHumanCriticalFailure(const std::string& notes) const {
    std::string lower_notes = notes;
    std::transform(lower_notes.begin(), lower_notes.end(), lower_notes.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // 检测关键失败关键词
    return (lower_notes.find("严重错误") != std::string::npos ||
            lower_notes.find("完全错误") != std::string::npos ||
            lower_notes.find("答非所问") != std::string::npos ||
            lower_notes.find("编造") != std::string::npos ||
            lower_notes.find("瞎编") != std::string::npos);
}
