#include "mock_judge.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>

using namespace replyx;

/**
 * @brief 辅助函数：将字符串转换为小写
 */
static std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

/**
 * @brief 辅助函数：计算两个字符串的相似度（简单的词汇重叠率）
 */
static double calculateSimilarity(const std::string& str1, const std::string& str2) {
    if (str1.empty() || str2.empty()) return 0.0;
    
    std::string s1_lower = toLower(str1);
    std::string s2_lower = toLower(str2);
    
    // 计算词汇重叠
    int overlap_count = 0;
    std::istringstream iss1(s1_lower);
    std::string word1;
    
    while (iss1 >> word1) {
        if (s2_lower.find(word1) != std::string::npos) {
            overlap_count++;
        }
    }
    
    int total_words = 0;
    std::istringstream iss2(s1_lower);
    while (iss2 >> word1) total_words++;
    
    if (total_words == 0) return 0.0;
    return static_cast<double>(overlap_count) / total_words;
}

/**
 * @brief 辅助函数：检查是否包含关键词
 */
static int countKeywords(const std::string& text, const std::vector<std::string>& keywords) {
    std::string text_lower = toLower(text);
    int count = 0;
    
    for (const auto& keyword : keywords) {
        std::string keyword_lower = toLower(keyword);
        size_t pos = 0;
        while ((pos = text_lower.find(keyword_lower, pos)) != std::string::npos) {
            count++;
            pos += keyword_lower.length();
        }
    }
    
    return count;
}

/**
 * @brief MockJudge 构造函数
 */
MockJudge::MockJudge() {
    // 初始化规则库（未来可扩展）
}

/**
 * @brief Accuracy 评分逻辑
 */
static int scoreAccuracy(const ReplyCase& reply, const ReferenceCase& reference) {
    // 基础长度检查
    if (reply.auto_reply.length() < 20) {
        return 1; // 回复太短，可能不够准确
    }
    
    // 相似度检查
    double similarity = calculateSimilarity(reply.auto_reply, reference.human_reference);
    
    // 关键词检查：是否包含"帮您处理", "为您处理"等主动表述
    std::vector<std::string> proactive_keywords = {
        "帮您", "为您", "我来", "我们来", "直接"
    };
    int proactive_count = countKeywords(reply.auto_reply, proactive_keywords);
    
    // 检查是否推诿给用户（不好的信号）
    std::vector<std::string> pushoff_keywords = {
        "自己", "自己去", "你自己", "请自己", "自己查", "自己确认"
    };
    int pushoff_count = countKeywords(reply.auto_reply, pushoff_keywords);
    
    // 综合评分
    int score = 3; // 默认分数
    
    if (similarity > 0.6) {
        score = 5; // 与参考答案高度相似
    } else if (similarity > 0.4) {
        score = 4; // 基本相似
    } else if (similarity > 0.2) {
        score = 3; // 部分相似
    } else {
        score = 2; // 相似度低
    }
    
    // 根据主动性和推诿性调整
    if (proactive_count > 0) score += 1;
    if (pushoff_count > 0) score = std::max(1, score - 2);
    
    // 限制分数范围
    return std::min(5, std::max(0, score));
}

/**
 * @brief Helpfulness 评分逻辑
 */
static int scoreHelpfulness(const ReplyCase& reply, const ReferenceCase& reference) {
    std::string reply_lower = toLower(reply.auto_reply);
    int score = 3; // 默认分数
    
    // 检查是否包含具体步骤
    std::vector<std::string> step_keywords = {"1.", "2.", "3.", "步骤", "操作", "方法", "如下"};
    int step_count = countKeywords(reply.auto_reply, step_keywords);
    
    if (step_count > 0) {
        score = 4; // 提供了步骤
    }
    
    // 检查是否主动服务
    std::vector<std::string> service_keywords = {
        "帮您处理", "帮您查", "为您处理", "为您", "我来帮", "直接处理", "立即处理"
    };
    int service_count = countKeywords(reply.auto_reply, service_keywords);
    
    if (service_count > 0) {
        score = 5; // 直接帮用户处理
    }
    
    // 检查是否让用户自己做（不好的信号）
    std::vector<std::string> diy_keywords = {
        "自己查", "自己去", "你自己", "请自己", "自己看", "自己确认", "你可以",
        "您可以自己", "自行", "请您", "请问您", "还需要"
    };
    int diy_count = countKeywords(reply.auto_reply, diy_keywords);
    
    if (diy_count > 1) {
        score = std::max(1, score - 2);
    }
    
    // 回复长度也是帮助度的指示
    if (reply.auto_reply.length() < 30) {
        score = std::max(score - 1, 1);
    }
    
    return std::min(5, std::max(0, score));
}

