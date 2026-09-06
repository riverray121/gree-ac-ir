import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor

CONF_PIN = "pin"

kelvinator_rx_ns = cg.esphome_ns.namespace("kelvinator_rx")
KelvinatorRx = kelvinator_rx_ns.class_("KelvinatorRx", text_sensor.TextSensor, cg.Component)

CONFIG_SCHEMA = text_sensor.text_sensor_schema(KelvinatorRx).extend(
    {
        cv.Required(CONF_PIN): cv.int_range(min=0, max=39),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config, config[CONF_PIN])
    await cg.register_component(var, config)
    cg.add_library(
        "IRremoteESP8266",
        None,
        "https://github.com/crankyoldgit/IRremoteESP8266.git",
    )
