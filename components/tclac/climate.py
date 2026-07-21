from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart
from esphome.const import (
    CONF_ID,
    CONF_LEVEL,
    CONF_BEEPER,
    CONF_VISUAL,
    CONF_MAX_TEMPERATURE,
    CONF_MIN_TEMPERATURE,
    CONF_SUPPORTED_MODES,
    CONF_TEMPERATURE_STEP,
    CONF_SUPPORTED_PRESETS,
    CONF_TARGET_TEMPERATURE,
    CONF_SUPPORTED_SWING_MODES,
)

from esphome.components.climate import (
    ClimateMode,
    ClimatePreset,
    ClimateSwingMode,
    CONF_CURRENT_TEMPERATURE,
)

AUTO_LOAD = ["climate"]
CODEOWNERS = ["@chindocaine", "@I-am-nightingale", "@xaxexa", "@junkfix"]
DEPENDENCIES = ["climate", "uart"]

TCLAC_MIN_TEMPERATURE = 16.0
TCLAC_MAX_TEMPERATURE = 31.0
TCLAC_TARGET_TEMPERATURE_STEP = 1.0
TCLAC_CURRENT_TEMPERATURE_STEP = 1.0

CONF_FAN_SPEED_LEVELS = "fan_speed_levels"
CONF_DISPLAY = "show_display"
CONF_FORCE_MODE = "force_mode"

tclac_ns = cg.esphome_ns.namespace("tclac")
tclacClimate = tclac_ns.class_("tclacClimate", uart.UARTDevice, climate.Climate, cg.PollingComponent)

SUPPORTED_SWING_MODES_OPTIONS = {
    "OFF": ClimateSwingMode.CLIMATE_SWING_OFF,  # Always available
    "VERTICAL": ClimateSwingMode.CLIMATE_SWING_VERTICAL,
    "HORIZONTAL": ClimateSwingMode.CLIMATE_SWING_HORIZONTAL,
    "BOTH": ClimateSwingMode.CLIMATE_SWING_BOTH,
}

SUPPORTED_CLIMATE_MODES_OPTIONS = {
    "OFF": ClimateMode.CLIMATE_MODE_OFF,  # Always available
    "AUTO": ClimateMode.CLIMATE_MODE_HEAT_COOL,  # Always available
    "COOL": ClimateMode.CLIMATE_MODE_COOL,
    "HEAT": ClimateMode.CLIMATE_MODE_HEAT,
    "DRY": ClimateMode.CLIMATE_MODE_DRY,
    "FAN_ONLY": ClimateMode.CLIMATE_MODE_FAN_ONLY,
}

SUPPORTED_CLIMATE_PRESETS_OPTIONS = {
    "NONE": ClimatePreset.CLIMATE_PRESET_NONE, # Always available
    "ECO": ClimatePreset.CLIMATE_PRESET_ECO,
    "SLEEP": ClimatePreset.CLIMATE_PRESET_SLEEP,
    "COMFORT": ClimatePreset.CLIMATE_PRESET_COMFORT,
}

CONF_SUPPORTED_CUSTOM_PRESETS = "supported_custom_presets"

# Value must match PRESET_GENTLE_BREEZE in tclac.h
SUPPORTED_CUSTOM_PRESETS_OPTIONS = {
    "GENTLE_BREEZE": "Gentle Breeze",
}

# UI configuration check and taking default values
def validate_visual(config):
    if CONF_VISUAL in config:
        visual_config = config[CONF_VISUAL]
        if CONF_MIN_TEMPERATURE in visual_config:
            min_temp = visual_config[CONF_MIN_TEMPERATURE]
            if min_temp < TCLAC_MIN_TEMPERATURE:
                raise cv.Invalid(f"Specified UI minimum temperature of {min_temp} is lower than the allowed {TCLAC_MIN_TEMPERATURE} for the air conditioner")
        else:
            config[CONF_VISUAL][CONF_MIN_TEMPERATURE] = TCLAC_MIN_TEMPERATURE
        if CONF_MAX_TEMPERATURE in visual_config:
            max_temp = visual_config[CONF_MAX_TEMPERATURE]
            if max_temp > TCLAC_MAX_TEMPERATURE:
                raise cv.Invalid(f"Specified UI maximum temperature of {max_temp} is higher than the allowed {TCLAC_MAX_TEMPERATURE} for the air conditioner")
        else:
            config[CONF_VISUAL][CONF_MAX_TEMPERATURE] = TCLAC_MAX_TEMPERATURE
        if CONF_TEMPERATURE_STEP in visual_config:
            temp_step = config[CONF_VISUAL][CONF_TEMPERATURE_STEP][CONF_TARGET_TEMPERATURE]
            if ((int)(temp_step * 2)) / 2 != temp_step:
                raise cv.Invalid(f"Specified temperature step {temp_step} is incorrect, must be a multiple of 1")
        else:
            config[CONF_VISUAL][CONF_TEMPERATURE_STEP] = {CONF_TARGET_TEMPERATURE: TCLAC_TARGET_TEMPERATURE_STEP,CONF_CURRENT_TEMPERATURE: TCLAC_CURRENT_TEMPERATURE_STEP,}
    else:
        config[CONF_VISUAL] = {CONF_MIN_TEMPERATURE: TCLAC_MIN_TEMPERATURE,CONF_MAX_TEMPERATURE: TCLAC_MAX_TEMPERATURE,CONF_TEMPERATURE_STEP: {CONF_TARGET_TEMPERATURE: TCLAC_TARGET_TEMPERATURE_STEP,CONF_CURRENT_TEMPERATURE: TCLAC_CURRENT_TEMPERATURE_STEP,},}
    return config

