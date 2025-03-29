# DDC/CI 屏幕控制器

[![GitHub stars](https://img.shields.io/github/stars/xiaobai-Lethe/DDC-CI-control?style=social)](https://github.com/xiaobai-Lethe/DDC-CI-control/stargazers)
[![GitHub license](https://img.shields.io/github/license/xiaobai-Lethe/DDC-CI-control?color=brightgreen)](https://github.com/xiaobai-Lethe/DDC-CI-control/blob/main/LICENSE)
[![GitHub issues](https://img.shields.io/github/issues/xiaobai-Lethe/DDC-CI-control?color=blue)](https://github.com/xiaobai-Lethe/DDC-CI-control/issues)

一款通过DDC/CI协议控制显示器亮度和设置的桌面应用程序。

中文 | [English](README.md)

## 功能特点

- 通过DDC/CI协议控制屏幕亮度
- 支持快捷键控制
- 自动启动功能
- 多显示器支持
- 用户友好的界面


## 星标历史

[![Star History Chart](https://api.star-history.com/svg?repos=xiaobai-Lethe/DDC-CI-control&type=Date)](https://star-history.com/#xiaobai-Lethe/DDC-CI-control&Date)

## 架构

```mermaid
graph TD
    A[用户界面] --> B[DDC/CI 管理器]
    B --> C[显示器 1]
    B --> D[显示器 2]
    B --> E[显示器 n...]
    F[快捷键管理器] --> B
    G[自动启动管理器] --> A
```

## 亮度响应曲线

```mermaid
graph LR
    A[输入] --> B[处理]
    B --> C[输出]
    B --> D[调整曲线]
    D --> B
```

## 开发信息

本应用程序使用以下技术开发：
- C++用于核心功能
- Qt框架用于用户界面
- 现代C++标准

## 开始使用

[这里将提供安装和使用说明]

## 许可证

MIT 许可证

---

*专为桌面电脑设计，通过DDC/CI协议控制屏幕亮度，支持快捷键控制和自动启动* 