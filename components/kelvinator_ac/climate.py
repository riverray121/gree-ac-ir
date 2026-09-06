import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, remote_transmitter, text_sensor

AUTO_LOAD = ["text_sensor", "remote_base"]

CONF_PIN = "pin"
CONF_TX_DELAY = "tx_delay"
CONF_TX_LOG = "tx_log"
CONF_TRANSMITTER_ID = "transmitter_id"
CONF_TIMING = "timing"

TIMING_KEYS = ("header_mark", "header_space", "bit_mark", "one_space", "zero_space", "gap")
TIMING_SCHEMA = cv.Schema({cv.Required(k): cv.positive_int for k in TIMING_KEYS})

kelvinator_ns = cg.esphome_ns.namespace("kelvinator_ac")
KelvinatorAC = kelvinator_ns.class_("KelvinatorAC", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.climate_schema(KelvinatorAC).extend(
    {
        cv.Required(CONF_PIN): cv.int_range(min=0, max=39),
        cv.Optional(CONF_TX_DELAY, default="0ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_TX_LOG): text_sensor.text_sensor_schema(),
        # Hardware-timed transmit path: the frame is built from the library's
        # state bytes and sent through an RMT remote_transmitter with the
        # given pulse lengths instead of the library's bit-banged send.
        cv.Inclusive(CONF_TRANSMITTER_ID, "rmt"): cv.use_id(remote_transmitter.RemoteTransmitterComponent),
        cv.Inclusive(CONF_TIMING, "rmt"): TIMING_SCHEMA,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await climate.new_climate(config, config[CONF_PIN])
    await cg.register_component(var, config)
    cg.add(var.set_tx_delay(config[CONF_TX_DELAY]))
    if CONF_TX_LOG in config:
        ts = await text_sensor.new_text_sensor(config[CONF_TX_LOG])
        cg.add(var.set_tx_log(ts))
    if CONF_TRANSMITTER_ID in config:
        tx = await cg.get_variable(config[CONF_TRANSMITTER_ID])
        t = config[CONF_TIMING]
        cg.add(var.set_rmt(tx, *[t[k] for k in TIMING_KEYS]))
    cg.add_library(
        "IRremoteESP8266",
        None,
        "https://github.com/crankyoldgit/IRremoteESP8266.git",
    )
