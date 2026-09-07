# VoiceEdge：i.MX6ULL 流式语音控制终端

面向正点原子 ALPHA（NXP i.MX6ULL / Cortex-A7）的 Qt 5 嵌入式 Linux 项目骨架。工程通过讯飞语音听写 WebAPI 将麦克风 PCM 音频实时转成文字，再由本地指令解析器控制板载 LED；同时提供无需账号和硬件的演示模式，适合作为作品集继续迭代。

> 当前阶段：主要模块与文档已完成，尚未在开发板上交叉编译、联调或测量识别率。`94%` 是待实验验证的目标，不是当前结果。

## 已实现模块

- Qt Widgets 800×480 触控界面：识别状态、实时文本、音量、LED 状态和诊断日志。
- 音频采集：`QAudioInput` 获取 16 kHz / 16 bit / mono / little-endian PCM，格式不匹配时明确失败。
- 接口鉴权：RFC1123 UTC 时间、HMAC-SHA256、Base64 与 URL 查询参数构建。
- WebSocket 流式上传：40 ms/1280 byte 音频块，正确发送 `status=0/1/2` 首帧、中间帧和结束帧。
- 结果解析：解析 `ws/cw`，支持 `dwa=wpgs` 的 `apd/rpl/rg` 动态修正。
- 用户词表：随项目维护场景词，供讯飞控制台发布和本地测试语料使用。
- 指令与 BSP：识别 LED 开/关/切换意图，经 `ILedDevice` 抽象写入 `sys-led` 的 sysfs 节点。
- 演示模式：默认不联网、不需要密钥，模拟识别“打开开发板指示灯”和 LED 动作。
- 自动测试骨架：鉴权 URL、动态修正合并、命令解析。

