// Copyright 2019 Project March.
#ifndef MARCH_HARDWARE_INTERFACE_POWERNETTYPE_H
#define MARCH_HARDWARE_INTERFACE_POWERNETTYPE_H
#include <string>
#include <ros/package.h>

class PowerNetType
{
public:
  enum Value : int
  {
    undefined = 0,
    high_voltage = 1,
    low_voltage = 2
  };

  PowerNetType()
  {
    value = undefined;
  }

  explicit PowerNetType(const std::string& name)
  {
    if (name == "high_voltage")
    {
      this->value = high_voltage;
    }
    else if (name == "low_voltage")
    {
      this->value = low_voltage;
    }
    else
    {
      ROS_ASSERT_MSG(false, "Unknown power net type %s", name.c_str());
      this->value = undefined;
    }
  }

  bool operator==(PowerNetType a) const
  {
    return value == a.value;
  }
  bool operator!=(PowerNetType a) const
  {
    return value != a.value;
  }

  bool operator==(int a) const
  {
    return value == a;
  }
  bool operator!=(int a) const
  {
    return value != a;
  }

  /** @brief Override stream operator for clean printing */
  friend ::std::ostream& operator<<(std::ostream& os, const PowerNetType& powerNetType)
  {
    if (powerNetType.value == high_voltage)
    {
      return os << "PowerNetType(type:HighVoltage)";
    }
    else if (powerNetType.value == low_voltage)
    {
      return os << "PowerNetType(type:LowVoltage)";
    }
    else
    {
      return os << "PowerNetType(type:Undefined)";
    }
  }

private:
  Value value;
};

#endif  // MARCH_HARDWARE_INTERFACE_POWERNETTYPE_H
