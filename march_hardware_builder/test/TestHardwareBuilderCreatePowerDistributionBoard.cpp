// Copyright 2019 Project March.

#include "gtest/gtest.h"
#include "ros/ros.h"
#include <gmock/gmock.h>
#include <ros/package.h>
#include <march_hardware_builder/HardwareConfigExceptions.h>
#include <march_hardware_builder/HardwareBuilder.h>

using ::testing::Return;
using ::testing::AtLeast;

class PowerDistributionBoardTest : public ::testing::Test
{
protected:
  std::string base_path;
  HardwareBuilder hardwareBuilder;

  void SetUp() override
  {
    base_path = ros::package::getPath("march_hardware_builder").append("/test/yaml/powerdistributionboard");
  }

  std::string fullPath(const std::string& relativePath)
  {
    return this->base_path.append(relativePath);
  }
};

// TEST_F(PowerDistributionBoardTest, ValidPowerDistributionBoard)
//{
//  std::string fullPath = this->fullPath("/power_distribution_board.yaml");
//  YAML::Node config = YAML::LoadFile(fullPath);
//
//  march4cpp::PowerDistributionBoard createdPowerDistributionBoard =
//      hardwareBuilder.createPowerDistributionBoard(config);
//  march4cpp::PowerDistributionBoard powerDistributionBoard = march4cpp::PowerDistributionBoard(1, 0, 5);
//
//  ASSERT_EQ(powerDistributionBoard, createdPowerDistributionBoard);
//}
//
// TEST_F(PowerDistributionBoardTest, MissingSlaveIndexPowerDistributionBoard)
//{
//  std::string fullPath = this->fullPath("/power_distribution_board_missing_slave_index.yaml");
//  YAML::Node config = YAML::LoadFile(fullPath);
//  ASSERT_THROW(hardwareBuilder.createPowerDistributionBoard(config), MissingKeyException);
//}
