# i.MX6ULL / Qt Creator 移植说明

## 已从配套资料确认的基线

- 开发板：正点原子 ALPHA，NXP i.MX6ULL（Cortex-A7）。
- 配套 SDK：`fsl-imx-x11` 4.1.15-2.1.0，目标为 `cortexa7hf-neon`。
- Qt：资料包含 Qt 5.12.9 移植手册与 qmake 工程示例。
- 板载灯：例程使用 `sys-led`，亮度路径为 `/sys/devices/platform/leds/leds/sys-led/brightness`。

## 1. 安装交叉 SDK（Ubuntu 主机）

资料中的安装器：

```text
05、开发工具/01、交叉编译器/
fsl-imx-x11-glibc-x86_64-meta-toolchain-qt5-cortexa7hf-neon-toolchain-4.1.15-2.1.0_20241230.sh
```

默认安装目录由安装脚本声明为 `/opt/fsl-imx-x11/4.1.15-2.1.0`。安装后，以实际生成的 `environment-setup-*` 文件名为准：

```bash
source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi
which qmake
qmake -v
```

## 2. 确认 Qt 模块

工程依赖 `Core Gui Widgets Network Multimedia WebSockets`。执行：

```bash
qmake -query
find "$OECORE_TARGET_SYSROOT" -iname '*Qt5WebSockets*' -o -iname 'libQt5WebSockets.so*'
```

若 SDK 未带 Qt WebSockets，需要按相同 Qt 5.12.9、相同 mkspec/sysroot 交叉编译 `qtwebsockets`，同时把目标库部署到板端。不要链接桌面 x86/x64 讯飞 `libmsc.so`；桌面资料中的旧 MSC 包没有 ARM 库。

## 3. Qt Creator Kit

在 Qt Creator 的 Kits 中设置：

1. Compiler：SDK 内的 ARM C/C++ compiler wrapper。
2. Qt version：SDK 环境中的交叉 `qmake`。
3. Debugger：SDK 内的 `arm-poky-linux-gnueabi-gdb`（名称以实际文件为准）。
4. Device：Generic Linux Device，配置开发板 IP、SSH 用户和密钥。
5. Sysroot：`$OECORE_TARGET_SYSROOT` 对应目录。

用 Qt Creator 打开根目录的 `VoiceAssistant.pro`，Shadow Build 建议放在纯英文路径。

## 4. 板端运行前检查

```bash
date -u
arecord -l
find /sys/class/leds -maxdepth 2 -type f -name brightness
ldd /opt/voice-assistant/bin/imx6ull-voice-assistant
```

讯飞鉴权允许的时钟偏差只有约 5 分钟，因此 NTP/RTC 必须正确。根文件系统需要 CA 证书、Qt Multimedia 的 ALSA 后端，以及 Qt WebSockets/Network 的 TLS 依赖。

## 5. 配置与启动

复制 `config/voice_assistant.ini.example` 为板端的 `voice_assistant.ini`，先保留 `mock_mode=true` 验证 UI/命令/LED，再切换：

```ini
[runtime]
mock_mode=false

[hardware]
simulate=false
```

密钥建议通过进程环境注入：

```bash
export XFYUN_APP_ID='...'
export XFYUN_API_KEY='...'
export XFYUN_API_SECRET='...'
/opt/voice-assistant/run-on-board.sh
```

如果采用 X11，确保 `DISPLAY=:0`；无桌面系统时脚本自动使用 `linuxfb`。触摸输入、字体和屏幕旋转按现有 BSP 的 Qt 环境变量配置。

## 6. LED 权限

先用板端现有 root 环境验证节点。产品化时不要长期以 root 运行 GUI，应通过 udev/systemd 规则把该 LED 的 `brightness` 和 `trigger` 写权限授予专用组，并把应用用户加入该组。

