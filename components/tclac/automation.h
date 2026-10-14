#pragma once

#include "esphome/core/automation.h"
#include "tclac.h"

namespace esphome {
namespace tclac {

// Action template: turn on display
template<typename... Ts> class DisplayOnAction : public Action<Ts...> {
 public:
  DisplayOnAction(tclacClimate *parent) : parent_(parent) {}
  void play(Ts... x) { this->parent_->set_display_state(true); }

 protected:
  tclacClimate *parent_;
};

// Action template: turn off display
template<typename... Ts> class DisplayOffAction : public Action<Ts...> {
 public:
  DisplayOffAction(tclacClimate *parent) : parent_(parent) {}
  void play(Ts... x) { this->parent_->set_display_state(false); }

 protected:
  tclacClimate *parent_;
};

// Action template: turn on beeper
template<typename... Ts> class BeeperOnAction : public Action<Ts...> {
 public:
  BeeperOnAction(tclacClimate *parent) : parent_(parent) {}
  void play(Ts... x) { this->parent_->set_beeper_state(true); }

 protected:
  tclacClimate *parent_;
};

// Action template: turn off beeper
template<typename... Ts> class BeeperOffAction : public Action<Ts...> {
 public:
  BeeperOffAction(tclacClimate *parent) : parent_(parent) {}
  void play(Ts... x) { this->parent_->set_beeper_state(false); }

 protected:
  tclacClimate *parent_;
};

// Action template: turn on force settings application
template<typename... Ts> class ForceOnAction : public Action<Ts...> {
 public:
  ForceOnAction(tclacClimate *parent) : parent_(parent) {}
  void play(Ts... x) { this->parent_->set_force_mode_state(true); }

 protected:
  tclacClimate *parent_;
};

// Action template: turn off force settings application
template<typename... Ts> class ForceOffAction : public Action<Ts...> {
 public:
  ForceOffAction(tclacClimate *parent) : parent_(parent) {}
  void play(Ts... x) { this->parent_->set_force_mode_state(false); }

 protected:
  tclacClimate *parent_;
};

// Action template: set a fixed vertical louver position
template<typename... Ts> class SetVerticalDirectionAction : public Action<Ts...> {
 public:
  SetVerticalDirectionAction(tclacClimate *parent) : parent_(parent) {}
  void set_direction(AirflowVerticalDirection direction) { this->direction_ = direction; }
  void play(Ts... x) { this->parent_->set_vertical_direction(this->direction_); }

 protected:
  tclacClimate *parent_;
  AirflowVerticalDirection direction_;
};

// Action template: set a fixed horizontal louver position
template<typename... Ts> class SetHorizontalDirectionAction : public Action<Ts...> {
 public:
  SetHorizontalDirectionAction(tclacClimate *parent) : parent_(parent) {}
  void set_direction(AirflowHorizontalDirection direction) { this->direction_ = direction; }
  void play(Ts... x) { this->parent_->set_horizontal_direction(this->direction_); }

 protected:
  tclacClimate *parent_;
  AirflowHorizontalDirection direction_;
};

}  // namespace tclac
}  // namespace esphome
