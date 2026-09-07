# 资料使用说明

本工程为独立实现，没有复制桌面 SDK 的闭源动态库、密钥或第三方示例源码。

- 正点原子资料用于确认 i.MX6ULL Qt 版本、交叉工具链和板载 LED sysfs 节点。
- 桌面 `Linux_iat1227_cdba3e53` 仅提供 x86/x64 `libmsc.so`，不进入 ARM 工程。
- WebSocket 协议字段按讯飞开放平台的语音听写（流式版）WebAPI 文档实现。
- `config/userwords.txt` 需要由用户在自己的讯飞应用控制台上传、发布；本地读取不会自动改变云端模型。