/**
 * @brief Tone 评分逻辑
 */
static int scoreTone(const ReplyCase& reply, const ReferenceCase& reference) {
    std::string reply_lower = toLower(reply.auto_reply);
    int score = 3; // 默认分数
    
    // 礼貌程度检查
    std::vector<std::string> polite_keywords = {
        "您好", "感谢", "谢谢", "抱歉", "不好意思", "麻烦", "请", "谢"
    };
    int polite_count = countKeywords(reply.auto_reply, polite_keywords);
    
    if (polite_count >= 2) {
        score = 5; // 非常礼貌
    } else if (polite_count >= 1) {
        score = 4; // 基本礼貌
    }
    
    // 检查是否包含同情心（特别是在投诉或问题场景）
    if (reply.user_question.find("抱怨") != std::string::npos ||
        reply.user_question.find("不满") != std::string::npos ||
        reply.user_question.find("生气") != std::string::npos) {
        
        std::vector<std::string> empathy_keywords = {"抱歉", "不便", "理解", "感到", "同情"};
        int empathy_count = countKeywords(reply.auto_reply, empathy_keywords);
        
        if (empathy_count == 0 && score > 2) {
            score -= 1; // 在投诉场景下缺少同情心
        }
    }
    
    // 检查是否有不当表述
    std::vector<std::string> rude_keywords = {
        "自己去", "你", "废话", "无法", "很难", "不可能", "无能"
    };
    int rude_count = countKeywords(reply.auto_reply, rude_keywords);
    
    if (rude_count > 0) {
        score = std::max(1, score - 2);
    }
    
    return std::min(5, std::max(0, score));
}

/**
 * @brief Factual Reliability 评分逻辑
 */
static int scoreFactualReliability(const ReplyCase& reply, const ReferenceCase& reference) {
    int score = 5; // 默认无幻觉
    
    // 检查是否编造了具体数字
    std::vector<std::string> numbers_in_reply;
    for (char c : reply.auto_reply) {
        if (std::isdigit(c)) {
            numbers_in_reply.push_back(std::string(1, c));
        }
    }
    
    // 如果回复中包含具体数字但参考资料中没有相似数字，可能是幻觉
    int has_specific_numbers = 0;
    if (reply.auto_reply.find("工作日") != std::string::npos ||
        reply.auto_reply.find("天") != std::string::npos ||
        reply.auto_reply.find("元") != std::string::npos ||
        reply.auto_reply.find("小时") != std::string::npos) {
        has_specific_numbers++;
    }
    
    // 检查参考资料中是否也提到了这些信息
    if (has_specific_numbers > 0) {
        double similarity = calculateSimilarity(reply.auto_reply, reference.human_reference);
        if (similarity < 0.3) {
            score = 2; // 很可能编造了具体信息
        } else if (similarity < 0.5) {
            score = 3; // 可能有编造
        }
    }
    
    // 检查是否与标注备注中的"推诿"或"编造"相关信息冲突
    std::string notes_lower = toLower(reference.annotator_notes);
    if (notes_lower.find("编造") != std::string::npos ||
        notes_lower.find("幻觉") != std::string::npos ||
        notes_lower.find("未经") != std::string::npos) {
        score = std::max(0, score - 2);
    }
    
    return std::min(5, std::max(0, score));
}

/**
 * @brief 评估实现（有参考答案）
 */
