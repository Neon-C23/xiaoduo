#pragma once

#include "models.h"
#include <stdexcept>

namespace replyx {

/**
 * @brief 抽象评估器接口
 * 支持多种实现（MockJudge、LLMJudge）的策略模式
 */
class IJudge {
public:
    virtual ~IJudge() = default;

    /**
     * @brief 对一个 ReplyCase 进行评估（有参考答案）
     * @param reply 包含用户问题和自动回复的 Case
     * @param reference 人工参考答案和标注
     * @return 评估分数
     * @throws std::exception 如果评估过程出现异常
     */
    virtual EvaluationScore evaluate(
        const ReplyCase& reply,
        const ReferenceCase& reference
    ) = 0;

    /**
     * @brief 对一个 ReplyCase 进行评估（无参考答案）
     * @param reply 包含用户问题和自动回复的 Case
     * @return 评估分数（基于回复本身质量）
     * @throws std::exception 如果评估过程出现异常或不支持无参考评估
     */
    virtual EvaluationScore evaluateStandalone(
        const ReplyCase& reply
    ) {
        // 默认实现：不支持无参考评估
        throw std::runtime_error("Standalone evaluation not supported by this judge");
    }

    /**
     * @brief 获取评估器的名称
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 是否支持无参考答案评估
     */
    virtual bool supportsStandalone() const { return false; }
};

}  // namespace replyx
