# DDC/CI Screen Controller

[![GitHub stars](https://img.shields.io/github/stars/xiaobai-Lethe/DDC-CI-control?style=social)](https://github.com/xiaobai-Lethe/DDC-CI-control/stargazers)
[![GitHub license](https://img.shields.io/github/license/xiaobai-Lethe/DDC-CI-control?color=brightgreen)](https://github.com/xiaobai-Lethe/DDC-CI-control/blob/main/LICENSE)
[![GitHub issues](https://img.shields.io/github/issues/xiaobai-Lethe/DDC-CI-control?color=blue)](https://github.com/xiaobai-Lethe/DDC-CI-control/issues)

A desktop application to control monitor brightness and settings via the DDC/CI protocol. 

[中文文档](README_ZH.md) | English

## Features

- Controls screen brightness via DDC/CI protocol
- Supports shortcut key control
- Auto-start capability 
- Multi-monitor support
- User-friendly interface


## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=xiaobai-Lethe/DDC-CI-control&type=Date)](https://star-history.com/#xiaobai-Lethe/DDC-CI-control&Date)

## Architecture

```mermaid
graph TD
    A[User Interface] --> B[DDC/CI Manager]
    B --> C[Monitor 1]
    B --> D[Monitor 2]
    B --> E[Monitor n...]
    F[Shortcut Manager] --> B
    G[Auto-start Manager] --> A
```

## Brightness Response Curve

```mermaid
graph LR
    A[Input] --> B[Processing]
    B --> C[Output]
    B --> D[Adjustment Curve]
    D --> B
```

## Development

This application is developed using:
- C++ for core functionality
- Qt framework for the user interface
- Modern C++ standards

## Getting Started

[Installation and usage instructions will go here]

## License

MIT License

---

*Made for desktop computers, controls screen brightness via DDC/CI protocol, supports shortcut key control and auto-start*
