// Copyright 2019 Project March.
#ifndef MARCH4CPP__SLAVE_H
#define MARCH4CPP__SLAVE_H

#include <stdexcept>
#include <ros/ros.h>

namespace march4cpp
{
class Slave
{
protected:
  int slaveIndex;

public:
  explicit Slave(int slaveIndex)
  {
    ROS_ASSERT_MSG(slaveIndex >= 1, "Slave configuration error: slaveindex %d can not be smaller than 1.", slaveIndex);
    this->slaveIndex = slaveIndex;
  };

  Slave()
  {
    slaveIndex = -1;
  };

  ~Slave() = default;

  virtual void writeInitialSDOs(int ecatCycleTime)
  {
  }

  int getSlaveIndex()
  {
    return this->slaveIndex;
  }
};
}

#endif