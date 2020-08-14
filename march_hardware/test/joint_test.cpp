// Copyright 2018 Project March.
#include "mocks/mock_temperature_ges.h"
#include "mocks/mock_motor_controller.h"
#include "march_hardware/error/hardware_exception.h"
#include "march_hardware/joint.h"

#include <memory>
#include <utility>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using testing::_;
using testing::Eq;
using testing::Return;

class JointTest : public testing::Test
{
protected:
  void SetUp() override
  {
    this->motor_controller = std::make_unique<MockMotorController>();
    this->temperature_ges = std::make_unique<MockTemperatureGES>();
  }

  std::unique_ptr<MockMotorController> motor_controller;
  std::unique_ptr<MockTemperatureGES> temperature_ges;
};

TEST_F(JointTest, InitializeWithoutTemperatureGes)
{
  const int expected_cycle = 3;
  EXPECT_CALL(*this->motor_controller, initialize(expected_cycle)).Times(1);

  march::Joint joint("test", 0, false, std::move(this->motor_controller));
  ASSERT_NO_THROW(joint.initialize(expected_cycle));
}

TEST_F(JointTest, AllowActuation)
{
  march::Joint joint("test", 0, true, std::move(this->motor_controller));
  ASSERT_TRUE(joint.canActuate());
}

TEST_F(JointTest, DisableActuation)
{
  march::Joint joint("test", 0, false, std::move(this->motor_controller));
  ASSERT_FALSE(joint.canActuate());
}

TEST_F(JointTest, SetAllowActuation)
{
  march::Joint joint("test", 0, false, std::move(this->motor_controller));
  ASSERT_FALSE(joint.canActuate());
  joint.setAllowActuation(true);
  ASSERT_TRUE(joint.canActuate());
}

TEST_F(JointTest, GetName)
{
  const std::string expected_name = "test";
  march::Joint joint(expected_name, 0, false, std::move(this->motor_controller));
  ASSERT_EQ(expected_name, joint.getName());
}

TEST_F(JointTest, GetNetNumber)
{
  const int expected_net_number = 2;
  march::Joint joint("test", expected_net_number, false, std::move(this->motor_controller));
  ASSERT_EQ(expected_net_number, joint.getNetNumber());
}

TEST_F(JointTest, ActuatePositionDisableActuation)
{
  march::Joint joint("actuate_false", 0, false, std::move(this->motor_controller));
  EXPECT_FALSE(joint.canActuate());
  ASSERT_THROW(joint.actuateRad(0.3), march::error::HardwareException);
}

TEST_F(JointTest, ActuatePosition)
{
  const double expected_rad = 5;
  EXPECT_CALL(*this->motor_controller, actuateRad(Eq(expected_rad))).Times(1);

  march::Joint joint("actuate_false", 0, true, std::move(this->motor_controller));
  ASSERT_NO_THROW(joint.actuateRad(expected_rad));
}

TEST_F(JointTest, ActuateTorqueDisableActuation)
{
  march::Joint joint("actuate_false", 0, false, std::move(this->motor_controller));
  EXPECT_FALSE(joint.canActuate());
  ASSERT_THROW(joint.actuateTorque(3), march::error::HardwareException);
}

TEST_F(JointTest, ActuateTorque)
{
  const int16_t expected_torque = 5;
  EXPECT_CALL(*this->motor_controller, actuateTorque(Eq(expected_torque))).Times(1);

  march::Joint joint("actuate_false", 0, true, std::move(this->motor_controller));
  ASSERT_NO_THROW(joint.actuateTorque(expected_torque));
}

TEST_F(JointTest, PrepareForActuationNotAllowed)
{
  march::Joint joint("actuate_false", 0, false, std::move(this->motor_controller));
  ASSERT_THROW(joint.prepareActuation(), march::error::HardwareException);
}

TEST_F(JointTest, PrepareForActuationAllowed)
{
  EXPECT_CALL(*this->motor_controller, prepareActuation()).Times(1);
  march::Joint joint("actuate_true", 0, true, std::move(this->motor_controller));
  ASSERT_NO_THROW(joint.prepareActuation());
}

TEST_F(JointTest, GetTemperature)
{
  const float expected_temperature = 45.0;
  EXPECT_CALL(*this->temperature_ges, getTemperature()).WillOnce(Return(expected_temperature));

  march::Joint joint("get_temperature", 0, false, nullptr, std::move(this->temperature_ges));
  ASSERT_FLOAT_EQ(joint.getTemperature(), expected_temperature);
}

