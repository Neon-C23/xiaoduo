#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "models.h"
#include "json_loader.h"

/**
 * @brief 简单的 JSON 解析辅助函数
 */
namespace {
    /**
     * @brief 从 JSON 字符串中提取字段值
     * 支持简单的 "field": "value" 格式
     */
    std::string extractJsonField(const std::string& json_str, const std::string& field_name) {
        std::string search_key = "\"" + field_name + "\":";
        size_t pos = json_str.find(search_key);
        if (pos == std::string::npos) {
            return "";
        }
        
        pos += search_key.length();
        
        // 跳过空白和引号
        while (pos < json_str.length() && (json_str[pos] == ' ' || json_str[pos] == '\"')) {
            pos++;
        }
        
        // 找到值的结束（引号、逗号或括号）
        size_t end_pos = pos;
        int brace_count = 0;
        while (end_pos < json_str.length()) {
            if (json_str[end_pos] == '\"' && (end_pos == 0 || json_str[end_pos-1] != '\\')) {
                break;
            }
            if (json_str[end_pos] == '{') brace_count++;
            if (json_str[end_pos] == '}') brace_count--;
            if (brace_count < 0 || (brace_count == 0 && (json_str[end_pos] == ',' || json_str[end_pos] == '}'))) {
                break;
            }
            end_pos++;
        }
        
        return json_str.substr(pos, end_pos - pos);
    }
    
    /**
     * @brief 读取整个JSON文件到字符串
     */
    std::string readFileToString(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filepath);
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    /**
     * @brief 将JSON数组字符串分割成单个对象
     */
    std::vector<std::string> splitJsonObjects(const std::string& json_array_str) {
        std::vector<std::string> objects;
        int brace_count = 0;
        size_t start = 0;
        bool in_string = false;
        bool escape_next = false;
        
        for (size_t i = 0; i < json_array_str.length(); ++i) {
            char c = json_array_str[i];
            
            // 处理转义字符
            if (escape_next) {
                escape_next = false;
                continue;
            }
            
            if (c == '\\') {
                escape_next = true;
                continue;
            }
            
            // 处理字符串
            if (c == '\"') {
                in_string = !in_string;
                continue;
            }
            
            if (in_string) continue;
            
            // 处理对象边界
            if (c == '{') {
                if (brace_count == 0) start = i;
                brace_count++;
            } else if (c == '}') {
                brace_count--;
                if (brace_count == 0) {
                    objects.push_back(json_array_str.substr(start, i - start + 1));
                }
            }
        }
        
        return objects;
    }
}  // anonymous namespace

namespace replyx {

/**
 * @brief 从 JSON 文件加载自动回复数据
 */
std::vector<ReplyCase> loadReplies(const std::string& filepath) {
    std::string content = readFileToString(filepath);
    std::vector<std::string> objects = splitJsonObjects(content);
    
    std::vector<ReplyCase> replies;
    
    for (const auto& obj : objects) {
        ReplyCase reply;
        reply.id = extractJsonField(obj, "id");
        reply.user_question = extractJsonField(obj, "user_question");
        reply.auto_reply = extractJsonField(obj, "auto_reply");
        
        if (!reply.id.empty() && !reply.user_question.empty() && !reply.auto_reply.empty()) {
            replies.push_back(reply);
        }
    }
    
    std::cout << "Loaded " << replies.size() << " reply cases from " << filepath << "\n";
    return replies;
}

/**
 * @brief 从 JSON 文件加载人工参考数据
 */
std::vector<ReferenceCase> loadReferences(const std::string& filepath) {
    std::string content = readFileToString(filepath);
    std::vector<std::string> objects = splitJsonObjects(content);
    
    std::vector<ReferenceCase> references;
    
    for (const auto& obj : objects) {
        ReferenceCase ref;
        ref.id = extractJsonField(obj, "id");
        ref.human_reference = extractJsonField(obj, "human_reference");
        ref.annotator_notes = extractJsonField(obj, "annotator_notes");
        
        if (!ref.id.empty() && !ref.human_reference.empty()) {
            references.push_back(ref);
        }
    }
    
    std::cout << "Loaded " << references.size() << " reference cases from " << filepath << "\n";
    return references;
}

}  // namespace replyx
