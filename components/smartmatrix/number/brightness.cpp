#include "brightness.h"
#include "esphome/core/log.h"

namespace esphome {
namespace smartmatrix {

static const char *const TAG = "smartmatrix.brightness";

void SmartMatrixBrightness::setup() {
  this->publish_state(this->parent_->get_initial_brightness());
}

void SmartMatrixBrightness::control(float value) {
  this->parent_->set_brightness(value);
  this->publish_state(value);
}

}  // namespace smartmatrix
}  // namespace esphome 