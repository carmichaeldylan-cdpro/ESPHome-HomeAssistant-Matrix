#include "smartmatrix.h"

namespace esphome {
namespace smartmatrix {

static const char *const TAG = "SmartMatrixDisplay";

// --- MQTT state ---
static char applet_topic[32];
static char applet_rts_topic[32];
static char message_to_publish[16];

static bool has_received_size = false;
static bool need_publish = false;
static bool need_subscribe = true;

static WebPData webp_data;
static WebPDemuxer *demux = nullptr;
static WebPIterator iter;
static uint8_t *buffer = nullptr;
static unsigned long buffer_position = 0;
static unsigned long buffer_size = 0;
static unsigned long last_frame_time = 0;
static unsigned long last_frame_duration = 0;
static uint32_t webp_flags = 0;
static uint32_t current_frame = 1;
static uint32_t frame_count = 0;

static int current_mode = 1;  // 1 = READY, 2 = APPLET, 0 = NONE
static bool has_new_applet = false;
static bool is_on = true;

static char config_identifier[6];
static uint8_t mac_address[6];

void SmartMatrixDisplay::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SmartMatrixDisplay...");

  this->mqtt_client_ = mqtt::global_mqtt_client;
  esp_read_mac(mac_address, ESP_MAC_WIFI_STA);
  sprintf(config_identifier, "%02X%02X%02X", mac_address[3], mac_address[4], mac_address[5]);

  snprintf(applet_topic, sizeof(applet_topic), "plm/%s/rx", config_identifier);
  snprintf(applet_rts_topic, sizeof(applet_rts_topic), "plm/%s/tx", config_identifier);

  if (this->device_id_sensor_ != nullptr) {
    this->device_id_sensor_->publish_state(config_identifier);
  }

  // Create config from user pins and dimensions
  HUB75_I2S_CFG config(this->panel_width_, this->panel_height_, this->chain_length_, this->pins_);
  config.double_buff = true;

  this->dma_display_ = new MatrixPanel_I2S_DMA(config);
  this->dma_display_->begin();
  this->dma_display_->clearScreen();

  this->set_brightness(this->initial_brightness_);
  this->enabled_ = true;

  if (wifi::global_wifi_component->has_sta() && this->mqtt_client_->is_connected()) {
    this->mqtt_client_->subscribe(applet_topic, [this](const std::string &topic, const std::string &payload) {
      this->on_message(payload);
    });
    need_subscribe = false;
    strncpy(message_to_publish, "DEVICE_BOOT", sizeof(message_to_publish));
    need_publish = true;
  }
}

void SmartMatrixDisplay::loop() {
  if (!wifi::global_wifi_component->has_sta()) return;

  if (!this->mqtt_client_->is_connected()) return;

  if (need_subscribe) {
    this->mqtt_client_->subscribe(applet_topic, [this](const std::string &topic, const std::string &payload) {
      this->on_message(payload);
    });
    need_subscribe = false;
  }

  if (need_publish) {
    this->mqtt_client_->publish(applet_rts_topic, message_to_publish);
    need_publish = false;
  }

  if (current_mode == 2) {
    if (has_new_applet) {
      demux = WebPDemux(&webp_data);
      frame_count = WebPDemuxGetI(demux, WEBP_FF_FRAME_COUNT);
      webp_flags = WebPDemuxGetI(demux, WEBP_FF_FORMAT_FLAGS);
      has_new_applet = false;
      current_frame = 1;
    } else {
      if ((webp_flags & ANIMATION_FLAG) && (millis() - last_frame_time > last_frame_duration)) {
        if (WebPDemuxGetFrame(demux, current_frame, &iter)) {
          if (WebPDecodeRGBAInto(iter.fragment.bytes, iter.fragment.size,
                                 this->pixel_buffer_, iter.width * iter.height * 4, iter.width * 4)) {
            int px = 0;
            for (int y = iter.y_offset; y < iter.y_offset + iter.height; y++) {
              for (int x = iter.x_offset; x < iter.x_offset + iter.width; x++) {
                int alpha = this->pixel_buffer_[px * 4 + 3];
                if (alpha == 255) {
                  this->dma_display_->drawPixelRGB888(x, y,
                    this->pixel_buffer_[px * 4], this->pixel_buffer_[px * 4 + 1], this->pixel_buffer_[px * 4 + 2]);
                }
                px++;
              }
            }
            last_frame_time = millis();
            last_frame_duration = iter.duration;
            current_frame = (current_frame % frame_count) + 1;
          }
        } else {
          current_mode = 0;
        }
      } else {
        if (WebPDecodeRGBInto(webp_data.bytes, webp_data.size,
                              this->pixel_buffer_,
                              this->panel_height_ * this->panel_width_ * 3,
                              this->panel_width_ * 3)) {
          for (int y = 0; y < this->panel_height_; y++) {
            for (int x = 0; x < this->panel_width_; x++) {
              int idx = (y * this->panel_width_ + x) * 3;
              this->dma_display_->drawPixelRGB888(
                x, y,
                this->pixel_buffer_[idx], this->pixel_buffer_[idx + 1], this->pixel_buffer_[idx + 2]
              );
            }
          }
          current_mode = 0;
        }
      }
    }
  }
}

void SmartMatrixDisplay::update() {
  // This is called by ESPHome's display system when it wants to update the display
  // We don't need to do anything here since we're handling updates in the loop()
  // for MQTT-based updates
}

void SmartMatrixDisplay::fill(Color color) {
  if (this->dma_display_)
    this->dma_display_->fillScreenRGB888(color.r, color.g, color.b);
}

void SmartMatrixDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || y < 0 || x >= this->get_width_internal() || y >= this->get_height_internal()) return;
  if (this->dma_display_)
    this->dma_display_->drawPixelRGB888(x, y, color.r, color.g, color.b);
}

