# 项目介绍
本项目基于图形渲染API Vulkan,涉及模型加载到显卡并实时渲染的demo。
## 效果展示

# 环境依赖
- 设备支持 Vulkan 驱动
- 仅支持 Windows 系统

# 目录结构描述
1. ├── ReadMe.md // 帮助文档
2. ├── bin //二进制文件目录，包括动态链接库，和生成的可执行文件(.exe)
3. ├── lib // 静态库目录
4. ├── include // 头文件目录
5. ├── kekeYo.cpp //主函数
6. ├── keke.sln //VS C++ 项目文件


# 使用说明
1. 克隆项目到本地：
git clone https://github.com/Yoyuzu/Yuzu.git
2. VS C++ 打开文件 keke.sln
3. 打开文件 ./include/shader/VertexFrom.hpp 修改36 - 41 行的路径(如果路径不一致)
