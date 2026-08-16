# 银发守护 —— 基于 openvela + ai_agent 的端侧多模态 AI 老人关怀系统

## 一、作品简介

**银发守护**面向独居老人居家安全场景，在 ESP32-S3-EYE 开发板上构建一个「能主动监测、会自主告警」的嵌入式 AI Agent。系统通过加速度计实时感知跌倒冲击，联动摄像头做视觉确认，并通过飞书即时通知家属，形成「传感器触发 → 视觉确认 → 主动告警」的闭环。

- **主动监测**：QMA7981 加速度计以 3g 阈值 + 500ms 防抖持续监测冲击事件
- **事件驱动**：检测到疑似跌倒即向 ai_agent 消息总线发布 `fall_suspected` 事件
- **多模态确认**：摄像头拍照后由 Vision LLM 二次分析，降低误报
- **及时告警**：震动 + 语音播报 + 飞书通知家属
- **端侧优先**：核心逻辑端侧运行，原始图像不出设备（隐私友好）

## 二、赛道方向

AI 硬件产品创新（openvela + ai_agent 端侧多模态 AI 老人关怀系统）

## 三、目录结构

- `app/fall_detect_app/` —— 运行时入口：向 `/data/ai_agent/skills` 注入三个 skill（fall-detect / fall-patrol / voice-help），初始化 ai_agent 消息总线，启动加速度计监控线程（3g 阈值 + 500ms 防抖），检测到冲击即发布 `fall_suspected` 事件
- `packages/ai_agent/agent_skills/` —— 三个 Skill 定义：
  - **fall-detect**：跌倒检测（加速度计 3g 冲击 → 事件发布）
  - **fall-patrol**：视觉巡检（camera_capture → analyze_image → 飞书通知）
  - **voice-help**：语音呼救（本地语音提示 + 关键词响应）
- `defconfigs/esp32s3-eye/` —— 板级配置（PSRAM QPI、AI_AGENT、QMA7981 I2C1、V4L2 摄像头、cron、I2S 音频）
- `docs/architecture.md` —— 系统架构说明
- `docs/test-plan.md` —— 真机测试计划
- `logs/` —— AI Coding 日志

## 四、运行方式

1. 环境搭建与编译：参见 openvela 官方文档（Ubuntu 快速入门 + ESP32-S3-EYE 移植指南）
2. 烧录：`esptool.py -c esp32s3 -p <串口> write_flash 0x0 nuttx/nuttx.bin`
3. 启动后 NSH 中运行：`fall_detect_app`
4. ai_agent 对话：`ai_agent` 进入 `vela>` CLI；`set_wifi <ssid> <password>` 配网、`router_set <preset> <api_key>` 配置 LLM

## 五、AI Coding 使用说明

本作品全程使用 Claude Code 辅助开发：需求拆解、Skill 定义、驱动适配（QMA7981 I2C1 总线修正、PSRAM QPI 模式、板级 bringup 注册）、编译排障（HAL 补丁、Kconfig 依赖）等环节均与 AI 协作完成，完整对话日志见 `logs/` 目录。

## 六、已知适配说明（2026-08-16 真机验证）

- **PSRAM**：本板使用 QPI 模式 + `SPIRAM_BOOT_INIT=n`（OCT 模式启动挂起；复用 ROM 初始化，PSRAM heap 253KB 可用）
- **QMA7981**：位于 I2C1（SDA=GPIO4, SCL=GPIO5），CHIP_ID 0x90，软复位后需 10ms 等待（驱动补丁已提交上游 PR）
- **ai_agent**：38 个内置工具（camera_capture / feishu_send_mention / cron_add / music_play / vibrate 等）+ 10 个内置 skill + cron 主动任务 + WiFi 配网
