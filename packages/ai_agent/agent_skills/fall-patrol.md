# fall-patrol Skill

## 概述

跌倒巡检 Skill，在检测到跌倒事件后自动调用摄像头拍照、通过 Vision LLM 分析现场情况，并将结果推送到飞书。

## 触发条件

- 收到 `fall_detected` 事件（来自 fall-detect）
- 定时巡检任务（CONFIG_AI_AGENT_CRON）

## 数据流

1. 接收跌倒事件
2. 调用摄像头驱动（V4L2）拍摄现场照片
3. 将图片发送至 Vision LLM 进行分析
4. 综合传感器数据 + LLM 分析结果生成报告
5. 通过飞书 Webhook 发送通知（含图片和分析结果）

## 配置参数

| 参数            | 默认值                  | 说明               |
| --------------- | ----------------------- | ------------------ |
| CAMERA_DEV      | /dev/video0             | 摄像头设备路径     |
| LLM_ENDPOINT    | (配置文件)              | Vision LLM API     |
| FEISHU_WEBHOOK  | (配置文件)              | 飞书机器人 Webhook |
| PATROL_INTERVAL | 300s                    | 定时间隔           |

## 输出事件

- `photo_captured`：拍照完成
- `llm_analysis_done`：LLM 分析完成
- `feishu_notification_sent`：飞书通知已发送
