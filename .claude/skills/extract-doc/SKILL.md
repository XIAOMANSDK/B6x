---
name: extract-doc
description: |
  提取 .docx/.xlsx/.pdf/图片 文档内容，转换为 Markdown 格式（支持 OCR 识别）。
  这是**只读提取工具**，专注于将现有文档转换为 Markdown。

  TRIGGER when: 用户要求提取文档内容、读取 Word/Excel/PDF 内容、提取图片文字、
  OCR 识别、文档转 Markdown、读取需求文档、提取规格书内容、把 docx 转成 md、
  把 xlsx 转成 markdown、PDF 内容提取、扫描件识别、"帮我看看这个文档里有什么"、
  "把这个 Word 转成 Markdown"、"读取 Excel 里的数据"、"识别图片中的文字"。

  DO NOT TRIGGER when: 用户要求**创建**或**编辑**文档（用 document-skills:docx/xlsx/pdf）、
  生成新的 Word/Excel/PDF 文件、修改现有文档内容、添加表格/图表到文档、
  格式化文档排版、合并多个 PDF。此 skill 仅用于**读取和提取**，不用于创建或修改。

user-invocable: true
allowed-tools: Read, Grep, Glob, Write, Bash, mcp__zai-mcp-server__extract_text_from_screenshot
---

# Extract Doc

提取 .docx/.xlsx/.pdf/图片 文档内容，转换为 Markdown 格式。

## 触发

`/extract-doc <file_path>`

**参数**:
- `file_path`: 文档路径（支持 .docx/.xlsx/.pdf/.png/.jpg/.bmp）

---

## 支持格式

| 格式 | 扩展名 | 提取方法 | 特殊处理 |
|------|--------|----------|----------|
| Word | .docx | python-docx | 段落、表格、列表 |
| Excel | .xlsx | openpyxl | 多 Sheet、表格 |
| PDF | .pdf | pymupdf + MCP OCR | 文本型/扫描型 |
| 图片 | .png/.jpg/.bmp | MCP OCR | 中文/英文/代码 |

---

## OCR 规则

> **必须使用 MCP OCR**，禁止安装本地 OCR 库 (pytesseract 等)。

**工具**: `mcp__zai-mcp-server__extract_text_from_screenshot`

```
参数:
  image_source: 图片路径或 URL
  prompt: 提取指令 (如 "提取所有文本内容，保持原有排版")
  programming_language: 代码语言 (可选，如 "python")
```

---

## 依赖安装

> **首次使用前执行**

```bash
pip install python-docx openpyxl pymupdf
```

| 库 | 版本 | 用途 |
|-----|------|------|
| python-docx | >=0.8.11 | Word 解析 |
| openpyxl | >=3.1.0 | Excel 解析 |
| pymupdf | >=1.23.0 | PDF 解析 |

---

## 执行流程

### Step 1: 按类型提取

验证文件存在后，根据扩展名选择提取方法。

#### .docx 文件

```python
from docx import Document
doc = Document(path)

# 段落: 识别标题层级
for para in doc.paragraphs:
    if para.style.name.startswith('Heading'):
        level = para.style.name.split()[-1]
        # 输出: {'#' * int(level)} {para.text}
    else:
        # 输出: para.text

# 表格: 转 Markdown 表格
for table in doc.tables:
    # 第1行为表头, 其余为数据行
```

#### .xlsx 文件

```python
from openpyxl import load_workbook
wb = load_workbook(path)

for sheet_name in wb.sheetnames:
    sheet = wb[sheet_name]
    # 第1行检测表头, 第2行起为数据, 转为 Markdown 表格
```

#### .pdf 文件

```python
import fitz  # pymupdf
doc = fitz.open(path)

for i, page in enumerate(doc, 1):
    text = page.get_text().strip()
    if text:
        # 文本型 PDF - 直接提取
    else:
        # 扫描型 PDF - 渲染为图片 + MCP OCR
        pix = page.get_pixmap(dpi=150)
        pix.save(f"/tmp/pdf_page_{i}.png")
        # 调用 mcp__zai-mcp-server__extract_text_from_screenshot(...)
```

#### 图片文件 (MCP OCR)

```
直接调用:
mcp__zai-mcp-server__extract_text_from_screenshot(
    image_source="D:/docs/scan.png",
    prompt="提取图片中的所有文字，保持原有排版和表格结构"
)
```

### Step 2: 格式化输出

转换为 Markdown，保持原有结构。

---

## 输出格式

```markdown
# 文档名称

## 元信息
- 类型: [Word/Excel/PDF/图片]
- 页数/工作表: [N]
- OCR: [是/否]

## 内容

[提取的文本内容，保持原始结构]

### 表格 1

| 列1 | 列2 | 列3 |
|-----|-----|-----|
| ... | ... | ... |
```

---

## 错误处理

| 错误 | 处理 |
|------|------|
| 文件不存在 | 验证路径，搜索相似文件名 |
| 库未安装 | 显示 `pip install` 命令 |
| 大文件 (>10MB) | 警告并建议分段提取 |
| 编码问题 | 尝试 UTF-8/GBK 解码 |
| PDF 加密 | 提示需要密码 |
| OCR 失败 | 检查图片清晰度，建议 DPI >= 150 |
| 不支持的格式 | 列出支持的格式列表 |

---

## B6x 项目典型用例

| 文档类型 | 用例 |
|----------|------|
| 需求文档 (.docx) | 提取功能需求转 Markdown |
| 规格书 (.pdf) | 提取芯片规格、寄存器描述 |
| 测试用例 (.xlsx) | 提取测试项转为可读格式 |
| 扫描件 (.png) | OCR 识别手写笔记 |
| 代码截图 | 提取代码文本 |
