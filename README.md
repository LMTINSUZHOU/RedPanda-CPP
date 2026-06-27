# RedPanda C++

RedPanda C++（中文名“小熊猫 C++”，旧名 Red Panda Dev-C++ 7）是一款快速、轻量、开源、跨平台的 C/C++/GNU 汇编 IDE。项目以 Qt 和 C++ 开发，面向日常 C/C++ 学习、工程开发、调试，以及算法竞赛场景。

简体中文网站：[http://royqh.net/redpandacpp](http://royqh.net/redpandacpp)

英文网站：[https://sourceforge.net/projects/redpanda-cpp](https://sourceforge.net/projects/redpanda-cpp)

[为这个项目捐款](https://ko-fi.com/royqh1979)

## 项目介绍

RedPanda C++ 延续了 Dev-C++ 简洁易用的体验，同时重写和扩展了大量现代 IDE 能力。它内置代码编辑器、项目管理、编译运行、GDB 调试、代码补全、符号跳转、格式化、主题配色、试题集管理等功能，适合希望使用轻量工具完成 C/C++ 开发的用户。

这个仓库不仅包含主程序源码，还包含编辑器组件、Lua 附加组件运行环境、平台集成文件、打包脚本和辅助工具。当前代码采用 C++17/C11，主要依赖 Qt 5.15 或 Qt 6.8+，可通过 CMake 或 Xmake 构建，并支持 Windows、Linux 和 macOS。

## 主要功能

### 开发与编辑

- C/C++ 与 GNU 汇编编辑、编译、运行和调试。
- 支持 GNU Assembler、NASM，以及 SDCC 编译器。
- 增强的自动缩进、代码补全、代码折叠和符号解析。
- 支持 UTF-8 标识符、C++14 类型别名、C 风格枚举变量定义、带参数宏、lambda 表达式等语法场景。
- 查找符号引用、跳转定义/声明、TODO 视图、书签管理。
- 内置代码格式化、代码片段、文件模板和多套编辑器配色方案。

### 调试与运行

- 使用 GDB/MI 接口进行调试。
- 支持断点、监视表达式、调用栈、CPU/反汇编视图和内存查看。
- 支持 gdbserver 调试模式。
- 提供 `consolepauser` 辅助控制台程序，改善运行控制台程序时的交互体验。

### 算法竞赛与试题集

- 支持创建、管理和保存试题集。
- 可按预设输入运行程序，并与期望输出进行比较。
- 支持 Competitive Companion，可从在线评测网站抓取题目数据。

### 界面与扩展

- 完整支持高 DPI，包括字体和图标。
- 支持深色主题、主题切换和多语言翻译。
- 支持 Lua 附加组件，当前用于主题和编译器提示等扩展场景。
- 提供 Git askpass 辅助工具，并可选开启 Git 集成。

## 相比 Red Panda Dev-C++ 6 的改进

- 跨平台支持：Windows、Linux、macOS。
- 新增试题集和 Competitive Companion 支持。
- 新增 GNU 汇编、NASM、SDCC 等工具链相关能力。
- 改进查找/替换、书签、TODO、符号引用和内存查看等 IDE 功能。
- 更好的高 DPI、深色主题和编辑器配色支持。
- 更强的自动缩进、代码补全、代码折叠和 C/C++ 解析能力。
- 基于 GDB/MI 的调试流程，并增强监视、gdbserver 等调试功能。

更多版本变化请参阅 [NEWS.md](NEWS.md)。

## 源码结构

- `RedPandaIDE/`：主 IDE 程序，包含编辑器、主窗口、编译运行、调试器、项目管理、设置界面、试题集和 C/C++ 解析器等模块。
- `libs/`：项目内部库，包括 `qsynedit` 编辑器组件、Qt 工具库和 Lua。
- `tools/`：辅助工具，包括 `consolepauser`、Linux/macOS Git askpass、Windows Git askpass。
- `addon/`：Lua/Teal 附加组件示例与类型定义。
- `docs/`：开发文档，目前包含附加组件接口说明。
- `packages/`：Windows、Linux、macOS 以及多种发行版的软件包构建脚本。
- `platform/`：桌面文件、图标、macOS bundle、Windows Qt 配置等平台集成资源。
- `tests/`：测试和示例输入文件。

## 构建

推荐先阅读 [BUILD_cn.md](BUILD_cn.md)。项目支持包管理器配方、独立应用构建脚本，以及适合开发调试的手动构建方式。

手动构建的基本依赖：

- Qt 6.8+ 或 Qt 5.15。
- CMake 3.19+ 或 Xmake。
- 支持 C++17 和 C11 的编译器。

使用 CMake 的基本命令：

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --parallel
sudo cmake --install build --strip
```

使用 Xmake：

```sh
xmake f -m release
xmake
```

不同平台和打包目标可能需要额外参数或脚本，详见 [BUILD_cn.md](BUILD_cn.md)。

## 贡献

欢迎提交问题报告、功能建议、翻译、文档和代码贡献。贡献者列表见 [Contributors.md](Contributors.md)。

提交代码前建议先确认：

- 变更范围清晰，尽量遵循现有代码风格。
- 与用户界面相关的改动已检查多语言文本和高 DPI 表现。
- 编译、运行或相关测试通过。

## 许可证

RedPanda C++ 使用 GNU General Public License v3.0 授权。详情见 [LICENSE](LICENSE)。

## 致谢

- [Lua](https://www.lua.org/) 5.4.6（[源码镜像](https://github.com/lua/lua/tree/v5.4.6)）用作附加组件运行环境。
- 感谢所有为 RedPanda C++、Dev-C++ 生态、Qt、GDB、编译器工具链和开源软件社区作出贡献的人。