EvaluationScore MockJudge::evaluate(
    const ReplyCase& reply,
    const ReferenceCase& reference) {

    EvaluationScore score;

    // 调用各维度的评分函数
    score.accuracy = scoreAccuracy(reply, reference);
    score.helpfulness = scoreHelpfulness(reply, reference);
    score.tone = scoreTone(reply, reference);
    score.factual_reliability = scoreFactualReliability(reply, reference);

    // 生成理由
    std::ostringstream reason;
    reason << "MockJudge评估: "
           << "准确性=" << score.accuracy << " "
           << "有用性=" << score.helpfulness << " "
           << "语气=" << score.tone << " "
           << "事实可靠性=" << score.factual_reliability;
    score.reason = reason.str();

    return score;
}

/**
 * @brief 无参考答案评估实现
 * 基于回复本身和用户问题进行评估
 */
EvaluationScore MockJudge::evaluateStandalone(
    const ReplyCase& reply) {

    EvaluationScore score;

    // Accuracy：基于问题-回复相关性
    int accuracy_score = 3;  // 默认分数

    std::string question_lower = toLower(reply.user_question);
    std::string reply_lower = toLower(reply.auto_reply);

    // 检查回复是否回答了问题中的关键词
    std::vector<std::string> question_keywords;
    std::istringstream q_iss(question_lower);
    std::string q_word;
    while (q_iss >> q_word) {
        // 跳过常见词
        if (q_word.length() <= 1) continue;
        if (q_word == "的" || q_word == "了" || q_word == "是" ||
            q_word == "我" || q_word == "你" || q_word == "吗" ||
            q_word == "怎么" || q_word == "什么" || q_word == "如何") continue;
        question_keywords.push_back(q_word);
    }

    // 计算回复中包含问题关键词的比例
    int matched_keywords = 0;
    for (const auto& keyword : question_keywords) {
        if (reply_lower.find(keyword) != std::string::npos) {
            matched_keywords++;
        }
    }

    if (!question_keywords.empty()) {
        double match_ratio = static_cast<double>(matched_keywords) / question_keywords.size();
        if (match_ratio >= 0.5) {
            accuracy_score = 4;  // 回复与问题相关性较高
        } else if (match_ratio >= 0.3) {
            accuracy_score = 3;  // 有一定相关性
        } else {
            accuracy_score = 2;  // 相关性较低
        }
    }

    // 检查是否有明显的答非所问
    if (reply_lower.find("不清楚") != std::string::npos ||
        reply_lower.find("不知道") != std::string::npos ||
        reply_lower.find("无法回答") != std::string::npos) {
        accuracy_score = std::min(accuracy_score, 2);
    }

    score.accuracy = accuracy_score;

    // Helpfulness：使用原有逻辑（不依赖参考答案）
    score.helpfulness = scoreHelpfulness(reply, ReferenceCase{});  // 空的 reference

    // Tone：使用原有逻辑（不依赖参考答案）
    score.tone = scoreTone(reply, ReferenceCase{});  // 空的 reference

    // Factual Reliability：基于回复本身的可疑性检测
    int factual_score = 4;  // 默认较高，因为没有证据表明有问题

    // 检查是否有具体的数字承诺（在没有参考验证时降低评分）
    if (reply.auto_reply.find("工作日") != std::string::npos ||
        reply.auto_reply.find("天") != std::string::npos && reply.auto_reply.length() < 50) {
        factual_score = 3;  // 有具体时间信息但无法验证
    }

    // 检查是否有过度承诺
    std::vector<std::string> overpromise_keywords = {"保证", "一定", "绝对", "百分之百", "完全"};
    int overpromise_count = countKeywords(reply.auto_reply, overpromise_keywords);
    if (overpromise_count > 0) {
        factual_score = std::max(2, factual_score - 1);
    }

    score.factual_reliability = factual_score;

    // 生成理由
    std::ostringstream reason;
    reason << "MockJudge评估(无参考): "
           << "准确性=" << score.accuracy << " "
           << "有用性=" << score.helpfulness << " "
           << "语气=" << score.tone << " "
           << "事实可靠性=" << score.factual_reliability << " "
           << "(⚠️ 无参考答案验证)";
    score.reason = reason.str();

    return score;
}

/**
 * @brief 获取评估器名称
 */
std::string MockJudge::getName() const {
    return "MockJudge";
}