void SmartMatrixDisplay::on_message(const std::string &payload) {
  const char *data = payload.c_str();
  size_t len = payload.length();

  if (strncmp(data, "START", 5) == 0) {
    has_received_size = false;
    buffer_position = 0;
    strncpy(message_to_publish, "OK", sizeof(message_to_publish));
    need_publish = true;
  } else if (strncmp(data, "PING", 4) == 0) {
    strncpy(message_to_publish, "PONG", sizeof(message_to_publish));
    need_publish = true;
  } else if (!has_received_size) {
    buffer_size = atoi(data);
    if (buffer_size <= 0 || buffer_size > (ESP.getFreeHeap() - 1024)) {
      strncpy(message_to_publish, "ERROR", sizeof(message_to_publish));
      need_publish = true;
      return;
    }
    if (buffer) free(buffer);
    buffer = static_cast<uint8_t *>(malloc(buffer_size));
    has_received_size = buffer != nullptr;
    strncpy(message_to_publish, has_received_size ? "OK" : "ERROR", sizeof(message_to_publish));
    need_publish = true;
  } else {
    if (strncmp(data, "FINISH", 6) == 0) {
      if (strncmp((const char *)buffer, "RIFF", 4) == 0) {
        WebPDataClear(&webp_data);
        WebPDemuxReleaseIterator(&iter);
        WebPDemuxDelete(demux);
        webp_data.size = buffer_size;
        webp_data.bytes = static_cast<uint8_t *>(WebPMalloc(buffer_size));
        if (webp_data.bytes) {
          memcpy((void *)webp_data.bytes, buffer, buffer_size);
          has_new_applet = true;
          current_mode = 2;
          strncpy(message_to_publish, "PUSHED", sizeof(message_to_publish));
        } else {
          strncpy(message_to_publish, "DECODE_ERROR", sizeof(message_to_publish));
        }
        free(buffer);
        buffer = nullptr;
      } else {
        strncpy(message_to_publish, "DECODE_ERROR", sizeof(message_to_publish));
      }
      buffer_position = 0;
      has_received_size = false;
      need_publish = true;
    } else {
      if (buffer_position + len <= buffer_size) {
        memcpy(buffer + buffer_position, data, len);
        buffer_position += len;
        strncpy(message_to_publish, "OK", sizeof(message_to_publish));
        need_publish = true;
      }
    }
  }
}

void SmartMatrixDisplay::set_brightness(int brightness) {
  brightness = clamp(brightness, 0, 255);
  if (is_on && this->dma_display_)
    this->dma_display_->setBrightness8(brightness);
}

void SmartMatrixDisplay::set_state(bool state) {
  is_on = state;
  this->set_brightness(state ? this->initial_brightness_ : 0);
}

void SmartMatrixDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "SmartMatrixDisplay:");
  ESP_LOGCONFIG(TAG, "  Panel size: %dx%d", this->panel_width_, this->panel_height_);
  ESP_LOGCONFIG(TAG, "  Chain length: %d", this->chain_length_);
  ESP_LOGCONFIG(TAG, "  Initial brightness: %d", this->initial_brightness_);
}

}  // namespace smartmatrix
}  // namespace esphome
