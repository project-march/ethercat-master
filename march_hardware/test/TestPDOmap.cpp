// Copyright 2018 Project March.

#include "gtest/gtest.h"
#include <gmock/gmock.h>

#include <march_hardware/PDOmap.h>
#include <march_hardware/MarchRobot.h>

class PDOTest : public ::testing::Test
{
protected:
};

TEST_F(PDOTest, sortPDOmap)
{
  march::PDOmap pdoMapMISO = march::PDOmap();
  pdoMapMISO.addObject(march::IMCObjectName::StatusWord);
  pdoMapMISO.addObject(march::IMCObjectName::ActualPosition);
  std::unordered_map<march::IMCObjectName, uint8_t> misoByteOffsets = pdoMapMISO.map(1, march::DataDirection::MISO);

  ASSERT_EQ(0u, misoByteOffsets[march::IMCObjectName::ActualPosition]);
  ASSERT_EQ(4u, misoByteOffsets[march::IMCObjectName::StatusWord]);
}

TEST_F(PDOTest, InvalidDataDirection)
{
  march::PDOmap map;
  std::unordered_map<march::IMCObjectName, uint8_t> expected;
  ASSERT_EQ(map.map(1, (march::DataDirection)7), expected);
}

TEST_F(PDOTest, multipleAddObjects)
{
  march::PDOmap pdoMapMISO = march::PDOmap();

  pdoMapMISO.addObject(march::IMCObjectName::ActualPosition);
  pdoMapMISO.addObject(march::IMCObjectName::StatusWord);
  pdoMapMISO.addObject(march::IMCObjectName::StatusWord);
  std::unordered_map<march::IMCObjectName, uint8_t> misoByteOffsets = pdoMapMISO.map(1, march::DataDirection::MISO);
  ASSERT_EQ(2u, misoByteOffsets.size());
}

TEST_F(PDOTest, ObjectCounts)
{
  march::PDOmap pdoMapMISO = march::PDOmap();
  pdoMapMISO.addObject(march::IMCObjectName::CurrentLimit);
  std::unordered_map<march::IMCObjectName, uint8_t> misoByteOffsets = pdoMapMISO.map(1, march::DataDirection::MISO);
  ASSERT_EQ(1u, misoByteOffsets.count(march::IMCObjectName::CurrentLimit));
  ASSERT_EQ(0u, misoByteOffsets.count(march::IMCObjectName::DCLinkVoltage));
}