TEST_F(JointTest, GetTemperatureWithoutTemperatureGes)
{
  march::Joint joint("get_temperature", 0, false, nullptr, nullptr);
  ASSERT_FLOAT_EQ(joint.getTemperature(), -1.0);
}

TEST_F(JointTest, ResetController)
{
  EXPECT_CALL(*this->motor_controller, reset()).Times(1);
  march::Joint joint("reset_controller", 0, true, std::move(this->motor_controller));
  ASSERT_NO_THROW(joint.resetMotorController());
}

TEST_F(JointTest, TestPrepareActuation)
{
  EXPECT_CALL(*this->motor_controller, getAngleRadIncremental()).WillOnce(Return(5));
  EXPECT_CALL(*this->motor_controller, getAngleRadAbsolute()).WillOnce(Return(3));
  EXPECT_CALL(*this->motor_controller, prepareActuation()).Times(1);
  march::Joint joint("actuate_true", 0, true, std::move(this->motor_controller));
  joint.prepareActuation();
  ASSERT_EQ(joint.getIncrementalPosition(), 5);
  ASSERT_EQ(joint.getAbsolutePosition(), 3);
}

TEST_F(JointTest, TestReceivedDataUpdateFirstTimeTrue)
{
  EXPECT_CALL(*this->motor_controller, getMotorControllerVoltage()).WillOnce(Return(48));
  march::Joint joint("actuate_true", 0, true, std::move(this->motor_controller));
  ASSERT_TRUE(joint.receivedDataUpdate());
}

TEST_F(JointTest, TestReceivedDataUpdateTrue)
{
  EXPECT_CALL(*this->motor_controller, getMotorControllerVoltage()).WillOnce(Return(48)).WillOnce(Return(48.001));
  march::Joint joint("actuate_true", 0, true, std::move(this->motor_controller));
  joint.receivedDataUpdate();
  ASSERT_TRUE(joint.receivedDataUpdate());
}

TEST_F(JointTest, TestReceivedDataUpdateFalse)
{
  EXPECT_CALL(*this->motor_controller, getMotorControllerVoltage()).WillRepeatedly(Return(48));
  march::Joint joint("actuate_true", 0, true, std::move(this->motor_controller));
  joint.receivedDataUpdate();
  ASSERT_FALSE(joint.receivedDataUpdate());
}

TEST_F(JointTest, TestReadEncodersOnce)
{
  ros::Duration elapsed_time(0.2);
  double velocity = 0.5;
  double velocity_with_noise = velocity - 2 / elapsed_time.toSec();

  double initial_incremental_position = 5;
  double initial_absolute_position = 3;

  double new_incremental_position = initial_incremental_position + velocity * elapsed_time.toSec();
  double new_absolute_position = initial_absolute_position + velocity_with_noise * elapsed_time.toSec();

  EXPECT_CALL(*this->motor_controller, getMotorControllerVoltage()).WillOnce(Return(48));
  EXPECT_CALL(*this->motor_controller, getIncrementalMorePrecise()).WillOnce(Return(true));
  EXPECT_CALL(*this->motor_controller, getMotorCurrent()).WillOnce(Return(5));
  EXPECT_CALL(*this->motor_controller, getAngleRadIncremental())
      .WillOnce(Return(initial_incremental_position))
      .WillOnce(Return(new_incremental_position))
      .WillOnce(Return(new_incremental_position));
  EXPECT_CALL(*this->motor_controller, getAngleRadAbsolute())
      .WillOnce(Return(initial_absolute_position))
      .WillOnce(Return(new_absolute_position))
      .WillOnce(Return(new_absolute_position));

  EXPECT_CALL(*this->motor_controller, getVelocityRadIncremental())
      .WillOnce(Return(velocity))
      .WillOnce(Return(velocity));
  EXPECT_CALL(*this->motor_controller, getVelocityRadAbsolute()).WillOnce(Return(velocity_with_noise));

  march::Joint joint("actuate_true", 0, true, std::move(this->motor_controller));
  joint.prepareActuation();

  joint.readEncoders(elapsed_time);

  ASSERT_DOUBLE_EQ(joint.getPosition(), initial_absolute_position + velocity * elapsed_time.toSec());
  ASSERT_NEAR(joint.getVelocity(), (new_incremental_position - initial_incremental_position) / elapsed_time.toSec(),
              0.0000001);
}

