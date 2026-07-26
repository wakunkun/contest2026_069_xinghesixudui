# fall_detect_app（跌倒检测 AI Agent 应用）

映射到 openvela `packages/demos/contest2026_069_fall_detect_app`。

## 功能

初始化 AI Agent 框架并注册三个 Skill：

1. **fall-detect** — 通过加速度计实时检测跌倒事件
2. **fall-patrol** — 跌倒后自动拍照 + Vision LLM 分析 + 飞书通知
3. **voice-help** — TTS 语音播报 + 麦克风监听用户回复

## 依赖

- `CONFIG_LVX_USE_DEMO_CONTEST2026_069_FALL_DETECT_APP=y`
- `CONFIG_AI_AGENT_CAMERA=y`（摄像头捕获）
- `CONFIG_AI_AGENT_FEISHU=y`（飞书通知）
- `CONFIG_AI_AGENT_CRON=y`（定时巡检）

## 构建

在 menuconfig 中启用后，正常编译即可：

```bash
make -j$(nproc)
```
