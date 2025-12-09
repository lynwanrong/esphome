import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import CONF_ID

# 定义依赖
DEPENDENCIES = ['uart']
AUTO_LOAD = ['sensor']

# 创建命名空间
bl0942_ns = cg.esphome_ns.namespace('bl0942')
BL0942 = bl0942_ns.class_('BL0942', cg.PollingComponent, uart.UARTDevice)

# 配置模式
CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(BL0942),
    }) 
    .extend(cv.polling_component_schema('1s')) # 默认1秒轮询
    .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)