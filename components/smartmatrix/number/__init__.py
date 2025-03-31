import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_MODE, CONF_MIN_VALUE, CONF_MAX_VALUE, CONF_STEP, CONF_INITIAL_VALUE

from ..display import MATRIX_ID, SmartMatrixDisplay

smartmatrix_ns = cg.esphome_ns.namespace("smartmatrix")
SmartMatrixBrightness = smartmatrix_ns.class_(
    "SmartMatrixBrightness", number.Number, cg.Component
)

CONFIG_SCHEMA = number.NUMBER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SmartMatrixBrightness),
        cv.Required(MATRIX_ID): cv.use_id(SmartMatrixDisplay),
        cv.Optional(CONF_MODE, default="SLIDER"): cv.enum(
            number.NUMBER_MODES, upper=True
        ),
        cv.Optional(CONF_MIN_VALUE, default=0): cv.float_,
        cv.Optional(CONF_MAX_VALUE, default=255): cv.float_,
        cv.Optional(CONF_STEP, default=1): cv.float_,
        cv.Optional(CONF_INITIAL_VALUE, default=255): cv.float_,
    }
)

def to_code(config):
    var = yield number.new_number(
        config,
        min_value=config.get(CONF_MIN_VALUE, 0),
        max_value=config.get(CONF_MAX_VALUE, 255),
        step=config.get(CONF_STEP, 1)
    )
    yield cg.register_component(var, config)

    matrix = yield cg.get_variable(config[MATRIX_ID])
    cg.add(matrix.register_brightness(var))
    
    # Set initial value after creation
    if CONF_INITIAL_VALUE in config:
        cg.add(var.publish_state(config[CONF_INITIAL_VALUE]))