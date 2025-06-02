#pragma once

#include "LinBus.h"

namespace esphome {
namespace linbus {

uint8_t addr_parity(const uint8_t pid);
uint8_t data_checksum(const uint8_t *message, uint8_t length, uint16_t sum);


}  // namespace linbus
}  // namespace esphome