TEST_F(JointTest, TestReadEncodersTwice)
{
  ros::Duration elapsed_time(0.2);
  double first_velocity = 0.5;
  double second_velocity = 0.8;

  double absolute_noise = -1;
  double first_velocity_with_noise = first_velocity + absolute_noise / elapsed_time.toSec();
  double second_velocity_with_noise = second_velocity + absolute_noise / elapsed_time.toSec();

  double initial_incremental_position = 5;
  double initial_absolute_position = 3;
  double second_incremental_position = initial_incremental_position + first_velocity * elapsed_time.toSec();
  double second_absolute_position = initial_absolute_position + first_velocity * elapsed_time.toSec();
  double third_incremental_position = second_incremental_position + second_velocity * elapsed_time.toSec();
  double third_absolute_position = second_absolute_position + second_velocity_with_noise * elapsed_time.toSec();

  EXPECT_CALL(*this->motor_controller, getMotorControllerVoltage()).WillOnce(Return(48)).WillOnce(Return(48.01));
  EXPECT_CALL(*this->motor_controller, getIncrementalMorePrecise()).WillRepeatedly(Return(true));
  EXPECT_CALL(*this->motor_controller, getAngleRadIncremental())
      .WillOnce(Return(initial_incremental_position))
      .WillOnce(Return(second_incremental_position))
      .WillOnce(Return(second_incremental_position))
      .WillOnce(Return(third_incremental_position))
      .WillOnce(Return(third_incremental_position));
  EXPECT_CALL(*this->motor_controller, getAngleRadAbsolute())
      .WillOnce(Return(initial_absolute_position))
      .WillOnce(Return(second_absolute_position))
      .WillOnce(Return(second_absolute_position))
      .WillOnce(Return(third_absolute_position))
      .WillOnce(Return(third_absolute_position));
  EXPECT_CALL(*this->motor_controller, getVelocityRadIncremental())
      .WillOnce(Return(first_velocity))
      .WillOnce(Return(first_velocity))
      .WillOnce(Return(second_velocity))
      .WillOnce(Return(second_velocity));
  EXPECT_CALL(*this->motor_controller, getVelocityRadAbsolute())
      .WillOnce(Return(first_velocity_with_noise))
      .WillOnce(Return(second_velocity_with_noise));

  march::Joint joint("actuate_true", 0, true, std::move(this->motor_controller));
  joint.prepareActuation();

  joint.readEncoders(elapsed_time);
  joint.readEncoders(elapsed_time);

  ASSERT_DOUBLE_EQ(joint.getPosition(),
                   initial_absolute_position + (first_velocity + second_velocity) * elapsed_time.toSec());
  ASSERT_NEAR(joint.getVelocity(), (third_incremental_position - second_incremental_position) / elapsed_time.toSec(),
              0.0000001);
}

TEST_F(JointTest, TestReadEncodersNoUpdate)
{
  ros::Duration elapsed_time(0.2);

  double velocity = 0.5;

  double absolute_noise = -1;

  double initial_incremental_position = 5;
  double initial_absolute_position = 3;
  double second_incremental_position = initial_incremental_position + velocity * elapsed_time.toSec();
  double second_absolute_position = initial_absolute_position + velocity * elapsed_time.toSec() + absolute_noise;

  EXPECT_CALL(*this->motor_controller, getMotorControllerVoltage()).WillRepeatedly(Return(48));
  EXPECT_CALL(*this->motor_controller, getIncrementalMorePrecise()).WillRepeatedly(Return(true));
  EXPECT_CALL(*this->motor_controller, getAngleRadIncremental())
      .WillOnce(Return(initial_incremental_position))
      .WillRepeatedly(Return(second_incremental_position));
  EXPECT_CALL(*this->motor_controller, getAngleRadAbsolute())
      .WillOnce(Return(initial_absolute_position))
      .WillRepeatedly(Return(second_absolute_position));
  EXPECT_CALL(*this->motor_controller, getVelocityRadIncremental()).WillRepeatedly(Return(velocity));
  EXPECT_CALL(*this->motor_controller, getVelocityRadAbsolute()).WillRepeatedly(Return(velocity));

  march::Joint joint("actuate_true", 0, true, std::move(this->motor_controller));
  joint.prepareActuation();

  joint.readEncoders(elapsed_time);
  joint.readEncoders(elapsed_time);

  ASSERT_DOUBLE_EQ(joint.getPosition(), initial_absolute_position + 2 * velocity * elapsed_time.toSec());
  ASSERT_DOUBLE_EQ(joint.getVelocity(), velocity);
}
