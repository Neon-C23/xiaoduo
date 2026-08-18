#include "report.h"
#include "validation.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace replyx;

/**
 * @brief 将 ResultStatus 转换为字符串
 */
std::string statusToString(ResultStatus status) {
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

/**
 * @brief 生成 Markdown 格式的评估报告
 */
std::string ReportGenerator::generateMarkdownReport(
    const std::vector<EvaluationResult>& results,
    const EvaluationSummary& summary,
    const std::string& mode) {
    
    std::stringstream report;
    
    // 标题
    report << "# 自动回复质量评估报告\n\n";
    report << "**生成时间**: " << "2026-08-18" << "\n";
    report << "**评估模式**: " << mode << " Mode\n";
    report << "**总样本数**: " << summary.total_cases << "\n\n";
    
    // 汇总统计
    report << "## 汇总统计\n\n";
    report << "| 指标 | 数值 |\n";
    report << "|---|---|\n";
    report << "| 总样本数 | " << summary.total_cases << " |\n";
    report << "| PASS 数 | " << summary.pass_count << " |\n";
    report << "| WARNING 数 | " << summary.warning_count << " |\n";
    report << "| FAIL 数 | " << summary.fail_count << " |\n";
    report << "| Critical Failure 数 | " << summary.critical_failure_count << " |\n";
    report << "| 平均最终分数 | " << std::fixed << std::setprecision(2) << summary.avg_final_score << " / 5.0 |\n";
    report << "| 平均准确性 | " << std::fixed << std::setprecision(2) << summary.avg_accuracy << " / 5.0 |\n";
    report << "| 平均有用性 | " << std::fixed << std::setprecision(2) << summary.avg_helpfulness << " / 5.0 |\n";
    report << "| 平均事实可靠性 | " << std::fixed << std::setprecision(2) << summary.avg_factual_reliability << " / 5.0 |\n";
    report << "| 平均语气 | " << std::fixed << std::setprecision(2) << summary.avg_tone << " / 5.0 |\n\n";
    
    // 详细结果
    report << "## 详细结果\n\n";
    report << "| ID | 状态 | 最终分 | Acc | Help | Fact | Tone | 失败 |\n";
    report << "|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|\n";

    for (const auto& result : results) {
        report << "| " << result.id << " | ";
        report << statusToString(result.status) << " | ";
        report << std::fixed << std::setprecision(2) << result.final_score << " | ";
        report << result.score.accuracy << " | ";
        report << result.score.helpfulness << " | ";
        report << result.score.factual_reliability << " | ";
        report << result.score.tone << " | ";
        report << (result.critical_failure ? "✓" : "-") << " |\n";
    }
    
    report << "\n";
    
    // 低质量案例分析（FAIL 和 Critical Failure）
    report << "## 低质量案例分析\n\n";

    std::vector<const EvaluationResult*> failures;
    for (const auto& result : results) {
        if (result.status == ResultStatus::FAIL || result.critical_failure) {
            failures.push_back(&result);
        }
    }

    if (failures.empty()) {
        report << "没有 FAIL 或 Critical Failure 的案例。\n\n";
    } else {
        for (const auto* result : failures) {
            report << "### " << result->id << " - " << statusToString(result->status) << "\n\n";
            report << "**评分**: Acc=" << result->score.accuracy
                   << " Help=" << result->score.helpfulness
                   << " Fact=" << result->score.factual_reliability
                   << " Tone=" << result->score.tone
                   << " | 最终=" << std::fixed << std::setprecision(2) << result->final_score << "\n\n";

            if (result->critical_failure) {
                report << "⚠️ **Critical Failure**: 准确性或事实可靠性过低\n\n";
            }

            if (!result->score.reason.empty()) {
                report << "**分析**: " << result->score.reason << "\n\n";
            }

            if (!result->score.unsupported_claims.empty()) {
                report << "**无法支持的声明**:\n";
                for (const auto& claim : result->score.unsupported_claims) {
                    report << "- " << claim << "\n";
                }
                report << "\n";
            }

            report << "---\n\n";
        }
    }
    
    // 结尾
    report << "---\n\n";
    report << "## 说明\n\n";
    report << "- **PASS**: 最终分数 >= 4.0，准确性 >= 3，事实可靠性 >= 4\n";
    report << "- **WARNING**: 3.0 <= 最终分数 < 4.0，且不存在 Critical Failure\n";
    report << "- **FAIL**: 最终分数 < 3.0 或存在 Critical Failure\n";
    report << "- **Critical Failure**: 准确性 <= 1 或事实可靠性 <= 1\n\n";
    report << "本报告由 reply_eval 自动生成。\n";

    return report.str();
}

/**
 * @brief 生成包含校验结果的 Markdown 格式的评估报告
 */
std::string ReportGenerator::generateMarkdownReport(
    const std::vector<EvaluationResult>& results,
    const EvaluationSummary& summary,
    const ValidationResult& validation,
    const std::string& mode) {

    // 生成基础报告
    std::string base_report = generateMarkdownReport(results, summary, mode);

    // 在说明章节之前插入校验结果
    std::string validation_report = validation.generateReport();

    // 找到插入位置（在 "## 说明" 之前）
    size_t insert_pos = base_report.find("## 说明");
    if (insert_pos != std::string::npos) {
        base_report.insert(insert_pos, validation_report);
    } else {
        // 如果没找到，附加到末尾
        base_report += "\n" + validation_report;
    }

    return base_report;
}

/**
 * @brief 将报告写入文件
 */
bool ReportGenerator::writeReportToFile(
    const std::string& filepath,
    const std::string& content) {
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file for writing: " << filepath << "\n";
        return false;
    }
    
    file << content;
    file.close();
    
    std::cout << "Report written to: " << filepath << "\n";
    return true;
}
