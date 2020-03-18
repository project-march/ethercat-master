// Copyright 2020 Project March.

#ifndef MARCH_HARDWARE_ERROR_TYPE_H
#define MARCH_HARDWARE_ERROR_TYPE_H
#include <ostream>

namespace march
{
namespace error
{
enum class ErrorType
{
  INVALID_ACTUATION_MODE = 100,
  INVALID_ACTUATE_POSITION = 101,
  ENCODER_RESET = 102,
  OUTSIDE_HARD_LIMITS = 103,
  TARGET_EXCEEDS_MAX_DIFFERENCE = 104,
  TARGET_TORQUE_EXCEEDS_MAX_TORQUE = 105,
  PDO_OBJECT_NOT_DEFINED = 106,
  PDO_REGISTER_OVERFLOW = 107,
  WRITING_INITIAL_SETTINGS_FAILED = 108,
  NO_SOCKET_CONNECTION = 109,
  NOT_ALL_SLAVES_FOUND = 110,
  FAILED_TO_REACH_OPERATIONAL_STATE = 111,
  INVALID_ENCODER_RESOLUTION = 112,
  INVALID_RANGE_OF_MOTION = 114,
  UNKNOWN = 999,
};

const char* getErrorDescription(ErrorType type);

std::ostream& operator<<(std::ostream& s, ErrorType type);
}  // namespace error
}  // namespace march

#endif  // MARCH_HARDWARE_ERROR_TYPE_H
