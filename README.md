# 银发守护——基于 openvela + ai_agent 的端侧多模态 AI 老人关怀系统

## 一、作品简介

银发守护是一套运行在 ESP32-S3 EYE 开发板上的端侧多模态 AI 老人关怀系统。系统利用加速度计实时监测老人运动状态，当检测到疑似跌倒时自动调用摄像头拍照并通过 Vision LLM 进行视觉确认，确认后通过语音播报提醒老人、同时通过飞书通知紧急联系人。此外，系统还提供语音呼救识别功能，当老人喊出"救命""帮帮我"等关键词时，自动拍照留证并发送紧急求助通知，为独居老人提供双重安全保障。

## 二、选题方向

**AI 硬件产品创新**

我国已进入深度老龄化社会，独居老人的安全监护是迫切的社会需求。本作品基于 openvela RTOS + ai_agent 框架，在低功耗嵌入式设备上实现了"传感器检测 → 视觉 AI 确认 → 多通道告警"的完整闭环，无需云端依赖，兼顾实时性与隐私保护。

## 三、目录结构

```
contest2026_069_xinghesixudui/
├── README.md                          — 本文件，作品说明
├── app/                               — 应用代码
├── board/                             — 板级适配（ESP32-S3 EYE）
├── quickapp/                          — 快应用（备用）
├── logs/                              — AI Coding 日志
└── packages/ai_agent/agent_skills/    — AI Agent Skill 定义
    ├── fall-detect.md                 — 跌倒检测 Skill：加速度计冲击 → 摄像头拍照 → Vision LLM 判断 → 震动+语音+飞书
    ├── fall-patrol.md                 — 定时巡检 Skill：每 5 分钟读取加速度计，异常触发跌倒检测
    └── voice-help.md                  — 语音呼救 Skill：识别救命关键词 → 拍照留证 → 震动+语音+飞书
```

## 四、运行方式

### 环境准备
1. 拉取 openvela 完整工程：
   ```bash
   repo init -u https://github.com/open-vela/contest2026_069_xinghesixudui \
     -b dev-ai-contest-2026 -m contest2026_069_xinghesixudui.xml
   repo sync -c -j8
   ```

2. 编译固件：
   ```bash
   cd ..
   ./build.sh vendor/openvela/boards/esp32s3-eye/configs/ai_agent -j8
   ```

3. 烧录到 ESP32-S3 EYE 开发板（通过 USB）

### 运行
设备上电后 AI Agent 自动启动，执行以下守护流程：
- **fall-patrol**：每 5 分钟自动巡检加速度计数据，异常时触发跌倒检测
- **voice-help**：持续监听语音输入，识别呼救关键词立即触发紧急求助
- **fall-detect**：被巡检或手动触发时，拍照 → Vision LLM 判断 → 震动 + 语音播报 + 飞书通知

### 配置飞书通知
在设备 NSH 终端执行：
```
vela> ask 配置飞书紧急联系人
```

## 五、技术栈

| 层级 | 技术 | 说明 |
|------|------|------|
| OS | openvela RTOS | 基于 NuttX 的实时操作系统 |
| AI 框架 | ai_agent | 内置工具调用 + Skill 驱动的 AI Agent |
| 传感器 | accelerometer | 加速度计冲击/跌倒检测 |
| 视觉 | camera_capture + Vision LLM | 摄像头拍照 + 大模型图像分析确认 |
| 语音 | ASR 语音识别 | 呼救关键词检测 |
| 播报 | music_play (Google TTS) | 语音播报提示 |
| 通知 | feishu_send_mention | 飞书紧急联系人通知 |
| 定时 | cron_add | 定时巡检任务调度 |

## 六、AI Coding 使用说明

本作品全程借助 AI 辅助开发：
- **Skill 设计**：与 AI 对话定义 fall-detect / fall-patrol / voice-help 三个 Skill 的完整流程
- **配置调优**：AI 辅助检查 defconfig 配置，确保 camera / video / feishu 等模块正确开启
- **文档生成**：README 及 Skill 文件均由 AI 协作完成

完整对话日志见 `logs/` 目录。
