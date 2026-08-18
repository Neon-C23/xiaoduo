#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdlib>
#include <iomanip>
#include "config.h"
#include "models.h"
#include "evaluator.h"
#include "judge.h"
#include "json_loader.h"
#include "mock_judge.h"
#include "llm_judge.h"
#include "report.h"
#include "validation.h"

using namespace replyx;

/**
 * @brief 命令行参数结构体
 */
struct CLIArgs {
    JudgeMode mode = JudgeMode::MOCK;
    std::string input_file;
    std::string reference_file;
    std::string output_file = "reports/report.md";
    std::string api_endpoint = LLMConfig::DEEPSEEK_BASE_URL_OPENAI;
    std::string model = LLMConfig::DEFAULT_MODEL;
    bool skip_reference = false;  // 跳过参考答案加载
};

/**
 * @brief 打印帮助信息
 */
void printHelp(const char* program_name) {
    std::cout << "自动回复质量评估流水线 - reply_eval\n\n";
    std::cout << "用法: " << program_name << " [选项]\n\n";
    std::cout << "选项:\n";
    std::cout << "  --mode MODE              评估模式: mock 或 llm (默认: mock)\n";
    std::cout << "  --input FILE             输入 JSON 文件路径 (包含自动回复数据)\n";
    std::cout << "  --reference FILE         参考文件路径 (包含人工标注数据)\n";
    std::cout << "  --skip-reference         跳过参考答案加载，仅基于回复本身评估\n";
    std::cout << "  --output FILE            输出报告路径 (默认: reports/report.md)\n";
    std::cout << "  --api-endpoint URL       LLM API 端点 (默认: " << LLMConfig::DEEPSEEK_BASE_URL_OPENAI << ")\n";
    std::cout << "  --model MODEL            LLM 模型名称 (默认: " << LLMConfig::DEFAULT_MODEL << ")\n";
    std::cout << "  --help                   显示本帮助信息\n\n";
    std::cout << "示例:\n";
    std::cout << "  # Mock 模式（有参考答案）\n";
    std::cout << "  " << program_name << " --mode mock --input doc/auto_replies.json --reference doc/human_ref.json\n\n";
    std::cout << "  # Mock 模式（无参考答案）\n";
    std::cout << "  " << program_name << " --mode mock --input doc/auto_replies.json --skip-reference\n\n";
    std::cout << "  # LLM 模式（需要设置 LLM_API_KEY 环境变量）\n";
    std::cout << "  export LLM_API_KEY=\"sk-xxxxxxx\"\n";
    std::cout << "  " << program_name << " --mode llm --input doc/auto_replies.json --skip-reference\n";
}

/**
 * @brief 解析命令行参数
 */
bool parseArgs(int argc, char* argv[], CLIArgs& args) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return false;
        }
        else if (arg == "--mode" && i + 1 < argc) {
            std::string mode = argv[++i];
            if (mode == "mock") {
                args.mode = JudgeMode::MOCK;
            } else if (mode == "llm") {
                args.mode = JudgeMode::LLM;
            } else {
                std::cerr << "错误: 无效的 mode: " << mode << " (应该是 mock 或 llm)\n";
                return false;
            }
        }
        else if (arg == "--input" && i + 1 < argc) {
            args.input_file = argv[++i];
        }
        else if (arg == "--reference" && i + 1 < argc) {
            args.reference_file = argv[++i];
        }
        else if (arg == "--output" && i + 1 < argc) {
            args.output_file = argv[++i];
        }
        else if (arg == "--api-endpoint" && i + 1 < argc) {
            args.api_endpoint = argv[++i];
        }
        else if (arg == "--model" && i + 1 < argc) {
            args.model = argv[++i];
        }
        else if (arg == "--skip-reference") {
            args.skip_reference = true;
        }
        else {
            std::cerr << "错误: 未知的参数: " << arg << "\n";
            return false;
        }
    }

    // 验证必填参数
    if (args.input_file.empty()) {
        std::cerr << "错误: 缺少必填参数 --input\n";
        return false;
    }
    if (!args.skip_reference && args.reference_file.empty()) {
        std::cerr << "错误: 缺少必填参数 --reference (或使用 --skip-reference 跳过)\n";
        return false;
    }
    
    return true;
}

/**
 * @brief 获取环境变量，支持默认值
 */
std::string getEnvVar(const std::string& var_name, const std::string& default_value = "") {
    const char* val = std::getenv(var_name.c_str());
    return val ? std::string(val) : default_value;
}

/**
 * @brief 主程序入口
 */