# Component configuration check and taking default values
CONFIG_SCHEMA = cv.All(
    climate.climate_schema(tclacClimate)
    .extend(
        {
            cv.Optional(CONF_BEEPER, default=True): cv.boolean,
            cv.Optional(CONF_DISPLAY, default=True): cv.boolean,
            cv.Optional(CONF_FORCE_MODE, default=True): cv.boolean,
            cv.Optional(CONF_SUPPORTED_PRESETS,default=["NONE","ECO","SLEEP","COMFORT",],): cv.ensure_list(cv.enum(SUPPORTED_CLIMATE_PRESETS_OPTIONS, upper=True)),
            cv.Optional(CONF_SUPPORTED_CUSTOM_PRESETS,default=["GENTLE_BREEZE",],): cv.ensure_list(cv.enum(SUPPORTED_CUSTOM_PRESETS_OPTIONS, upper=True)),
            cv.Optional(CONF_SUPPORTED_SWING_MODES,default=["OFF","VERTICAL","HORIZONTAL","BOTH",],): cv.ensure_list(cv.enum(SUPPORTED_SWING_MODES_OPTIONS, upper=True)),
            cv.Optional(CONF_SUPPORTED_MODES,default=["OFF","AUTO","COOL","HEAT","DRY","FAN_ONLY",],): cv.ensure_list(cv.enum(SUPPORTED_CLIMATE_MODES_OPTIONS, upper=True)),
            cv.Optional(CONF_FAN_SPEED_LEVELS, default=5): cv.one_of(3, 5, int=True),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    validate_visual,
)

ForceOnAction = tclac_ns.class_("ForceOnAction", automation.Action)
ForceOffAction = tclac_ns.class_("ForceOffAction", automation.Action)
BeeperOnAction = tclac_ns.class_("BeeperOnAction", automation.Action)
BeeperOffAction = tclac_ns.class_("BeeperOffAction", automation.Action)
DisplayOnAction = tclac_ns.class_("DisplayOnAction", automation.Action)
DisplayOffAction = tclac_ns.class_("DisplayOffAction", automation.Action)

TCLAC_ACTION_BASE_SCHEMA = automation.maybe_simple_id({cv.GenerateID(CONF_ID): cv.use_id(tclacClimate),})

# Registration of air conditioner display on and off events
@automation.register_action(
    "climate.tclac.display_on", DisplayOnAction, cv.Schema, synchronous=True
)
@automation.register_action(
    "climate.tclac.display_off", DisplayOffAction, cv.Schema, synchronous=True
)
async def display_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var

# Registration of air conditioner beeper on and off events
@automation.register_action(
    "climate.tclac.beeper_on", BeeperOnAction, cv.Schema, synchronous=True
)
@automation.register_action(
    "climate.tclac.beeper_off", BeeperOffAction, cv.Schema, synchronous=True
)
async def beeper_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var

# Registration of force mode on and off events
@automation.register_action(
    "climate.tclac.force_mode_on", ForceOnAction, cv.Schema, synchronous=True
)
@automation.register_action(
    "climate.tclac.force_mode_off", ForceOffAction, cv.Schema, synchronous=True
)
async def force_mode_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var


# Adding configuration to code
def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
    yield climate.register_climate(var, config)

    if CONF_BEEPER in config:
        cg.add(var.set_beeper_state(config[CONF_BEEPER]))
    if CONF_DISPLAY in config:
        cg.add(var.set_display_state(config[CONF_DISPLAY]))
    if CONF_FORCE_MODE in config:
        cg.add(var.set_force_mode_state(config[CONF_FORCE_MODE]))
    if CONF_SUPPORTED_MODES in config:
        cg.add(var.set_supported_modes(config[CONF_SUPPORTED_MODES]))
    if CONF_SUPPORTED_PRESETS in config:
        cg.add(var.set_supported_presets(config[CONF_SUPPORTED_PRESETS]))
    if CONF_SUPPORTED_CUSTOM_PRESETS in config:
        cg.add(var.set_supported_custom_presets_option(config[CONF_SUPPORTED_CUSTOM_PRESETS]))
    if CONF_FAN_SPEED_LEVELS in config:
        cg.add(var.set_fan_speed_levels(config[CONF_FAN_SPEED_LEVELS]))
    if CONF_SUPPORTED_SWING_MODES in config:
        cg.add(var.set_supported_swing_modes(config[CONF_SUPPORTED_SWING_MODES]))