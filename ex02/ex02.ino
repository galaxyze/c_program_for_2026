// 定义 ESP32 默认的内置LED引脚
const int ledPin = 2; 

// 记录上一次状态切换的时间戳
unsigned long previousMillis = 0; 

// 当前状态机所处的步骤索引
int stageIndex = 0; 

// 精确映射您提供示例的 SOS 完整亮灭时间序列（单位：毫秒）
// 偶数索引（0, 2, 4...）为 LED 开启时间；奇数索引（1, 3, 5...）为 LED 关闭时间
const int sosDurationTable[] = {
  200, 200, 200, 200, 200, 700,  // 阶段一：S (3次短闪，最后一次熄灭包含500ms字母间隔)
  600, 200, 600, 200, 600, 700,  // 阶段二：O (3次长闪，最后一次熄灭包含500ms字母间隔)
  200, 200, 200, 200, 200, 2200  // 阶段三：S (3次短闪，最后一次熄灭包含2000ms单词间隔)
};

// 自动计算状态序列的总步数
const int maxStages = sizeof(sosDurationTable) / sizeof(sosDurationTable[0]);

void setup() {
    // 初始化引脚2为输出模式
    pinMode(ledPin, OUTPUT); 
}

void loop() {
    // 获取当前的系统运行毫秒数
    unsigned long currentMillis = millis(); 
    
    // 获取当前步骤应该持续的时间长度
    int targetInterval = sosDurationTable[stageIndex]; 

    // 检查是否达到当前步骤的切换时间点
    if (currentMillis - previousMillis >= targetInterval) {
        previousMillis = currentMillis; // 更新时间戳
        stageIndex++;                  // 步进到下一个状态
        
        // 整个 SOS 周期播放完毕后归零，重头循环
        if (stageIndex >= maxStages) {
            stageIndex = 0; 
        }
    }

    // 状态机硬件驱动逻辑：偶数步输出高电平，奇数步输出低电平
    if (stageIndex % 2 == 0) {
        digitalWrite(ledPin, HIGH);
    } else {
        digitalWrite(ledPin, LOW);
    }
}