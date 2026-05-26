# unitree_asr_voice

ROS1 节点：订阅宇树 G1 机器人 DDS 通道的 ASR（自动语音识别）音频消息，提取文本字段，并将结果打印到 ROS 日志。

## 目录结构

```
src/unitree_asr_voice/
├── CMakeLists.txt              # 构建配置
├── package.xml                 # 包信息
├── config/
│   └── default.yaml            # 默认参数配置
├── launch/
│   └── unitree_asr_voice.launch # 启动文件
└── src/
    └── unitree_asr_voice_node.cpp  # 主节点源代码
```

## 依赖项

- **ROS1** (roscpp, std_msgs)
- **nlohmann_json** — JSON 解析库
- **unitree_sdk2** — 宇树机器人 SDK

## 构建方法

确保已 source ROS1 环境，然后在 catkin 工作空间根目录执行：

```bash
catkin_make
```

## 配置参数

所有参数可通过 ROS 参数服务器设置（默认值如下）：

| 参数名 | 默认值 | 说明 |
|--------|--------|------|
| `network_interface` | `eth0` | 连接机器人的网络接口 |
| `dds_topic`  | `rt/audio_msg` | 宇树 DDS ASR 话题名称 |
| `queue_size` | `10` | ROS 发布队列大小 |

## 启动方式

### 使用 launch 文件（推荐）

```bash
roslaunch unitree_asr_voice unitree_asr_voice.launch
```

可覆盖默认参数：

```bash
roslaunch unitree_asr_voice unitree_asr_voice.launch network_interface:=wlan0
```

### 直接运行节点

```bash
rosrun unitree_asr_voice unitree_asr_voice_node
```

## 话题说明

### 订阅话题

- **`rt/audio_msg`**（DDS 通道）— 宇树机器人发布的 ASR 原始消息，格式为 JSON 字符串，例如：
  ```json
  {"index":133, "timestamp":1779762202365, "type":0, "text":"导航。", "angle":0, "speaker_id":0, "emotion":"<|NEUTRAL|>", "confidence":0.500000, "language":"<|zh|>", "is_final":false}
  ```


## 核心函数说明

### `NormalizeAsrText`

该函数用于**去除 ASR 文本末尾的标点符号**，确保下游处理获得干净的文本。

#### 处理流程

1. **去除首尾空白**：调用 `TrimAsciiWhitespace()` 去掉字符串首尾的 ASCII 空白字符。
2. **定义标点后缀列表**：包含中英文标点符号。
3. **循环去除末尾标点**：
   - 遍历标点列表，检查字符串是否以某个标点结尾。
   - 如果是，则从末尾删除该标点，并再次去除首尾空白。
   - 重复此过程，直到字符串末尾不再有任何标点符号。

#### 示例

| 输入 | 输出 | 说明 |
|------|------|------|
| `"导航。"` | `"导航"` | 去掉中文句号 |
| `"你好！！！"` | `"你好"` | 多次循环去掉多个感叹号 |
| `"继续导航。"` | `"继续导航"` | 去掉末尾句号 |
| `"是的，没错。"` | `"是的，没错"` | 只去掉末尾句号，中间逗号保留 |
| `"Hello!"` | `"Hello"` | 去掉英文感叹号 |
| `"测试。。。"` | `"测试"` | 多次循环去掉多个句号 |

#### 关键特点

- **只处理末尾**：仅删除字符串**末尾**的标点，不影响中间或开头的标点。
- **多次循环**：通过 `while(true)` 循环反复检查，确保去除所有末尾标点。
- **中英文兼容**：同时支持中英文标点符号（`。！？，、；：.!?,;:`）。

## 许可证

MIT
