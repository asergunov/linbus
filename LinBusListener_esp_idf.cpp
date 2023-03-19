#ifdef USE_ESP32_FRAMEWORK_ESP_IDF
#include "LinBusListener.h"
#include "esphome/core/log.h"
#include "soc/uart_reg.h"
#include "esphome/components/uart/linbus_uart_component_esp_idf.h"
#include "esphome/components/uart/uart_component_esp_idf.h"

namespace esphome {
namespace lin_bus {

static const char *const TAG = "linbus.LinBusListener";

void LinBusListener::setup_framework() {
  // uartSetFastReading
  auto uartComp = static_cast<uart::linbus_IDFUARTComponent *>(this->parent_);

  auto uart_num = uartComp->get_hw_serial_number();

  // Tweak the fifo settings so data is available as soon as the first byte is recieved.
  // If not it will wait either until fifo is filled or a certain time has passed.
  uart_intr_config_t uart_intr;
  uart_intr.intr_enable_mask =
      UART_RXFIFO_FULL_INT_ENA_M | UART_RXFIFO_TOUT_INT_ENA_M;  // only these IRQs - no BREAK, PARITY or OVERFLOW
  uart_intr.rxfifo_full_thresh =
      1;  // UART_FULL_THRESH_DEFAULT,  //120 default!! aghh! need receive 120 chars before we see them
  uart_intr.rx_timeout_thresh =
      10;  // UART_TOUT_THRESH_DEFAULT,  //10 works well for my short messages I need send/receive
  uart_intr.txfifo_empty_intr_thresh = 10;  // UART_EMPTY_THRESH_DEFAULT
  uart_intr_config(uart_num, &uart_intr);

  // Creating UART event Task
  xTaskCreatePinnedToCore(LinBusListener::uartEventTask_,
                          "uart_event_task",                      // name
                          ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE,   // stack size (in words)
                          this,                                   // input params
                          24,                                     // priority
                          &this->uartEventTaskHandle_,            // handle
                          ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE  // core
  );
  if (this->uartEventTaskHandle_ == NULL) {
    ESP_LOGE(TAG, " -- UART%d Event Task not created!", uart_num);
  }

  // Creating LIN msg event Task
  xTaskCreatePinnedToCore(LinBusListener::eventTask_,
                          "lin_event_task",                       // name
                          ARDUINO_SERIAL_EVENT_TASK_STACK_SIZE,   // stack size (in words)
                          this,                                   // input params
                          2,                                      // priority
                          &this->eventTaskHandle_,                // handle
                          ARDUINO_SERIAL_EVENT_TASK_RUNNING_CORE  // core
  );

  if (this->eventTaskHandle_ == NULL) {
    ESP_LOGE(TAG, " -- LIN message Task not created!");
  }
}

void LinBusListener::uartEventTask_(void *args) {
  LinBusListener *instance = (LinBusListener *) args;
  auto uartComp = static_cast<uart::linbus_IDFUARTComponent *>(instance->parent_);
  auto uart_num = uartComp->get_hw_serial_number();
  auto uartEventQueue = uartComp->get_uart_event_queue();
  uart_event_t event;
  for (;;) {
    // Waiting for UART event.
    if (xQueueReceive(*uartEventQueue, (void *) &event, (portTickType) portMAX_DELAY)) {
      if (event.type == UART_DATA && instance->available() > 0) {
        instance->onReceive_();
      } else if (event.type == UART_BREAK) {
        // If the break is valid the `onReceive` is called first and the break is handeld. Therfore the expectation is
        // that the state should be in waiting for `SYNC`.
        if (instance->current_state_ != READ_STATE_SYNC) {
          instance->current_state_ = READ_STATE_BREAK;
        }
      }
    }
  }
  vTaskDelete(NULL);
}

void LinBusListener::eventTask_(void *args) {
  LinBusListener *instance = (LinBusListener *) args;
  for (;;) {
    instance->process_lin_msg_queue_((portTickType) portMAX_DELAY);
  }
}

}  // namespace lin_bus
}  // namespace esphome

#endif  // USE_ESP32_FRAMEWORK_ESP_IDF