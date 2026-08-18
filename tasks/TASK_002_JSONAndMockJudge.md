# TASK_002: JSON 数据加载与 MockJudge 规则评分实现

**任务编号**: TASK_002  
**关联计划**: [PLAN_002_JSONAndMockJudge](../plans/PLAN_002_JSONAndMockJudge.md)  
**优先级**: HIGH  
**状态**: In Progress  
**创建日期**: 2026-08-18  
**预计完成**: 2026-08-19  

---

## 实现思路

本任务分为三部分：

1. **JSON 数据加载**：使用 nlohmann/json 将 auto_replies.json 和 human_ref.json 解析到内存
2. **MockJudge 规则评分**：基于启发式规则对四个维度进行评分
3. **主流程集成**：连接数据加载 → 批量评估 → 报告生成 → 文件输出

核心思路是：

- 通过文本分析（关键词、句长、是否包含步骤等）来推断各维度分数
- 结合人工参考答案进行对比，增加准确性
- 对于难以判断的情况，采用保守评分
- 支持规则的后续灵活调整和改进

## 实现步骤

### 第一部分：JSON 加载

1. **编写 json_loader.cpp**
   - 使用 `#include <nlohmann/json.hpp>`
   - 实现 `loadReplies()` 函数
   - 实现 `loadReferences()` 函数
   - 添加字段校验和异常处理

2. **数据结构映射**
   - auto_replies.json → `std::vector<ReplyCase>`
   - human_ref.json → `std::vector<ReferenceCase>`
   - 验证所有必填字段存在

3. **测试数据加载**
   - 加载后输出数据摘要验证
   - 确保 ID 匹配率正确

### 第二部分：MockJudge 规则评分

1. **Accuracy 评分规则**（权重 35%）
   ```text
   5: 回复包含解决方案，事实准确，与参考答案一致
   4: 基本准确，仅存在表述略有不同的轻微差异
   3: 部分准确，存在明显遗漏但核心方向正确
   2: 存在明显错误或严重遗漏
   1: 答非所问或基本错误
   0: 完全错误或与事实相反
   ```

   判断逻辑：
   - 检查回复长度（太短可能不准确）
   - 检查是否包含关键词（如"帮您查", "为您处理"表示主动）
   - 检查是否直接给出具体答案
   - 对比人工参考答案的相似度（词汇重叠、句子结构）

2. **Helpfulness 评分规则**（权重 30%）
   ```text
   5: 给出了直接可执行的方案（如具体步骤、订单号查询）
   4: 给出了有用的信息但需要用户进一步操作
   3: 给出了部分信息但用户可能需要继续追问
   2: 信息有限，用户需要自己做很多工作
   1: 几乎没有帮助
   0: 完全无用
   ```

   判断逻辑：
   - 是否包含具体步骤（"1.", "2.", "3." 等）
   - 是否说"帮您处理" / "帮您查" 等主动服务
   - 是否让用户"自己查" / "自己去" 等推诿性表述
   - 是否给出了具体的数字时间或操作路径

3. **Tone 评分规则**（权重 10%）
   ```text
   5: 包含问候 + 道歉/感谢 + 自然表述
   4: 包含基本的客套
   3: 略显机械但没有不妥
   2: 明显生硬或冷漠
   1: 不礼貌或消极
   0: 冒犯用户
   ```

   判断逻辑：
   - 是否包含"您好" / "感谢" / "抱歉" 等客套语
   - 是否包含"我" / "我们" / "帮您" 等主动承诺
   - 句子结构和用词是否自然（避免过于板生硬）
   - 如果用户在发泄怨气，回复是否表达了同情

4. **Factual Reliability 评分规则**（权重 25%）
   ```text
   5: 所有信息都在参考资料中有依据，无编造
   4: 基本有依据，极轻微推断
   3: 存在少量无法支持的信息但风险较低
   2: 存在明显未经支持的信息（如编造具体数字）
   1: 大量未经支持的信息
   0: 严重事实编造
   ```

   判断逻辑：
   - 检查回复中是否出现参考资料中没有的具体数字（如时间、金额）
   - 检查是否编造了产品特性或规定
   - 检查是否与参考答案或标注备注存在明显冲突
   - 检查是否有推测性表述但加了"一般" / "可能" 等限定词

### 第三部分：主流程集成

