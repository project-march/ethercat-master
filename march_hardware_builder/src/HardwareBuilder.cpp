// Copyright 2019 Project March.

#include "ros/ros.h"
#include <march_hardware_builder/HardwareBuilder.h>
#include <march_hardware_builder/HardwareConfigExceptions.h>

HardwareBuilder::HardwareBuilder(std::string yamlPath)
{
  this->yamlPath = yamlPath;
  this->robotConfig = YAML::LoadFile(yamlPath);
}

HardwareBuilder::HardwareBuilder(AllowedRobot robot)
{
  this->yamlPath = robot.getFilePath();
  ROS_INFO("yamlPath %s", this->yamlPath.c_str());
  this->robotConfig = YAML::LoadFile(yamlPath);
}

HardwareBuilder::HardwareBuilder() = default;

march4cpp::MarchRobot HardwareBuilder::createMarchRobot(YAML::Node marchRobotConfig)
{
  std::string robotName = marchRobotConfig.begin()->first.as<std::string>();
  ROS_INFO("Started creation of robot %s", robotName.c_str());

  // Remove top level robot name key
  marchRobotConfig = marchRobotConfig[robotName];
  std::string ifName = marchRobotConfig["ifName"].as<std::string>();
  int ecatCycleTime = marchRobotConfig["ecatCycleTime"].as<int>();

  YAML::Node jointListConfig = marchRobotConfig["joints"];

  std::vector<march4cpp::Joint> jointList;

  for (std::size_t i = 0; i < jointListConfig.size(); i++)
  {
    YAML::Node jointConfig = jointListConfig[i];
    std::string jointName = jointConfig.begin()->first.as<std::string>();
    jointList.push_back(this->createJoint(jointConfig[jointName], jointName));
  }

  ROS_INFO_STREAM("marchRobotConfig " << marchRobotConfig << "\n");
  if (marchRobotConfig["powerDistributionBoard"].Type() != YAML::NodeType::Undefined)
  {
    march4cpp::PowerDistributionBoard powerDistributionBoard =
        createPowerDistributionBoard(marchRobotConfig["powerDistributionBoard"]);
    ROS_INFO_STREAM("PowerDistributionBoard: " << powerDistributionBoard);
    return march4cpp::MarchRobot(jointList, powerDistributionBoard, ifName, ecatCycleTime);
  }
  else
  {
    ROS_INFO("powerDistributionBoard is NOT defined");
    return march4cpp::MarchRobot(jointList, ifName, ecatCycleTime);
  }
}

march4cpp::MarchRobot HardwareBuilder::createMarchRobot()
{
  ROS_ASSERT_MSG(this->robotConfig.Type() != YAML::NodeType::Null,
                 "Trying to create a MarchRobot without specifying a .yaml "
                 "file. Please do so in the constructor of "
                 "the HardwareBuilder or in the function createMarchRobot");
  return this->createMarchRobot(robotConfig);
}

march4cpp::Joint HardwareBuilder::createJoint(YAML::Node jointConfig, std::string jointName)
{
  ROS_INFO("Starting creation of joint %s", jointName.c_str());
  this->validateRequiredKeysExist(jointConfig, this->JOINT_REQUIRED_KEYS, "joint");

  march4cpp::IMotionCube imc;
  march4cpp::TemperatureGES temperatureGes;

  bool hasIMotionCube = false;
  bool hasTemperatureGes = false;
  bool hasNetNumber = false;
  int netNumber = -1;
  bool allowActuation = jointConfig["allowActuation"].as<bool>();

  if (jointConfig["imotioncube"].Type() == YAML::NodeType::Undefined)
  {
    ROS_WARN("Joint %s does not have a configuration for an IMotionCube", jointName.c_str());
  }
  else
  {
    hasIMotionCube = true;
    imc = this->createIMotionCube(jointConfig["imotioncube"]);
  }

  if (jointConfig["netNumber"].Type() == YAML::NodeType::Undefined)
  {
    ROS_WARN("Joint %s does not have a netNumber", jointName.c_str());
  }
  else
  {
    hasNetNumber = true;
    netNumber = jointConfig["netNumber"].as<int>();
  }

  if (jointConfig["temperatureges"].Type() == YAML::NodeType::Undefined)
  {
    ROS_WARN("Joint %s does not have a configuration for a TemperatureGes", jointName.c_str());
  }
  else
  {
    hasTemperatureGes = true;
    temperatureGes = this->createTemperatureGES(jointConfig["temperatureges"]);
  }

  ROS_ASSERT_MSG(hasIMotionCube || hasTemperatureGes, "Joint %s has no IMotionCube and no TemperatureGES. Please "
                                                      "check its purpose.",
                 jointName.c_str());
  if (hasTemperatureGes && hasIMotionCube)
  {
    if (hasNetNumber)
    {
      return march4cpp::Joint(jointName, allowActuation, temperatureGes, imc, netNumber);
    }
    else
    {
      return march4cpp::Joint(jointName, allowActuation, temperatureGes, imc);
    }
  }
  if (hasTemperatureGes)
  {
    return march4cpp::Joint(jointName, allowActuation, temperatureGes);
  }
  if (hasNetNumber)
  {
    return march4cpp::Joint(jointName, allowActuation, imc, netNumber);
  }
  else
  {
    return march4cpp::Joint(jointName, allowActuation, imc);
  }
}

