#pragma once

#include "esphome/core/component.h"
#include "esphome/core/optional.h"
#include "esphome/core/automation.h"

#include <list>
#include <queue>
#include <set>
#include <vector>

#include "LinBusProtocol.h"

namespace esphome {
namespace linbus {

#define LIN_SID_RESPONSE 0x40
#define LIN_SID_READ_STATE_BUFFER 0xBA
#define LIN_SID_FIll_STATE_BUFFFER 0xBB

/* struct StatusFrameListener {
  std::function<void(const StatusFrameHeater *)> on_heater_change = nullptr;
  std::function<void(const StatusFrameTimer *)> on_timer_change = nullptr;
  std::function<void(const StatusFrameClock *)> on_clock_change = nullptr;
  std::function<void(const StatusFrameConfig *)> on_config_change = nullptr;
};
 */

class LinbusTrigger;
template<typename... Ts> class LinbusSendAction;

// LinBus derives from LinBusProtocol protocoll derives from polling component and uart
class LinBus : public LinBusProtocol {
 public:
  // slave not yet implementded
  // LinBus(uint8_t expected_listener_count);

  // master onlysend:
  LinBus(){};

  // These methods are derived from baseclass and found in LinBusListener:
  // void setup() override;
  // void dump_config() override;

  void update() override;

  // should not bee needed, polling should be able to handle...
  // void loop() override;

  void lin_heartbeat() override;
  void lin_reset_device() override;
  void add_trigger(LinbusTrigger *trigger);
  void send_data(uint8_t lin_pid, const std::vector<uint8_t> &data);

  //  void add_trigger(LinbusTrigger *trigger);

 protected:
  template<typename... Ts> friend class LinbusSendAction;
  std::vector<LinbusTrigger *> triggers_{};
  uint8_t pid_{0x00};

  // bool answer_lin_order_(const uint8_t pid) override;


};

template<typename... Ts> class LinbusSendAction : public Action<Ts...>, public Parented<LinBus> {
 public:
  void set_data_template(const std::function<std::vector<uint8_t>(Ts...)> func) {
    this->data_func_ = func;
    this->static_ = false;
  }
  void set_data_static(const std::vector<uint8_t> &data) {
    this->data_static_ = data;
    this->static_ = true;
  }

  void set_lin_id(int8_t lin_pid) { this->pid_ = lin_pid; }

 protected:
  optional<uint8_t> lin_pid_{};
  bool static_{false};
  std::function<std::vector<uint8_t>(Ts...)> data_func_{};
  std::vector<uint8_t> data_static_{};
};

class LinbusTrigger : public Trigger<std::vector<uint8_t>, uint32_t, bool>, public Component {
  friend class Linbus;

 public:
  explicit LinbusTrigger(LinBus *parent, const std::uint8_t lin_id) : parent_(parent), lin_id_(lin_id){};

  // void setup() override { this->add_trigger(this); }
  void setup() override { this->parent_->add_trigger(this); }

 protected:
  LinBus *parent_;
  uint32_t lin_id_;
};

}  // namespace linbus
}  // namespace esphome
