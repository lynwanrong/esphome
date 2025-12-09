#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace bl0942 {

class BL0942 : public PollingComponent, public uart::UARTDevice {
 public:
  // 构造函数
  BL0942() = default;

  // 设置传感器对象的方法
  void set_voltage_sensor(sensor::Sensor *voltage_sensor) { voltage_sensor_ = voltage_sensor; }
  void set_current_sensor(sensor::Sensor *current_sensor) { current_sensor_ = current_sensor; }
  void set_power_sensor(sensor::Sensor *power_sensor) { power_sensor_ = power_sensor; }
  void set_power_factor_sensor(sensor::Sensor *power_factor_sensor) { power_factor_sensor_ = power_factor_sensor; }

  // 标准组件方法
  void setup() override;
  void update() override; // PollingComponent 定时调用
  void loop() override;   // 主循环调用，用于接收数据

 protected:
  // 内部数据解析函数
  void parse_data_(uint8_t *data, int len);

  // 传感器指针
  sensor::Sensor *voltage_sensor_{nullptr};
  sensor::Sensor *current_sensor_{nullptr};
  sensor::Sensor *power_sensor_{nullptr};
  sensor::Sensor *power_factor_sensor_{nullptr};

  // 缓冲区
  std::vector<uint8_t> rx_buffer_;
};

}  // namespace bl0942
}  // namespace esphome