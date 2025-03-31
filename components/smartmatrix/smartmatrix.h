#pragma once

#include <utility>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/core/helpers.h"

#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/wifi/wifi_component.h"

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <webp/demux.h>
#include <Fonts/TomThumb.h>

namespace esphome {
namespace smartmatrix {

class SmartMatrixBrightness;  // Forward declaration

class SmartMatrixDisplay : public display::DisplayBuffer {
  public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void update() override;

  display::DisplayType get_display_type() override {
    return display::DisplayType::DISPLAY_TYPE_COLOR;
  }

  // Rendering
  void fill(Color color) override;
  void draw_absolute_pixel_internal(int x, int y, Color color) override;

  // Display config
  void set_panel_width(int width) { panel_width_ = width; }
  void set_panel_height(int height) { panel_height_ = height; }
  void set_chain_length(int length) { chain_length_ = length; }
  void set_initial_brightness(int brightness) { initial_brightness_ = brightness; }
  int get_initial_brightness() const { return initial_brightness_; }

  // MQTT
  void on_message(const std::string &payload);

  // External references
  void set_device_id_sensor(text_sensor::TextSensor *sensor) { device_id_sensor_ = sensor; }

  // Brightness control
  void set_brightness(int brightness);
  void set_state(bool state);

  void register_brightness(SmartMatrixBrightness *brightness) {
    this->brightness_ = brightness;
  }

  // Pin setup
  void set_pins(InternalGPIOPin *R1_pin, InternalGPIOPin *G1_pin, InternalGPIOPin *B1_pin,
                InternalGPIOPin *R2_pin, InternalGPIOPin *G2_pin, InternalGPIOPin *B2_pin,
                InternalGPIOPin *A_pin, InternalGPIOPin *B_pin, InternalGPIOPin *C_pin,
                InternalGPIOPin *D_pin, InternalGPIOPin *E_pin, InternalGPIOPin *LAT_pin,
                InternalGPIOPin *OE_pin, InternalGPIOPin *CLK_pin) {
    int8_t e = -1;
    if (E_pin != nullptr)
      e = E_pin->get_pin();

    pins_ = {
      static_cast<int8_t>(R1_pin->get_pin()), static_cast<int8_t>(G1_pin->get_pin()),
      static_cast<int8_t>(B1_pin->get_pin()), static_cast<int8_t>(R2_pin->get_pin()),
      static_cast<int8_t>(G2_pin->get_pin()), static_cast<int8_t>(B2_pin->get_pin()),
      static_cast<int8_t>(A_pin->get_pin()),  static_cast<int8_t>(B_pin->get_pin()),
      static_cast<int8_t>(C_pin->get_pin()),  static_cast<int8_t>(D_pin->get_pin()),
      e, static_cast<int8_t>(LAT_pin->get_pin()), static_cast<int8_t>(OE_pin->get_pin()),
      static_cast<int8_t>(CLK_pin->get_pin())
    };
  }

  protected:
  // Rendering bounds
  int get_width_internal() override { return panel_width_ * chain_length_; }
  int get_height_internal() override { return panel_height_; }

  // Display internals
  MatrixPanel_I2S_DMA *dma_display_{nullptr};
  HUB75_I2S_CFG::i2s_pins pins_{};

  int panel_width_{64};
  int panel_height_{32};
  int chain_length_{1};
  int initial_brightness_{255};
  bool enabled_{true};

  uint8_t pixel_buffer_[64 * 32 * 4];  // Or dynamically sized later

  // External devices
  mqtt::MQTTClientComponent *mqtt_client_;
  text_sensor::TextSensor *device_id_sensor_{nullptr};

  // Brightness control
  SmartMatrixBrightness *brightness_{nullptr};
};

}  // namespace smartmatrix
}  // namespace esphome
