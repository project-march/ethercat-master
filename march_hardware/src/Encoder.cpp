// Copyright 2019 Project March.

#include <march_hardware/EtherCAT/EthercatIO.h>
#include <march_hardware/Encoder.h>
#include <cmath>
#include <ros/ros.h>

namespace march
{
Encoder::Encoder(int numberOfBits, int32_t minPositionIU, int32_t maxPositionIU, int32_t zeroPositionIU,
                 float safetyMarginRad)
{
  ROS_ASSERT_MSG(numberOfBits > 0 && numberOfBits <= 32, "Encoder resolution of %d is not within range (0, 32)",
                 numberOfBits);
  this->totalPositions = static_cast<int>(pow(2, numberOfBits) - 1);

  ROS_ASSERT_MSG(safetyMarginRad >= 0, "SafetyMarginRad %f is below zero", safetyMarginRad);

  this->safetyMarginRad = safetyMarginRad;
  this->slaveIndex = -1;
  this->upperHardLimitIU = maxPositionIU;
  this->lowerHardLimitIU = minPositionIU;
  this->zeroPositionIU = zeroPositionIU;
  int safetyMarginIU = RadtoIU(safetyMarginRad) - this->zeroPositionIU;
  this->upperSoftLimitIU = this->upperHardLimitIU - safetyMarginIU;
  this->lowerSoftLimitIU = this->lowerHardLimitIU + safetyMarginIU;

  ROS_ASSERT_MSG(this->lowerSoftLimitIU < this->upperSoftLimitIU,
                 "Invalid range of motion. Safety margin too large or "
                 "min/max position invalid. lowerSoftLimit: %i IU, upperSoftLimit: "
                 "%i IU. lowerHardLimit: %i IU, upperHardLimit %i IU. safetyMargin: %f rad = %i IU",
                 this->lowerSoftLimitIU, this->upperSoftLimitIU, this->lowerHardLimitIU, this->upperHardLimitIU,
                 this->safetyMarginRad, safetyMarginIU);
}

double Encoder::getAngleRad(uint8_t ActualPositionByteOffset)
{
  return IUtoRad(getAngleIU(ActualPositionByteOffset));
}

int32_t Encoder::getAngleIU(uint8_t ActualPositionByteOffset)
{
  if (this->slaveIndex == -1)
  {
    ROS_FATAL("Encoder has slaveIndex of -1");
  }
  union bit32 return_byte = get_input_bit32(this->slaveIndex, ActualPositionByteOffset);
  ROS_DEBUG("Encoder read (IU): %d", return_byte.i);
  return return_byte.i;
}

int32_t Encoder::RadtoIU(double rad)
{
  return static_cast<int32_t>(rad * totalPositions / (2 * M_PI) + zeroPositionIU);
}

double Encoder::IUtoRad(int32_t iu)
{
  return (iu - zeroPositionIU) * 2 * M_PI / totalPositions;
}

void Encoder::setSlaveIndex(int slaveIndex)
{
  this->slaveIndex = slaveIndex;
}

int Encoder::getSlaveIndex() const
{
  return this->slaveIndex;
}

bool Encoder::isWithinHardLimitsIU(int32_t positionIU)
{
  return (positionIU > this->lowerHardLimitIU && positionIU < this->upperHardLimitIU);
}

bool Encoder::isWithinSoftLimitsIU(int32_t positionIU)
{
  return (positionIU > this->lowerSoftLimitIU && positionIU < this->upperSoftLimitIU);
}

bool Encoder::isValidTargetIU(int32_t currentIU, int32_t targetIU)
{
  if (this->isWithinSoftLimitsIU(targetIU))
  {
    return true;
  }

  if (currentIU >= this->getUpperSoftLimitIU())
  {
    return (targetIU <= currentIU) && (targetIU > this->getLowerSoftLimitIU());
  }

  if (currentIU <= this->getLowerSoftLimitIU())
  {
    return (targetIU >= currentIU) && (targetIU < this->getUpperSoftLimitIU());
  }

  return false;
}

int32_t Encoder::getUpperSoftLimitIU() const
{
  return upperSoftLimitIU;
}

int32_t Encoder::getLowerSoftLimitIU() const
{
  return lowerSoftLimitIU;
}

int32_t Encoder::getUpperHardLimitIU() const
{
  return upperHardLimitIU;
}

int32_t Encoder::getLowerHardLimitIU() const
{
  return lowerHardLimitIU;
}

}  // namespace march
