#include "ssap10.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ssap10 {

static const char *const TAG = "ssap10";

static const int PM25_FRAME_LENGTH = 32;
static const int PM25_VALUE_HIGH_BYTE = 6;   // PM2.5数值高字节位置
static const int PM25_VALUE_LOW_BYTE = 7;    // PM2.5数值低字节位置
static const int PM25_ALTERNATIVE_LENGTH = 24;    // 备用帧长度

/**
 * 构造函数
 * 调用PollingComponent构造函数，设置默认轮询间隔为15秒（15000毫秒）
 * 这意味着update()方法会每15秒被调用一次
 */
SSAP10Sensor::SSAP10Sensor() : PollingComponent(15000) {}

/**
 * 获取设置优先级
 * 返回LATE优先级，表示这个组件在大多数其他组件之后初始化
 * 这确保了UART总线等依赖项已经被正确初始化
 * @return float 优先级值
 */
float SSAP10Sensor::get_setup_priority() const { 
  return setup_priority::LATE; 
}

/**
 * 组件初始化方法
 * 在ESPHome启动时被调用一次，用于初始化组件
 * 目前只输出一条配置日志信息
 */
void SSAP10Sensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Makerfabs PM2.5 Sensor...");
  ESP_LOGCONFIG(TAG, "SSAP10 requires 30 seconds stabilization time");
  
  // 设置传感器状态标志
  this->sensor_ready_ = false;
  
  // 等待传感器启动和预热 - SSAP10需要30秒稳定时间
  this->set_timeout(30000, [this]() {
    ESP_LOGCONFIG(TAG, "SSAP10 sensor 30-second stabilization completed");
    this->sensor_ready_ = true;
    // 稳定后立即进行第一次读取
    // this->update();
  });
  
  // 在稳定期间，每10秒检查一次连接状态
  // for (int i = 10; i <= 30; i += 10) {
  //   this->set_timeout(i * 1000, [this, i]() {
  //     ESP_LOGCONFIG(TAG, "SSAP10 stabilizing... %d/30 seconds", i);
  //     // 尝试简单的连接测试，但不发布数据
  //     this->test_connection();
  //   });
  // }
}

/**
 * 配置转储方法
 * 用于在启动时输出组件的配置信息到日志
 * 这有助于调试和确认组件配置是否正确
 */
void SSAP10Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Makerfabs PM2.5 Sensor (SSAP10):");
  LOG_SENSOR("  ", "PM2.5", this);
  ESP_LOGCONFIG(TAG, "  Update Interval: %ums", this->get_update_interval());
}

