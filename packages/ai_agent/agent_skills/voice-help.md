# voice-help Skill

## 概述

语音呼救 Skill，在确认跌倒事件后通过 TTS 进行语音播报提醒，并监听用户语音反馈。

## 触发条件

- 收到 `fall_detected` 事件（来自 fall-detect）
- 收到手动呼救语音指令

## 数据流

1. 接收跌倒事件或语音指令
2. 通过 TTS 引擎生成语音提示（"检测到跌倒，是否需要帮助？"）
3. 通过 I2S 接口输出到扬声器
4. 监听麦克风回复（5 秒超时）
5. 若无回复或回复"救命"，升级为紧急通知

## 配置参数

| 参数            | 默认值     | 说明               |
| --------------- | ---------- | ------------------ |
| TTS_LANG        | zh-CN      | TTS 语言           |
| LISTEN_TIMEOUT_S | 5         | 语音监听超时       |
| VOLUME          | 80         | 播报音量（0-100）  |

## 输出事件

- `voice_alert_sent`：语音播报已发出
- `user_response_received`：收到用户语音回复
- `emergency_escalated`：升级为紧急状态