int main(int argc, char* argv[]) {
    // 解析命令行参数
    CLIArgs args;
    if (argc == 1) {
        printHelp(argv[0]);
        return 0;
    }
    
    if (!parseArgs(argc, argv, args)) {
        return 1;
    }
    
    std::cout << "======================================\n";
    std::cout << "自动回复质量评估流水线\n";
    std::cout << "======================================\n";
    std::cout << "模式: " << (args.mode == JudgeMode::MOCK ? "Mock" : "LLM") << "\n";
    std::cout << "输入文件: " << args.input_file << "\n";
    std::cout << "参考文件: " << args.reference_file << "\n";
    std::cout << "输出文件: " << args.output_file << "\n";
    
    if (args.mode == JudgeMode::LLM) {
        std::cout << "API 端点: " << args.api_endpoint << "\n";
        std::cout << "模型: " << args.model << "\n";
        std::string api_key = getEnvVar(LLMConfig::API_KEY_ENV_VAR);
        if (api_key.empty()) {
            std::cerr << "错误: 未设置 LLM_API_KEY 环境变量\n";
            return 1;
        }
    }
    
    std::cout << "======================================\n\n";
    
    try {
        // 第 1 步：加载数据
        std::cout << "1. 加载输入数据...\n";
        auto replies = loadReplies(args.input_file);

        if (replies.empty()) {
            std::cerr << "错误: 无法加载数据\n";
            return 1;
        }

        std::cout << "   成功加载 " << replies.size() << " 条回复\n";

        std::vector<ReferenceCase> references;
        if (!args.skip_reference) {
            references = loadReferences(args.reference_file);
            if (references.empty()) {
                std::cerr << "错误: 无法加载参考数据\n";
                return 1;
            }
            std::cout << "   成功加载 " << references.size() << " 条参考数据\n";
        } else {
            std::cout << "   跳过参考答案加载（无参考评估模式）\n";
        }
        std::cout << "\n";

        // 第 2 步：创建评估器
        std::cout << "2. 初始化评估器...\n";
        std::unique_ptr<IJudge> judge;

        if (args.mode == JudgeMode::MOCK) {
            judge = std::make_unique<MockJudge>();
            std::cout << "   使用 Mock 模式";
        } else {
            try {
                judge = std::make_unique<LLMJudge>(args.api_endpoint, args.model);
                std::cout << "   使用 LLM 模式 (" << args.model << ")";
            } catch (const std::exception& e) {
                std::cerr << "错误: " << e.what() << "\n";
                return 1;
            }
        }

        if (args.skip_reference) {
            std::cout << "（无参考答案）\n\n";
        } else {
            std::cout << "（有参考答案）\n\n";
        }

        // 第 3 步：执行评估
        std::cout << "3. 执行评估...\n";
        std::vector<EvaluationResult> results;

        if (args.skip_reference) {
            if (!judge->supportsStandalone()) {
                std::cerr << "错误: 当前评估器不支持无参考答案评估\n";
                return 1;
            }
            results = Evaluator::evaluateBatchStandalone(replies, judge.get());
        } else {
            results = Evaluator::evaluateBatch(replies, references, judge.get());
        }

        std::cout << "   成功评估 " << results.size() << " 条回复\n\n";
        
        // 第 4 步：汇总统计
        std::cout << "4. 生成统计报告...\n";
        auto summary = Evaluator::summarize(results);
        
        std::cout << "   PASS: " << summary.pass_count << "\n";
        std::cout << "   WARNING: " << summary.warning_count << "\n";
        std::cout << "   FAIL: " << summary.fail_count << "\n";
        std::cout << "   Critical Failure: " << summary.critical_failure_count << "\n";
        std::cout << "   平均最终分数: " << std::fixed << std::setprecision(2) 
                  << summary.avg_final_score << " / 5.0\n\n";
        
        // 第 5 步：校验评估器（可选）
        std::cout << "5. 校验评估器...\n";
        ValidationResult validation;
        if (args.skip_reference) {
            std::cout << "   无参考答案模式，跳过校验\n\n";
        } else {
            EvaluatorValidator validator;
            if (validator.loadHumanScores(args.reference_file)) {
                validation = validator.validate(results);
                std::cout << "   校验完成，样本数: " << validation.total_samples << "\n\n";
            } else {
                std::cout << "   未找到人工评分数据，跳过校验\n\n";
            }
        }

        // 第 6 步：生成报告
        std::cout << "6. 生成 Markdown 报告...\n";
        std::string mode_name = (args.mode == JudgeMode::MOCK) ? "Mock" : "LLM";
        if (args.skip_reference) {
            mode_name += " (无参考)";
        }
        std::string report;
        if (!args.skip_reference && validation.total_samples > 0) {
            report = ReportGenerator::generateMarkdownReport(results, summary, validation, mode_name);
        } else {
            report = ReportGenerator::generateMarkdownReport(results, summary, mode_name);
        }

        // 第 7 步：输出报告
        std::cout << "7. 写入报告文件...\n";
        if (!ReportGenerator::writeReportToFile(args.output_file, report)) {
            std::cerr << "错误: 无法写入报告文件\n";
            return 1;
        }

        std::cout << "\n======================================\n";
        std::cout << "评估完成！\n";
        std::cout << "报告已保存到: " << args.output_file << "\n";
        if (validation.total_samples > 0) {
            std::cout << "校验结果已包含在报告中\n";
        }
        std::cout << "======================================\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << "\n";
        return 1;
    }
}
