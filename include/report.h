#pragma once

#include <string>
#include <vector>
#include "models.h"

// 前置声明
namespace replyx {
    struct ValidationResult;
}

namespace replyx {

/**
 * @brief 报告生成器
 */
class ReportGenerator {
public:
    /**
     * @brief 生成 Markdown 格式的评估报告
     * @param results 评估结果列表
     * @param summary 汇总统计信息
     * @param mode 使用的评估模式 (Mock / LLM)
     * @return Markdown 格式的报告字符串
     */
    static std::string generateMarkdownReport(
        const std::vector<EvaluationResult>& results,
        const EvaluationSummary& summary,
        const std::string& mode
    );

    /**
     * @brief 生成包含校验结果的 Markdown 格式报告
     * @param results 评估结果列表
     * @param summary 汇总统计信息
     * @param validation 校验结果
     * @param mode 使用的评估模式 (Mock / LLM)
     * @return Markdown 格式的报告字符串
     */
    static std::string generateMarkdownReport(
        const std::vector<EvaluationResult>& results,
        const EvaluationSummary& summary,
        const ValidationResult& validation,
        const std::string& mode
    );

    /**
     * @brief 将报告写入文件
     * @param filepath 输出文件路径
     * @param content 报告内容
     * @return true 如果写入成功
     */
    static bool writeReportToFile(
        const std::string& filepath,
        const std::string& content
    );
};

}  // namespace replyx