march4cpp::IMotionCube HardwareBuilder::createIMotionCube(YAML::Node iMotionCubeConfig)
{
  this->validateRequiredKeysExist(iMotionCubeConfig, this->IMOTIONCUBE_REQUIRED_KEYS, "imotioncube");

  YAML::Node encoderConfig = iMotionCubeConfig["encoder"];
  int slaveIndex = iMotionCubeConfig["slaveIndex"].as<int>();
  return march4cpp::IMotionCube(slaveIndex, this->createEncoder(encoderConfig));
}

march4cpp::Encoder HardwareBuilder::createEncoder(YAML::Node EncoderConfig)
{
  this->validateRequiredKeysExist(EncoderConfig, this->ENCODER_REQUIRED_KEYS, "encoder");

  int resolution = EncoderConfig["resolution"].as<int>();
  int minPositionIU = EncoderConfig["minPositionIU"].as<int>();
  int maxPositionIU = EncoderConfig["maxPositionIU"].as<int>();
  int zeroPositionIU = EncoderConfig["zeroPositionIU"].as<int>();
  auto safetyMarginRad = EncoderConfig["safetyMarginRad"].as<float>();
  return march4cpp::Encoder(resolution, minPositionIU, maxPositionIU, zeroPositionIU, safetyMarginRad);
}

march4cpp::TemperatureGES HardwareBuilder::createTemperatureGES(YAML::Node temperatureGESConfig)
{
  this->validateRequiredKeysExist(temperatureGESConfig, this->TEMPERATUREGES_REQUIRED_KEYS, "temperatureges");

  int slaveIndex = temperatureGESConfig["slaveIndex"].as<int>();
  int byteOffset = temperatureGESConfig["byteOffset"].as<int>();
  return march4cpp::TemperatureGES(slaveIndex, byteOffset);
}

march4cpp::PowerDistributionBoard HardwareBuilder::createPowerDistributionBoard(YAML::Node powerDistributionBoardConfig)
{
  ROS_INFO("Create power distribution board");
  this->validateRequiredKeysExist(powerDistributionBoardConfig, this->POWER_DISTRIBUTION_BOARD_REQUIRED_KEYS,
                                  "powerdistributionboard");

  ROS_INFO("Keys validated");
  int slaveIndex = powerDistributionBoardConfig["slaveIndex"].as<int>();
  ROS_INFO("slaveIndex retrieved");
  YAML::Node netMonitorByteOffsets = powerDistributionBoardConfig["netMonitorByteOffsets"];
  YAML::Node netDriverByteOffsets = powerDistributionBoardConfig["netDriverByteOffsets"];
  YAML::Node bootShutdownByteOffsets = powerDistributionBoardConfig["bootShutdownOffsets"];

  NetMonitorOffsets netMonitorOffsets = NetMonitorOffsets(
      netMonitorByteOffsets["powerDistributionBoardCurrent"].as<int>(),
      netMonitorByteOffsets["lowVoltageNet1Current"].as<int>(),
      netMonitorByteOffsets["lowVoltageNet2Current"].as<int>(),
      netMonitorByteOffsets["highVoltageNetCurrent"].as<int>(), netMonitorByteOffsets["lowVoltageState"].as<int>(),
      netMonitorByteOffsets["highVoltageOvercurrentTrigger"].as<int>(),
      netMonitorByteOffsets["highVoltageEnabled"].as<int>(), netMonitorByteOffsets["highVoltageState"].as<int>());

  NetDriverOffsets netDriverOffsets = NetDriverOffsets(netDriverByteOffsets["lowVoltageNetOnOff"].as<int>(),
                                                       netDriverByteOffsets["highVoltageNetOnOff"].as<int>(),
                                                       netDriverByteOffsets["allHighVoltageOnOff"].as<int>());

  BootShutdownOffsets bootShutdownOffsets =
      BootShutdownOffsets(bootShutdownByteOffsets["masterOk"].as<int>(), bootShutdownByteOffsets["shutdown"].as<int>(),
                          bootShutdownByteOffsets["shutdownAllowed"].as<int>());

  ROS_INFO("Returning PowerDistributionBoard");
  return march4cpp::PowerDistributionBoard(slaveIndex, netMonitorOffsets, netDriverOffsets, bootShutdownOffsets);
}

void HardwareBuilder::validateRequiredKeysExist(YAML::Node config, std::vector<std::string> keyList,
                                                const std::string& objectName)
{
  for (std::vector<std::string>::size_type i = 0; i != keyList.size(); i++)
  {
    if (config[keyList.at(i)].Type() == YAML::NodeType::Undefined)
    {
      throw MissingKeyException(keyList.at(i), objectName);
    }
  }
}
