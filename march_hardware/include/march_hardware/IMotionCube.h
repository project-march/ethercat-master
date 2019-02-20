//
// Created by projectmarch on 13-2-19.
//

#include <march_hardware/EtherCAT/EthercatIO.h>

#ifndef MARCH4CPP__IMOTIONCUBE_H
#define MARCH4CPP__IMOTIONCUBE_H
namespace march4cpp
{
class IMotionCube
{
private:
  int slaveIndex;

  //    TODO(Martijn) add PDO/SDO settings here.

public:
  IMotionCube(int slaveIndex);

  IMotionCube()
  {
    slaveIndex = -1;
  }

  void initialize();

  bool PDOmapping();

  bool StartupSDO(uint8 ecatCycleTime);

  float getAngle();

  int getSlaveIndex();
};

}  // namespace march4cpp
#endif  // MARCH4CPP__IMOTIONCUBE_H
