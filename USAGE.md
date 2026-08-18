# 自动回复质量评估流水线 - 使用说明

## 环境要求

### 编译器
- GCC 7.0+ 或 Clang 5.0+
- C++17 标准支持

### 必需软件
- CMake 3.16+
- make（或等效构建工具）

## 依赖库

| 库 | 版本要求 | 用途 | 安装方式 |
|---|----------|------|----------|
| nlohmann/json | 3.11+ | JSON 解析 | 项目自带（third_party/） |
| libcurl | 7.68+ | HTTP 请求（LLM 模式） | `sudo dnf install libcurl-devel` |

**Linux (Fedora/CentOS/RHEL):**
```bash
sudo dnf install cmake gcc-c++ libcurl-devel make
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt install cmake g++ libcurl4-openssl-dev make
```

**macOS:**
```bash
brew install cmake libcurl make
```

## 编译

```bash
mkdir -p build && cd build
cmake ..
make
```

可执行文件位于 `build/bin/reply_eval`

## DeepSeek API 配置

**测试用 API Key**：
```
sk-b4eceae2563042dd8ff3bd6a00293c60
```

**API 端点**：`https://api.deepseek.com`

**设置环境变量**：
```bash
export LLM_API_KEY="sk-b4eceae2563042dd8ff3bd6a00293c60"
```

## 命令行参数

```bash
./reply_eval [选项]
```

| 参数 | 说明 | 必填 | 默认值 |
|------|------|------|--------|
| `--mode MODE` | 评估模式：mock 或 llm | 否 | mock |
| `--input FILE` | 输入 JSON 文件路径 | 是* | - |
| `--reference FILE` | 参考文件路径 | 否* | - |
| `--skip-reference` | 跳过参考答案加载 | 否 | false |
| `--output FILE` | 输出报告路径 | 否 | reports/report.md |
| `--api-endpoint URL` | LLM API 端点 | 否 | https://api.deepseek.com |
| `--model MODEL` | LLM 模型名称 | 否 | deepseek-v4-flash |
| `--help` | 显示帮助信息 | 否 | - |

*注：`--input` 为必填；`--reference` 在未指定 `--skip-reference` 时必填

## 使用示例

示例程序在 Rocky Linux 10.2 操作系统中编译完成，GCC版本为 14.3.1

**Mock 模式（离线评估）：**
```bash
./reply_eval --mode mock --input ../doc/auto_replies.json --reference ../doc/human_ref.json
```

**LLM 模式（AI 评估）：**
```bash
export LLM_API_KEY="sk-b4eceae2563042dd8ff3bd6a00293c60"
./reply_eval --mode llm --input ../doc/auto_replies.json --reference ../doc/human_ref.json
```

**无参考答案模式：**
```bash
./reply_eval --mode mock --input ../doc/auto_replies.json --skip-reference
```
