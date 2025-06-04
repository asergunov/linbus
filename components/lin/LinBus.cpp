#include "LinBus.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "helpers.h"

namespace esphome {
namespace linbus {

static const char *const TAG = "linbus.LinBus";

// the listeners are needed for slave, not yet implemented here
// LinBus::LinBus(uint8_t expected_listener_count) {
//   this->listeners_heater_.reserve(expected_listener_count);
// }

// setup() and dump_config() are not overloaded. LinProtocol handles them.


void LinBus::send_data(uint8_t lin_pid, const std::vector<uint8_t> &data) {
  uint8_t len = static_cast<uint8_t>(data.size());

  // assuming we are master, different if slave, not implemented yet
  
  if (len > LIN_MAX_DATA_LENGTH)
    len = LIN_MAX_DATA_LENGTH;

  // call the protocol to send Data
  send_lin_pid_withdata_(data.data(), len, lin_pid);
}

void LinBus::lin_heartbeat() { this->device_registered_ = micros(); }

void LinBus::lin_reset_device() {
  LinBusProtocol::lin_reset_device();
  this->device_registered_ = micros();
  // this->init_recieved_ = 0;

  // this->update_time_ = 0;
}

// A listener is needed as slave, truma inet code below, not needed for LIN master:
// void LinBus::register_listener(const std::function<void(const StatusFrameHeater *)> &func) {
//   StatusFrameListener listener = {};
//   listener.on_heater_change = func;
//   this->listeners_heater_.push_back(std::move(listener));

//   if (this->status_heater_valid_) {
//     func(&this->status_heater_);
//   }
// }

void LinBus::add_trigger(LinbusTrigger *trigger) {
  ESP_LOGVV(TAG, "add trigger for pid=0x%03" PRIx32 "", trigger->lin_id_);

  this->triggers_.push_back(trigger);
};

// a slave function, maybe a "set_data" prepares the Slave response
// as Slave cannot "send"
bool LinBus::answer_lin_order_(const uint8_t pid) {
  // Alive message
  // TODO: ???
  // if (pid == LIN_PID_linbusINET_BOX) {
  //   std::array<uint8_t, 8> response = this->lin_empty_response_;

  //   if (this->updates_to_send_.empty() && !this->has_update_to_submit_()) {
  //     response[0] = 0xFE;
  //   }
  //   this->write_lin_answer_(response.data(), (uint8_t) sizeof(response));
  //   return true;
  // }
  return LinBusProtocol::answer_lin_order_(pid);
}


// slave functionq
/* bool LinBus::has_update_to_submit_() {
  // No logging in this message!
  // It is called by interrupt. Logging is a blocking operation (especially when Wifi Logging).
  // If logging is necessary use logging queue of LinBusListener class.
  if (this->init_requested_ == 0) {
    this->init_requested_ = micros();
    // ESP_LOGD(TAG, "Requesting initial data.");
    return true;
  } else if (this->init_recieved_ == 0) {
    auto init_wait_time = micros() - this->init_requested_;
    // it has been 5 seconds and i am still awaiting the init data.
    if (init_wait_time > 1000 * 1000 * 5) {
      // ESP_LOGD(TAG, "Requesting initial data again.");
      this->init_requested_ = micros();
      return true;
    }
  } else if (this->update_status_heater_unsubmitted_ || this->update_status_timer_unsubmitted_ ||
             this->update_status_clock_unsubmitted_) {
    if (this->update_time_ == 0) {
      // ESP_LOGD(TAG, "Notify CP Plus I got updates.");
      this->update_time_ = micros();
      return true;
    }
    auto update_wait_time = micros() - this->update_time_;
    if (update_wait_time > 1000 * 1000 * 5) {
      // ESP_LOGD(TAG, "Notify CP Plus again I still got updates.");
      this->update_time_ = micros();
      return true;
    }
  }
  return false;
} */

// was loop, not sure yet for lin
void LinBus::update() {
  // read all messages until queue is empty

  // check if incoming messages are present and

  // TODO: ???
  // from receive que jkjk:
  uint8_t lin_id = 0x11;
  // fire all triggers
  // for (auto *trigger : this->triggers_) {
  //   if (trigger->lin_id_ == lin_id) {
  //     trigger->trigger(data, lin_id);
  //     // where does it go? jkjk
  //   }
  // }

  LinBusProtocol::update();
}
}  // namespace linbus
}  // namespace esphome