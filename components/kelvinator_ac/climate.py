import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, text_sensor

AUTO_LOAD = ["text_sensor"]

CONF_PIN = "pin"
CONF_TX_DELAY = "tx_delay"
CONF_TX_LOG = "tx_log"

kelvinator_ns = cg.esphome_ns.namespace("kelvinator_ac")
KelvinatorAC = kelvinator_ns.class_("KelvinatorAC", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.climate_schema(KelvinatorAC).extend(
    {
        cv.Required(CONF_PIN): cv.int_range(min=0, max=39),
        cv.Optional(CONF_TX_DELAY, default="0ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_TX_LOG): text_sensor.text_sensor_schema(),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await climate.new_climate(config, config[CONF_PIN])
    await cg.register_component(var, config)
    cg.add(var.set_tx_delay(config[CONF_TX_DELAY]))
    if CONF_TX_LOG in config:
        ts = await text_sensor.new_text_sensor(config[CONF_TX_LOG])
        cg.add(var.set_tx_log(ts))
    cg.add_library(
        "IRremoteESP8266",
        None,
        "https://github.com/crankyoldgit/IRremoteESP8266.git",
    )
