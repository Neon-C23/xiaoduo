#pragma once

#include "judge.h"

namespace replyx {

/**
 * @brief 规则型离线评估器
 * 使用预定义的规则对回复进行评分
 */
class MockJudge : public IJudge {
public:
    MockJudge();
    virtual ~MockJudge() = default;

    /**
     * @brief 使用规则对回复进行评估（有参考答案）
     */
    EvaluationScore evaluate(
        const ReplyCase& reply,
        const ReferenceCase& reference
    ) override;

    /**
     * @brief 使用规则对回复进行评估（无参考答案）
     */
    EvaluationScore evaluateStandalone(
        const ReplyCase& reply
    ) override;

    /**
     * @brief 获取评估器名称
     */
    std::string getName() const override;

    /**
     * @brief 是否支持无参考答案评估
     */
    bool supportsStandalone() const override { return true; }

private:
    // TODO: 规则库相关成员变量
};

}  // namespace replyx