void SSAP10Sensor::update() {
  // 传感器是否已经稳定
  if (!this->sensor_ready_) {
    ESP_LOGW(TAG, "Sensor not ready yet, still in 30-second stabilization period");
    return;
  }
  
  ESP_LOGD(TAG, "=== Starting SSAP10 update cycle (sensor stabilized) ===");
  
  // 检查UART可用性
  if (!this->parent_->available() && !this->available()) {
    ESP_LOGW(TAG, "UART appears to be unavailable");
  }
  
  uint8_t buf[PM25_FRAME_LENGTH];

  while(this->available() < PM25_FRAME_LENGTH) {
    ESP_LOGD(TAG, "Waiting for data... currently have %d bytes, need %d bytes", this->available(), PM25_FRAME_LENGTH);
    delay(100);  // 等待更多数据到达
  }
  
  
  // 清空UART缓冲区并记录丢弃的字节
  int discarded_bytes = 0;
  while (this->available()) {
    this->read();
    discarded_bytes++;
    if (discarded_bytes > 100) {  // 防止无限循环
      ESP_LOGW(TAG, "Too many bytes in buffer, breaking");
      break;
    }
  }
  
  if (discarded_bytes > 0) {
    ESP_LOGD(TAG, "Discarded %d old bytes from UART buffer", discarded_bytes);
  }

  // 多次尝试读取数据
  bool read_success = false;
  for (int attempt = 1; attempt <= 3; attempt++) {
    ESP_LOGD(TAG, "Read attempt %d/3", attempt);
    
    // 尝试读取标准长度的数据帧
    if (this->read_array(buf, PM25_FRAME_LENGTH)) {
      ESP_LOGD(TAG, "Successfully read %d bytes on attempt %d", PM25_FRAME_LENGTH, attempt);
      read_success = true;
      break;
    } else {
      ESP_LOGW(TAG, "Failed to read %d bytes on attempt %d", PM25_FRAME_LENGTH, attempt);
      
      // 短暂延迟后重试
      delay(200);
      
      // 清理可能的部分数据
      while (this->available()) {
        this->read();
      }
    }
  }

  if (!read_success) {
    ESP_LOGE(TAG, "Failed to read data after 3 attempts - sensor may not be responding");
    this->publish_state(NAN);
    return;
  }

  // 输出原始数据用于调试 (前16字节)
  ESP_LOGD(TAG, "Raw data (first 16 bytes): %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
           buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

  // 验证帧头 - 支持多种可能的协议
  bool valid_header = false;
  if (buf[0] == 0x42 && buf[1] == 0x4d) {
    ESP_LOGD(TAG, "Detected PMS protocol frame header (0x42 0x4d)");
    valid_header = true;
  } else if (buf[0] == 0xAA && buf[1] == 0xC0) {
    ESP_LOGD(TAG, "Detected alternative protocol frame header (0xAA 0xC0)");
    valid_header = true;
  } else if (buf[0] == 0xFF && buf[1] == 0xFF) {
    ESP_LOGD(TAG, "Detected possible debug frame header (0xFF 0xFF)");
    valid_header = true;
  } else {
    ESP_LOGW(TAG, "Unknown or invalid frame header: 0x%02X 0x%02X", buf[0], buf[1]);
    ESP_LOGW(TAG, "This might indicate wrong protocol, baud rate, or sensor mode");
    this->publish_state(NAN);
    return;
  }

  // 计算校验和 (仅对标准PMS协议)
  if (buf[0] == 0x42 && buf[1] == 0x4d) {
    uint16_t checksum_calc = 0;
    for (int i = 0; i < PM25_FRAME_LENGTH - 2; i++) {
      checksum_calc += buf[i];
    }
    
    uint16_t checksum_recv = ((uint16_t)buf[PM25_FRAME_LENGTH - 2] << 8) | 
                             buf[PM25_FRAME_LENGTH - 1];

    ESP_LOGD(TAG, "Checksum - Calculated: 0x%04X, Received: 0x%04X", checksum_calc, checksum_recv);

    if (checksum_calc != checksum_recv) {
      ESP_LOGW(TAG, "Checksum mismatch! Data may be corrupted.");
      // 注意：某些传感器版本可能不使用校验和，所以这里只是警告而不是直接返回
    }
  }

  // 提取PM2.5数值
  int pm25_value;
  
  if (buf[0] == 0x42 && buf[1] == 0x4d) {
    // 标准PMS协议
    pm25_value = (int)buf[PM25_VALUE_HIGH_BYTE] * 256 + (int)buf[PM25_VALUE_LOW_BYTE];
    ESP_LOGD(TAG, "Using standard PMS protocol PM2.5 extraction");
  } else {
    // 其他协议，尝试不同的数据位置
    pm25_value = (int)buf[4] * 256 + (int)buf[5];  // 尝试不同位置
    ESP_LOGD(TAG, "Using alternative protocol PM2.5 extraction");
  }
  
  ESP_LOGD(TAG, "Raw PM2.5 value: %d", pm25_value);

  // 数值合理性检查
  if (pm25_value < 0 || pm25_value > 1000) {
    ESP_LOGW(TAG, "PM2.5 value %d seems unreasonable (0-1000 expected)", pm25_value);
    // 可以选择发布这个值或者丢弃
  }

  // 发布数值
  ESP_LOGI(TAG, "PM2.5 Concentration: %d μg/m³", pm25_value);
  this->publish_state(pm25_value);
  
  ESP_LOGD(TAG, "=== SSAP10 update cycle completed ===");
}

// 连接测试函数 - 在稳定期间检查连接状态
void SSAP10Sensor::test_connection() {
  ESP_LOGD(TAG, "Testing SSAP10 connection...");
  
  // 检查是否有数据可用
  if (this->available()) {
    int available_bytes = 0;
    while (this->available() && available_bytes < 50) {
      uint8_t byte = this->read();
      available_bytes++;
    }
    ESP_LOGD(TAG, "Found %d bytes in UART buffer (discarded during stabilization)", available_bytes);
  } else {
    ESP_LOGD(TAG, "No data available in UART buffer");
  }
}

}  // namespace ssap10
}  // namespace esphome