#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "../smartmatrix.h"

namespace esphome {
namespace smartmatrix {

class SmartMatrixBrightness : public number::Number, public Component {
 public:
  void setup() override;
  void control(float value) override;
  void set_parent(SmartMatrixDisplay *parent) { this->parent_ = parent; }

 protected:
  SmartMatrixDisplay *parent_;
};

}  // namespace smartmatrix
}  // namespace esphome 