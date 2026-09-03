import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate

CONF_PIN = "pin"
CONF_TX_DELAY = "tx_delay"

kelvinator_ns = cg.esphome_ns.namespace("kelvinator_ac")
KelvinatorAC = kelvinator_ns.class_("KelvinatorAC", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.climate_schema(KelvinatorAC).extend(
    {
        cv.Required(CONF_PIN): cv.int_range(min=0, max=39),
        cv.Optional(CONF_TX_DELAY, default="0ms"): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await climate.new_climate(config, config[CONF_PIN])
    await cg.register_component(var, config)
    cg.add(var.set_tx_delay(config[CONF_TX_DELAY]))
    cg.add_library(
        "IRremoteESP8266",
        None,
        "https://github.com/crankyoldgit/IRremoteESP8266.git",
    )
