#ifdef USE_RP2040
#include "LinBusListener.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/linbus_uart_component_rp2040.h"
#include "esphome/components/uart/uart_component_rp2040.h"
#include <SerialUART.h>

// Instance 1 for UART port 0
static esphome::linbus::LinBusListener *LIN_BUS_LISTENER_INSTANCE_1 = nullptr;
// Instance 2 for UART port 1
static esphome::linbus::LinBusListener *LIN_BUS_LISTENER_INSTANCE_2 = nullptr;

namespace esphome {
namespace lin_bus {

static const char *const TAG = "linbus.LinBusListener";

void LinBusListener::setup_framework() {
  auto uartComp = static_cast<esphome::uart::linbus_RP2040UartComponent *>(this->parent_);
  auto is_hw_serial = uartComp->is_hw_serial();
  if (!is_hw_serial) {
    ESP_LOGW(TAG, "Must use hardware serial SerialPIO is not supported.");
    this->mark_failed();
  }
  // auto hw_serial = static_cast<SerialUART *>(uartComp->get_hw_serial());
  auto hw_serial = uartComp->get_hw_serial();

  if ((*hw_serial) == Serial1) {
    LIN_BUS_LISTENER_INSTANCE_1 = this;
    this->uart_number_ = 1;
    this->uart_ = uart0;

  } else if ((*hw_serial) == Serial2) {
    LIN_BUS_LISTENER_INSTANCE_2 = this;
    this->uart_number_ = 2;
    this->uart_ = uart1;
  }

  if (this->uart_ != nullptr) {
    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(this->uart_, false);
  }
}

uint32_t LinBusListener::onSerialEvent() {
  this->onReceive_();

  if (this->uart_ != nullptr) {
    // Receive Status Register/Error Clear Register, UARTRSR/UARTECR
    // 0x00000004 [2]     : BE (0): Break error
    auto receive_status_register = uart_get_hw(this->uart_)->rsr;

    if ((receive_status_register & UART_UARTRSR_BE_BITS) == UART_UARTRSR_BE_BITS) {
      ESP_LOGVV(TAG, "UART%d RX break.", this->uart_number_);
      // If the break is valid the `onReceive` is called first and the break is handeld. Therfore the expectation is
      // that the state should be in waiting for `SYNC`.
      if (this->current_state_ != READ_STATE_BREAK || this->current_state_ != READ_STATE_SYNC) {
        this->current_state_ = READ_STATE_SYNC;
      }

      // Clear Receive Status Register
      hw_clear_bits(&uart_get_hw(this->uart_)->rsr, UART_UARTRSR_BE_BITS);
    }
  }

  if (this->current_state_ == READ_STATE_BREAK) {
    // Next is a break. CP Plus has an inter data break of ~35ms
    auto current = micros();
    if ((this->last_data_recieved_ + (1000 * 1000 /* 1 second */)) < current) {
      // I have not recieved data for a while. Sleep deeper
      return 750;
    } else if ((this->last_data_recieved_ + (50 * 1000 /* 0.1 second */)) < current) {
      // I have not recieved data for a while. Sleep deep
      return 50;
    } else {
      // Expecting a SYNC.
      return 10;
    }
  } else {
    // Expecting a byte.
    return 1;
  }
}

}  // namespace lin_bus
}  // namespace esphome

extern void loop1() {
  if (LIN_BUS_LISTENER_INSTANCE_1 == nullptr && LIN_BUS_LISTENER_INSTANCE_2 == nullptr) {
    // Wait for setup_framework to finish.
    delay(100);
  } else {
    uint32_t sleep1 = 0xFFFFFFFF;
    uint32_t sleep2 = 0xFFFFFFFF;
    if (LIN_BUS_LISTENER_INSTANCE_1 != nullptr) {
      sleep1 = LIN_BUS_LISTENER_INSTANCE_1->onSerialEvent();
    }
    if (LIN_BUS_LISTENER_INSTANCE_2 != nullptr) {
      sleep2 = LIN_BUS_LISTENER_INSTANCE_2->onSerialEvent();
    }
    // TODO: Reconsider processing lin messages here.
    // They contain blocking log messages.
    if (LIN_BUS_LISTENER_INSTANCE_1 != nullptr) {
      LIN_BUS_LISTENER_INSTANCE_1->process_lin_msg_queue_((portTickType) 0 /* No blocking*/);
    }
    if (LIN_BUS_LISTENER_INSTANCE_2 != nullptr) {
      LIN_BUS_LISTENER_INSTANCE_2->process_lin_msg_queue_((portTickType) 0 /* No blocking*/);
    }
    delay(sleep1 > sleep2 ? sleep2 : sleep1);
  }
}

#endif  // USE_RP2040
