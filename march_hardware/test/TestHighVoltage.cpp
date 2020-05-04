// Copyright 2018 Project March.
#include "mocks/MockPdoInterface.h"
#include "march_hardware/HighVoltage.h"

#include <gtest/gtest.h>
#include <sstream>

class TestHighVoltage : public ::testing::Test
{
protected:
  MockPdoInterface mock_pdo;
  march::PdoSlaveInterface pdo = march::PdoSlaveInterface(1, this->mock_pdo);
};

TEST_F(TestHighVoltage, Equals)
{
  NetMonitorOffsets netMonitoringOffsets;
  NetDriverOffsets netDriverOffsets;
  march::HighVoltage highVoltage1(this->pdo, netMonitoringOffsets, netDriverOffsets);
  march::HighVoltage highVoltage2(this->pdo, netMonitoringOffsets, netDriverOffsets);
  EXPECT_TRUE(highVoltage1 == highVoltage2);
}

TEST_F(TestHighVoltage, UnEqual)
{
  NetMonitorOffsets netMonitoringOffsets;
  NetDriverOffsets netDriverOffsets1;
  NetDriverOffsets netDriverOffsets2(1, 2, 3);
  march::HighVoltage highVoltage1(this->pdo, netMonitoringOffsets, netDriverOffsets1);
  march::HighVoltage highVoltage2(this->pdo, netMonitoringOffsets, netDriverOffsets2);
  EXPECT_FALSE(highVoltage1 == highVoltage2);
}

TEST_F(TestHighVoltage, Stream)
{
  NetMonitorOffsets netMonitoringOffsets;
  NetDriverOffsets netDriverOffsets;
  march::HighVoltage highVoltage(this->pdo, netMonitoringOffsets, netDriverOffsets);
  std::stringstream ss;
  ss << highVoltage;
  EXPECT_EQ("HighVoltage(netMonitoringOffsets: NetMonitorOffsets(powerDistributionBoardCurrent: -1, "
            "lowVoltageNet1Current: -1, lowVoltageNet2Current: -1, highVoltageNetCurrent: -1, lowVoltageState: -1, "
            "highVoltageOvercurrentTrigger: -1, highVoltageEnabled: -1, highVoltageState: -1), netDriverOffsets: "
            "NetDriverOffsets(lowVoltageNetOnOff: -1, highVoltageNetOnOff: -1, highVoltageNetEnableDisable: -1))",
            ss.str());
}
