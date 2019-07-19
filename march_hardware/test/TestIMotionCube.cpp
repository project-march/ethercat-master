// Copyright 2018 Project March.

#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include "march_hardware/IMotionCube.h"
#include "mocks/MockEncoder.cpp"

using ::testing::Return;
using ::testing::AtLeast;

class IMotionCubeTest : public ::testing::Test
{
protected:
  march4cpp::Encoder encoder;
  march4cpp::ActuationMode actuationmode;

  void SetUp() override
  {
    encoder = march4cpp::Encoder();
    actuationmode = march4cpp::ActuationMode();
  }
};

class IMotionCubeDeathTest : public IMotionCubeTest
{
};

TEST_F(IMotionCubeTest, SlaveIndexOne)
{
  march4cpp::IMotionCube imc = march4cpp::IMotionCube(1, encoder);
  ASSERT_EQ(1, imc.getSlaveIndex());
}

TEST_F(IMotionCubeDeathTest, SlaveIndexZero)
{
  ASSERT_DEATH(march4cpp::IMotionCube(0, encoder), "Slave configuration error: slaveindex 0 can not be smaller than "
                                                   "1.");
}

TEST_F(IMotionCubeTest, SlaveIndexMinusOne)
{
  ASSERT_DEATH(march4cpp::IMotionCube(-1, encoder), "Slave configuration error: slaveindex -1 can not be smaller than "
                                                    "1.");
}

TEST_F(IMotionCubeTest, NoSlaveIndexConstructor)
{
  ASSERT_NO_THROW(march4cpp::IMotionCube imc = march4cpp::IMotionCube());
}
TEST_F(IMotionCubeTest, NoSlaveIndexConstructorGetIndex)
{
  march4cpp::IMotionCube imc = march4cpp::IMotionCube();
  ASSERT_EQ(-1, imc.getSlaveIndex());
}

TEST_F(IMotionCubeTest, NoActuationMode)
{
    march4cpp::IMotionCube imc = march4cpp::IMotionCube(1, encoder);
    ASSERT_DEATH(imc.actuateRad(1), "trying to actuate rad, while actuationmode = unknown");
}

//TEST_F(IMotionCubeTest, ActuationModeTorqueActuateRad)
//{
//  actuationmode
//  march4cpp::IMotionCube imc = march4cpp::IMotionCube(1, encoder, actuationmode);
//  ASSERT_DEATH(imc.actuateRad(1), "trying to actuate rad, while actuationmode = unknown");
//}

TEST_F(IMotionCubeTest, ActuationModeTorqueImotionCubeActuateRad)
{
    march4cpp::Encoder actualEncoder = march4cpp::Encoder(20, 3, 40000, 5, 0.05);
    march4cpp::IMotionCube imc = march4cpp::IMotionCube(10, actualEncoder);
    ASSERT_DEATH(imc.actuateRad(1), "trying to actuate rad, while actuationmode = unknown");
}

TEST_F(IMotionCubeTest, ActuationModePositionJointActuateTorque)
{
    march4cpp::Encoder actualEncoder = march4cpp::Encoder(20, 3, 40000, 5, 0.05);
    march4cpp::IMotionCube imc = march4cpp::IMotionCube(10, actualEncoder);
    ASSERT_DEATH(imc.actuateTorque(1), "trying to actuate torque, while actuationmode = unknown");
}