1. **修改 main.cpp**
   - 调用 `loadReplies()` 和 `loadReferences()`
   - 创建 MockJudge 实例
   - 调用 `Evaluator::evaluateBatch()`
   - 调用 `Evaluator::summarize()`
   - 生成报告

2. **修改 report.cpp**
   - 实现完整的 `generateMarkdownReport()` 逻辑
   - 输出汇总统计、指标分布、最差 3 Case
   - 支持将报告写入文件

3. **完整的命令行运行**
   ```bash
   ./reply_eval --mode mock \
     --input ../doc/auto_replies.json \
     --reference ../doc/human_ref.json \
     --output ../reports/report.md
   ```

## 第三方库

| 库名 | 最低版本 | 用途 | 安装方式 |
|---|---|---|---|
| nlohmann/json | 3.11 | JSON 解析 | 系统包 或 单头文件 |

**如果系统找不到 nlohmann/json**：

```bash
# Option 1: 安装系统包
sudo dnf install nlohmann-json-devel

# Option 2: 使用单头文件
# 从 https://github.com/nlohmann/json/releases 下载 json.hpp
# 放到 third_party/nlohmann/ 目录
```

## 修改代码清单

### src/json_loader.cpp

```cpp
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "models.h"

using json = nlohmann::json;

std::vector<replyx::ReplyCase> loadReplies(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    
    json data = json::parse(file);
    std::vector<replyx::ReplyCase> replies;
    
    for (const auto& item : data) {
        replyx::ReplyCase reply;
        reply.id = item["id"].get<std::string>();
        reply.user_question = item["user_question"].get<std::string>();
        reply.auto_reply = item["auto_reply"].get<std::string>();
        
        replies.push_back(reply);
    }
    
    std::cout << "Loaded " << replies.size() << " reply cases\n";
    return replies;
}

std::vector<replyx::ReferenceCase> loadReferences(const std::string& filepath) {
    // 类似实现...
}
```

### src/mock_judge.cpp

```cpp
// 实现四维规则评分逻辑
// 包含 countKeywords(), analyzeLength(), checkProactiveServiceTerms() 等辅助函数
// 返回 0-5 的评分和理由字符串
```

### src/main.cpp

```cpp
// 集成主流程
try {
    auto replies = loadReplies(args.input_file);
    auto references = loadReferences(args.reference_file);
    
    auto judge = std::make_unique<MockJudge>();
    auto results = Evaluator::evaluateBatch(replies, references, judge.get());
    auto summary = Evaluator::summarize(results);
    
    auto report = ReportGenerator::generateMarkdownReport(
        results, summary, "Mock"
    );
    
    ReportGenerator::writeReportToFile(args.output_file, report);
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
}
```

## 测试方案

### 1. JSON 加载测试

```bash
# 编译后在 main.cpp 添加临时调试代码
./reply_eval --mode mock --input ../doc/auto_replies.json --reference ../doc/human_ref.json

# 预期输出：
# Loaded 20 reply cases
# Loaded 20 reference cases
# Evaluating...
```

### 2. MockJudge 评分测试

```bash
# 选中第一个 Case 手工验证评分是否合理
# case_01: 用户丢快递 → 自动回复推诿 → 应该 Helpfulness 较低
```

### 3. 完整流程测试

```bash
cd /home/chen/code/xiaoduo/build
cmake .. && make

./bin/reply_eval \
  --mode mock \
  --input ../doc/auto_replies.json \
  --reference ../doc/human_ref.json \
  --output ../reports/report.md

# 预期：
# - 程序正常退出（exit code 0）
# - 生成 reports/report.md
# - 报告包含统计摘要、所有 20 条 Case 的评分、最差 3 Case 分析
```

### 4. 报告验证

```bash
# 查看生成的报告
cat ../reports/report.md

# 验证内容：
# - 总样本数 = 20
# - PASS + WARNING + FAIL 数 = 20
# - 平均分在 0-5 之间
# - Critical Failure 数 >= 0
# - 最差 3 Case 的详细分析
```

## 验收标准

任务完成标准：

- [ ] JSON 加载正确，20 条 Case 全部加载
- [ ] MockJudge 四维评分均有规则实现
- [ ] 加权计算正确
- [ ] 报告生成正确，格式清晰
- [ ] 完整的命令行流程可运行
- [ ] 报告文件输出到指定路径
- [ ] 编译无错误（仅警告可接受）

---

**备注**：本任务完成后，项目具有完整的离线评估能力，为后续 LLM 集成奠定基础。