import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import display, number
from esphome.const import (
    CONF_HEIGHT,
    CONF_ID,
    CONF_LAMBDA,
    CONF_UPDATE_INTERVAL,
    CONF_WIDTH,
)

DEPENDENCIES = ["esp32", "mqtt"]
AUTO_LOAD = ["text_sensor", "number"]

smartmatrix_ns = cg.esphome_ns.namespace("smartmatrix")
SmartMatrixDisplay = smartmatrix_ns.class_(
    "SmartMatrixDisplay", cg.Component, display.DisplayBuffer
)

MATRIX_ID = "matrix_id"

CHAIN_LENGTH = "chain_length"
BRIGHTNESS = "brightness"

R1_PIN = "R1_pin"
G1_PIN = "G1_pin"
B1_PIN = "B1_pin"
R2_PIN = "R2_pin"
G2_PIN = "G2_pin"
B2_PIN = "B2_pin"

A_PIN = "A_pin"
B_PIN = "B_pin"
C_PIN = "C_pin"
D_PIN = "D_pin"
E_PIN = "E_pin"

LAT_PIN = "LAT_pin"
OE_PIN = "OE_pin"
CLK_PIN = "CLK_pin"

CONFIG_SCHEMA = display.FULL_DISPLAY_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SmartMatrixDisplay),
        cv.Optional(CONF_WIDTH, default=64): cv.positive_int,
        cv.Optional(CONF_HEIGHT, default=32): cv.positive_int,
        cv.Optional(CHAIN_LENGTH, default=1): cv.positive_int,
        cv.Optional(BRIGHTNESS, default=255): cv.int_range(min=0, max=255),
        cv.Optional(CONF_UPDATE_INTERVAL, default="16ms"): cv.positive_time_period_milliseconds,
        cv.Optional(R1_PIN, default=25): pins.gpio_output_pin_schema,
        cv.Optional(G1_PIN, default=26): pins.gpio_output_pin_schema,
        cv.Optional(B1_PIN, default=27): pins.gpio_output_pin_schema,
        cv.Optional(R2_PIN, default=14): pins.gpio_output_pin_schema,
        cv.Optional(G2_PIN, default=12): pins.gpio_output_pin_schema,
        cv.Optional(B2_PIN, default=13): pins.gpio_output_pin_schema,
        cv.Optional(A_PIN, default=23): pins.gpio_output_pin_schema,
        cv.Optional(B_PIN, default=19): pins.gpio_output_pin_schema,
        cv.Optional(C_PIN, default=5): pins.gpio_output_pin_schema,
        cv.Optional(D_PIN, default=17): pins.gpio_output_pin_schema,
        cv.Optional(E_PIN): pins.gpio_output_pin_schema,
        cv.Optional(LAT_PIN, default=4): pins.gpio_output_pin_schema,
        cv.Optional(OE_PIN, default=15): pins.gpio_output_pin_schema,
        cv.Optional(CLK_PIN, default=16): pins.gpio_output_pin_schema,
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await display.register_display(var, config)

    cg.add(var.set_panel_width(config[CONF_WIDTH]))
    cg.add(var.set_panel_height(config[CONF_HEIGHT]))
    cg.add(var.set_chain_length(config[CHAIN_LENGTH]))
    cg.add(var.set_initial_brightness(config[BRIGHTNESS]))

    # Resolve all pins
    R1 = await cg.gpio_pin_expression(config[R1_PIN])
    G1 = await cg.gpio_pin_expression(config[G1_PIN])
    B1 = await cg.gpio_pin_expression(config[B1_PIN])
    R2 = await cg.gpio_pin_expression(config[R2_PIN])
    G2 = await cg.gpio_pin_expression(config[G2_PIN])
    B2 = await cg.gpio_pin_expression(config[B2_PIN])

    A = await cg.gpio_pin_expression(config[A_PIN])
    B = await cg.gpio_pin_expression(config[B_PIN])
    C = await cg.gpio_pin_expression(config[C_PIN])
    D = await cg.gpio_pin_expression(config[D_PIN])
    LAT = await cg.gpio_pin_expression(config[LAT_PIN])
    OE = await cg.gpio_pin_expression(config[OE_PIN])
    CLK = await cg.gpio_pin_expression(config[CLK_PIN])

    if E_PIN in config:
        E = await cg.gpio_pin_expression(config[E_PIN])
    else:
        E = cg.nullptr  # Not used

    cg.add(var.set_pins(R1, G1, B1, R2, G2, B2, A, B, C, D, E, LAT, OE, CLK))

    # Optional lambda
    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))