#pragma once

#include <vector>
#include <string>
#include "models.h"

namespace replyx {

/**
 * @brief 从 JSON 文件加载自动回复数据
 * @param filepath 文件路径
 * @return 回复 Case 列表
 * @throws std::runtime_error 如果文件不存在或格式错误
 */
std::vector<ReplyCase> loadReplies(const std::string& filepath);

/**
 * @brief 从 JSON 文件加载人工参考数据
 * @param filepath 文件路径
 * @return 参考 Case 列表
 * @throws std::runtime_error 如果文件不存在或格式错误
 */
std::vector<ReferenceCase> loadReferences(const std::string& filepath);

}  // namespace replyx