讯飞官方文档说明流式接口使用 `wss://iat-api.xfyun.cn/v2/iat`、单次音频不超过 60 秒，并推荐 1280 byte 分帧；热词需在控制台上传并发布后才会影响云端识别。详见[语音听写（流式版）WebAPI](https://www.xfyun.cn/doc/asr/voicedictation/API.html)与[语音听写服务说明](https://www.xfyun.cn/doc/asr/voicedictation/voicedictation-description.html)。

## 项目结构

```text
imx6ull-voice-assistant/
├─ VoiceAssistant.pro          # Qt Creator/qmake 入口
├─ config/
│  ├─ voice_assistant.ini.example
│  └─ userwords.txt
├─ src/
│  ├─ app/                     # 模块编排与会话生命周期
│  ├─ audio/                   # PCM 麦克风采集
│  ├─ bsp/                     # LED 设备抽象和 sysfs 实现
│  ├─ cloud/                   # 讯飞鉴权、WebSocket、结果解析
│  ├─ config/                  # 外部配置与环境变量
│  ├─ domain/                  # 语音指令解析
│  └─ ui/                      # 800×480 Qt Widgets 界面
├─ tests/                      # Qt Test 单元测试
├─ scripts/                    # 板端启动与 SSH 部署脚本
└─ docs/                       # 架构、移植、测试和简历表述
```

更详细的数据流和状态机见 [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)。

## 快速体验（PC 演示模式）

依赖 Qt 5.12+，需要 `Core Gui Widgets Network Multimedia WebSockets` 模块。Qt Creator 打开 `VoiceAssistant.pro` 后构建运行。默认资源配置为：

```ini
[runtime]
mock_mode=true

[hardware]
simulate=true
```

点击“开始识别”会分段显示模拟文本并触发虚拟 LED，因此可以先检查界面、状态机和命令链路。演示模式不打开麦克风，也不连接外网。

## 接入真实讯飞服务

1. 在可执行文件旁创建 `config/voice_assistant.ini`，可直接复制 `config/voice_assistant.ini.example`。
2. 设置 `runtime/mock_mode=false`。
3. 不建议把密钥写入 Git；运行前设置环境变量：

```bash
export XFYUN_APP_ID='your-app-id'
export XFYUN_API_KEY='your-api-key'
export XFYUN_API_SECRET='your-api-secret'
./imx6ull-voice-assistant --config ./config/voice_assistant.ini
```

4. 确保系统 UTC 时间准确。讯飞鉴权会检查约 300 秒的时钟偏差。
5. 根文件系统应有 CA 证书。代码不会使用 `ignoreSslErrors()` 绕过 TLS 校验。

应用查找配置的顺序是 `--config` 指定路径、可执行文件旁的 `config/voice_assistant.ini`、当前目录对应文件、内置演示配置。环境变量覆盖 INI 中的三项密钥。

## 用户词表

`config/userwords.txt` 包含“恩智浦”“阿尔法开发板”“板载指示灯”等词。对流式 WebAPI，单纯在本地读取这个文件不会训练云端模型；需要在讯飞控制台进入对应应用的“个性化热词”，上传并发布，等待平台生效。

本地程序加载同一文件用于：

- 启动时检查场景词配置；
- 保持云端热词与项目测试语料同源；
- 为后续配置驱动的别名/模糊匹配预留入口。

准确率优化必须做发布前后对照实验，方案见 [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md)。

## LED 板级适配

正点原子 Qt 例程中的 ALPHA 板节点为：

```text
/sys/devices/platform/leds/leds/sys-led/brightness
```

工程也会尝试标准链接 `/sys/class/leds/sys-led/brightness`。真实板端把配置改为：

```ini
[hardware]
simulate=false
led_brightness=/sys/devices/platform/leds/leds/sys-led/brightness
led_trigger=/sys/class/leds/sys-led/trigger
active_high=true
```

写亮度前程序会把 kernel trigger 设置成 `none`。若你的设备树 LED 名称不同，只改 INI，不需要改业务代码。产品化时应使用专用用户组/udev 规则授权，避免让整个 GUI 长期以 root 运行。

## i.MX6ULL 交叉编译

桌面板卡资料已包含：

```text
fsl-imx-x11-glibc-x86_64-meta-toolchain-qt5-cortexa7hf-neon-toolchain-4.1.15-2.1.0_20241230.sh
```

在 Ubuntu 安装 SDK 并加载实际生成的环境脚本后：

```bash
source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi
mkdir build-imx6ull && cd build-imx6ull
qmake ../VoiceAssistant.pro
make -j"$(nproc)"
```

配套 SDK 是否包含 Qt WebSockets 仍需在 Ubuntu 安装后确认；如果缺失，应按相同 Qt 5.12.9 和 sysroot 交叉编译该模块。详细 Kit、板端库和运行环境检查见 [`docs/PORTING.md`](docs/PORTING.md)。

> 桌面 `Linux_iat1227_cdba3e53` 中的 `libmsc.so` 只有 x86/x64，不能链接进 ARM 可执行文件。本工程因此采用架构无关的 WebAPI，并未复制闭源 SDK 库。

## 构建测试

在带 Qt Test 的主机环境中：

```bash
mkdir build-tests && cd build-tests
qmake ../tests/tests.pro
make -j"$(nproc)"
./voice-assistant-core-tests
```

本次已使用 Windows Qt 5.14.2/MinGW 进行完整桌面构建，并运行核心测试：8 passed、0 failed；演示模式通过无界面启动冒烟测试。目标板配套版本为 Qt 5.12.9，仍需在 Ubuntu 交叉 SDK 中复编译确认。板端联调清单包括：麦克风格式、TLS/系统时间、WebSocket 错误码、60 秒自动收尾、动态修正、LED 权限和断网恢复。

## 下一阶段

- 在 Ubuntu 安装厂商 SDK，补齐 Qt WebSockets 后完成 ARM 交叉构建。
- 用 USB 声卡在板端验证 ALSA/Qt Multimedia 的准确格式支持。
- 上传并发布热词，按固定数据集比较关键词准确率、指令成功率、CER 和 P95 延迟。
- 增加蜂鸣器/继电器 GPIO、离线唤醒词、日志落盘与网络重连策略。

求职项目的可据实描述与实测后模板见 [`docs/RESUME.md`](docs/RESUME.md)。
