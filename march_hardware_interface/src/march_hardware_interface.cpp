// Copyright 2019 Project March.
#include "march_hardware_interface/march_hardware_interface.h"
#include "march_hardware_interface/power_net_on_off_command.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <memory>
#include <sstream>
#include <string>

#include <joint_limits_interface/joint_limits.h>
#include <joint_limits_interface/joint_limits_interface.h>
#include <joint_limits_interface/joint_limits_urdf.h>
#include <urdf/model.h>

#include <march_hardware/motor_controller/actuation_mode.h>
#include <march_hardware/motor_controller/motor_controller_states.h>
#include <march_hardware/joint.h>

using hardware_interface::JointHandle;
using hardware_interface::JointStateHandle;
using hardware_interface::PositionJointInterface;
using joint_limits_interface::EffortJointSoftLimitsHandle;
using joint_limits_interface::JointLimits;
using joint_limits_interface::PositionJointSoftLimitsHandle;
using joint_limits_interface::SoftJointLimits;

MarchHardwareInterface::MarchHardwareInterface(std::unique_ptr<march::MarchRobot> robot, bool reset_motor_controllers)
  : march_robot_(std::move(robot))
  , num_joints_(this->march_robot_->size())
  , reset_motor_controllers_(reset_motor_controllers)
{
}

bool MarchHardwareInterface::init(ros::NodeHandle& nh, ros::NodeHandle& /* robot_hw_nh */)
{
  // Initialize realtime publisher for the motor controller states
  this->motor_controller_state_pub_ =
      std::make_unique<realtime_tools::RealtimePublisher<march_shared_resources::MotorControllerState>>(
          nh, "/march/motor_controller_states/", 4);

  this->after_limit_joint_command_pub_ =
      std::make_unique<realtime_tools::RealtimePublisher<march_shared_resources::AfterLimitJointCommand>>(
          nh, "/march/controller/after_limit_joint_command/", 4);

  this->uploadJointNames(nh);

  this->reserveMemory();

  // Start communication cycle in the hardware
  this->march_robot_->startCommunication(this->reset_motor_controllers_);

  for (size_t i = 0; i < num_joints_; ++i)
  {
    const std::string name = this->march_robot_->getJoint(i).getName();

    SoftJointLimits soft_limits;
    SoftJointLimits soft_limits_error;

    getSoftJointLimits(this->march_robot_->getUrdf().getJoint(name), soft_limits);
    getSoftJointLimitsError(name, this->march_robot_->getUrdf().getJoint(name), soft_limits_error);

    ROS_DEBUG("[%s] ROS soft limits set to (%f, %f) and error limits set to (%f, %f)", name.c_str(),
              soft_limits.min_position, soft_limits.max_position, soft_limits_error.min_position,
              soft_limits_error.max_position);

    soft_limits_[i] = soft_limits;
    soft_limits_error_[i] = soft_limits_error;
  }

  if (this->march_robot_->hasPowerDistributionboard())
  {
    // Create march_pdb_state interface
    MarchPdbStateHandle march_pdb_state_handle("PDBhandle", this->march_robot_->getPowerDistributionBoard(),
                                               &master_shutdown_allowed_command_, &enable_high_voltage_command_,
                                               &power_net_on_off_command_);
    march_pdb_interface_.registerHandle(march_pdb_state_handle);

    for (const auto& joint : *this->march_robot_)
    {
      const int net_number = joint.getNetNumber();
      if (net_number == -1)
      {
        std::ostringstream error_stream;
        error_stream << "Joint " << joint.getName() << " has no net number";
        throw std::runtime_error(error_stream.str());
      }
      while (!march_robot_->getPowerDistributionBoard()->getHighVoltage().getNetOperational(net_number))
      {
        march_robot_->getPowerDistributionBoard()->getHighVoltage().setNetOnOff(true, net_number);
        usleep(100000);
        ROS_WARN("[%s] Waiting on high voltage", joint.getName().c_str());
      }
    }

    this->registerInterface(&this->march_pdb_interface_);
  }
  else
  {
    ROS_WARN("Running without Power Distribution Board");
  }

  // Initialize interfaces for each joint
  for (size_t i = 0; i < num_joints_; ++i)
  {
    march::Joint& joint = march_robot_->getJoint(i);

    // Create joint state interface
    JointStateHandle joint_state_handle(joint.getName(), &joint_position_[i], &joint_velocity_[i], &joint_effort_[i]);
    joint_state_interface_.registerHandle(joint_state_handle);

    // Retrieve joint (soft) limits from the urdf
    JointLimits limits;
    getJointLimits(this->march_robot_->getUrdf().getJoint(joint.getName()), limits);

    if (joint.getActuationMode() == march::ActuationMode::position)
    {
      // Create position joint interface
      JointHandle joint_position_handle(joint_state_handle, &joint_position_command_[i]);
      position_joint_interface_.registerHandle(joint_position_handle);

      // Create joint limit interface
      PositionJointSoftLimitsHandle joint_limits_handle(joint_position_handle, limits, soft_limits_[i]);
      position_joint_soft_limits_interface_.registerHandle(joint_limits_handle);
    }
    else if (joint.getActuationMode() == march::ActuationMode::torque)
    {
      // Create effort joint interface
      JointHandle joint_effort_handle_(joint_state_handle, &joint_effort_command_[i]);
      effort_joint_interface_.registerHandle(joint_effort_handle_);

      // Create joint effort limit interface
      EffortJointSoftLimitsHandle joint_effort_limits_handle(joint_effort_handle_, limits, soft_limits_[i]);
      effort_joint_soft_limits_interface_.registerHandle(joint_effort_limits_handle);
    }

    // Create velocity joint interface
    JointHandle joint_velocity_handle(joint_state_handle, &joint_velocity_command_[i]);
    velocity_joint_interface_.registerHandle(joint_velocity_handle);

    // Create march_state interface
    MarchTemperatureSensorHandle temperature_sensor_handle(joint.getName(), &joint_temperature_[i],
                                                           &joint_temperature_variance_[i]);
    march_temperature_interface_.registerHandle(temperature_sensor_handle);

    // Prepare Motor Controllers for actuation
    if (joint.canActuate())
    {
      joint.prepareActuation();

      // Set the first target as the current position
      joint_position_[i] = joint.getPosition();
      joint_velocity_[i] = joint.getVelocity();
      joint_effort_[i] = 0;

      if (joint.getActuationMode() == march::ActuationMode::position)
      {
        joint_position_command_[i] = joint_position_[i];
      }
      else if (joint.getActuationMode() == march::ActuationMode::torque)
      {
        joint_effort_command_[i] = 0;
      }
    }
  }
  ROS_INFO("Successfully actuated all joints");

  this->registerInterface(&this->march_temperature_interface_);
  this->registerInterface(&this->joint_state_interface_);
  this->registerInterface(&this->position_joint_interface_);
  this->registerInterface(&this->effort_joint_interface_);
  this->registerInterface(&this->position_joint_soft_limits_interface_);
  this->registerInterface(&this->effort_joint_soft_limits_interface_);

  return true;
}

void MarchHardwareInterface::validate()
{
  const auto last_exception = this->march_robot_->getLastCommunicationException();
  if (last_exception)
  {
    std::rethrow_exception(last_exception);
  }

  bool fault_state = false;
  for (size_t i = 0; i < num_joints_; i++)
  {
    this->outsideLimitsCheck(i);
    if (!this->motorControllerStateCheck(i))
    {
      fault_state = true;
    }
  }
  if (fault_state)
  {
    this->march_robot_->stopCommunication();
    throw std::runtime_error("One or more motor controllers are in fault state");
  }
}

void MarchHardwareInterface::waitForUpdate()
{
  this->march_robot_->waitForUpdate();
}

void MarchHardwareInterface::read(const ros::Time& /* time */, const ros::Duration& elapsed_time)
{
  for (size_t i = 0; i < num_joints_; i++)
  {
    march::Joint& joint = march_robot_->getJoint(i);

    // Update position with the most accurate velocity
    joint.readEncoders(elapsed_time);
    joint_position_[i] = joint.getPosition();
    joint_velocity_[i] = joint.getVelocity();

    if (joint.hasTemperatureGES())
    {
      joint_temperature_[i] = joint.getTemperature();
    }
    joint_effort_[i] = joint.getTorque();
  }

  this->updateMotorControllerStates();
}

void MarchHardwareInterface::write(const ros::Time& /* time */, const ros::Duration& elapsed_time)
{
  for (size_t i = 0; i < num_joints_; i++)
  {
    // Enlarge joint_effort_command because ROS control limits the pid values to a certain maximum
    joint_effort_command_[i] = joint_effort_command_[i];
    if (std::abs(joint_last_effort_command_[i] - joint_effort_command_[i]) > MAX_EFFORT_CHANGE)
    {
      joint_effort_command_[i] =
          joint_last_effort_command_[i] +
          std::copysign(MAX_EFFORT_CHANGE, joint_effort_command_[i] - joint_last_effort_command_[i]);
    }
    has_actuated_ |= (joint_effort_command_[i] != 0);
  }

  // Enforce limits on all joints in effort mode
  effort_joint_soft_limits_interface_.enforceLimits(elapsed_time);

  if (not has_actuated_)
  {
    bool found_non_zero = false;
    for (size_t i = 0; i < num_joints_; i++)
    {
      if (joint_effort_command_[i] != 0)
      {
        ROS_ERROR("Non-zero effort on first actuation for joint %s", march_robot_->getJoint(i).getName().c_str());
        found_non_zero = true;
      }
    }
    if (found_non_zero)
    {
      throw std::runtime_error("Safety limits acted before actual controller started actuating");
    }
  }
  // Enforce limits on all joints in position mode
  position_joint_soft_limits_interface_.enforceLimits(elapsed_time);

  for (size_t i = 0; i < num_joints_; i++)
  {
    march::Joint& joint = march_robot_->getJoint(i);

    if (joint.canActuate())
    {
      joint_last_effort_command_[i] = joint_effort_command_[i];

      if (joint.getActuationMode() == march::ActuationMode::position)
      {
        joint.actuateRad(joint_position_command_[i]);
      }
      else if (joint.getActuationMode() == march::ActuationMode::torque)
      {
        joint.actuateTorque(joint_effort_command_[i]);
      }
    }
  }

  this->updateAfterLimitJointCommand();

  if (this->march_robot_->hasPowerDistributionboard())
  {
    updatePowerDistributionBoard();
  }
}

int MarchHardwareInterface::getCycleTime() const
{
  return this->march_robot_->getCycleTime();
}

void MarchHardwareInterface::uploadJointNames(ros::NodeHandle& nh) const
{
  std::vector<std::string> joint_names;
  for (const auto& joint : *this->march_robot_)
  {
    joint_names.push_back(joint.getName());
  }
  std::sort(joint_names.begin(), joint_names.end());
  nh.setParam("/march/joint_names", joint_names);
}

void MarchHardwareInterface::reserveMemory()
{
  joint_position_.resize(num_joints_);
  joint_position_command_.resize(num_joints_);
  joint_velocity_.resize(num_joints_);
  joint_velocity_command_.resize(num_joints_);
  joint_effort_.resize(num_joints_);
  joint_effort_command_.resize(num_joints_);
  joint_last_effort_command_.resize(num_joints_);
  joint_temperature_.resize(num_joints_);
  joint_temperature_variance_.resize(num_joints_);
  soft_limits_.resize(num_joints_);
  soft_limits_error_.resize(num_joints_);

  after_limit_joint_command_pub_->msg_.name.resize(num_joints_);
  after_limit_joint_command_pub_->msg_.position_command.resize(num_joints_);
  after_limit_joint_command_pub_->msg_.effort_command.resize(num_joints_);

  motor_controller_state_pub_->msg_.joint_names.resize(num_joints_);
  motor_controller_state_pub_->msg_.motor_current.resize(num_joints_);
  motor_controller_state_pub_->msg_.controller_voltage.resize(num_joints_);
  motor_controller_state_pub_->msg_.motor_voltage.resize(num_joints_);
  motor_controller_state_pub_->msg_.absolute_encoder_value.resize(num_joints_);
  motor_controller_state_pub_->msg_.incremental_encoder_value.resize(num_joints_);
  motor_controller_state_pub_->msg_.absolute_velocity.resize(num_joints_);
  motor_controller_state_pub_->msg_.incremental_velocity.resize(num_joints_);
  motor_controller_state_pub_->msg_.error_status.resize(num_joints_);
}

void MarchHardwareInterface::updatePowerDistributionBoard()
{
  march_robot_->getPowerDistributionBoard()->setMasterOnline();
  march_robot_->getPowerDistributionBoard()->setMasterShutDownAllowed(master_shutdown_allowed_command_);
  updateHighVoltageEnable();
  updatePowerNet();
}

void MarchHardwareInterface::updateHighVoltageEnable()
{
  try
  {
    if (march_robot_->getPowerDistributionBoard()->getHighVoltage().getHighVoltageEnabled() !=
        enable_high_voltage_command_)
    {
      march_robot_->getPowerDistributionBoard()->getHighVoltage().enableDisableHighVoltage(
          enable_high_voltage_command_);
    }
    else if (!march_robot_->getPowerDistributionBoard()->getHighVoltage().getHighVoltageEnabled())
    {
      ROS_WARN_THROTTLE(2, "High voltage disabled");
    }
  }
  catch (std::exception& exception)
  {
    ROS_ERROR("%s", exception.what());
    ROS_DEBUG("Reverting the enable_high_voltage_command input, in attempt to prevent this exception is thrown "
              "again");
    enable_high_voltage_command_ = !enable_high_voltage_command_;
  }
}

void MarchHardwareInterface::updatePowerNet()
{
  if (power_net_on_off_command_.getType() == PowerNetType::high_voltage)
  {
    try
    {
      if (march_robot_->getPowerDistributionBoard()->getHighVoltage().getNetOperational(
              power_net_on_off_command_.getNetNumber()) != power_net_on_off_command_.isOnOrOff())
      {
        march_robot_->getPowerDistributionBoard()->getHighVoltage().setNetOnOff(
            power_net_on_off_command_.isOnOrOff(), power_net_on_off_command_.getNetNumber());
      }
    }
    catch (std::exception& exception)
    {
      ROS_ERROR("%s", exception.what());
      ROS_DEBUG("Reset power net command, in attempt to prevent this exception is thrown again");
      power_net_on_off_command_.reset();
    }
  }
  else if (power_net_on_off_command_.getType() == PowerNetType::low_voltage)
  {
    try
    {
      if (march_robot_->getPowerDistributionBoard()->getLowVoltage().getNetOperational(
              power_net_on_off_command_.getNetNumber()) != power_net_on_off_command_.isOnOrOff())
      {
        march_robot_->getPowerDistributionBoard()->getLowVoltage().setNetOnOff(
            power_net_on_off_command_.isOnOrOff(), power_net_on_off_command_.getNetNumber());
      }
    }
    catch (std::exception& exception)
    {
      ROS_ERROR("%s", exception.what());
      ROS_WARN("Reset power net command, in attempt to prevent this exception is thrown again");
      power_net_on_off_command_.reset();
    }
  }
}

void MarchHardwareInterface::updateAfterLimitJointCommand()
{
  if (!after_limit_joint_command_pub_->trylock())
  {
    return;
  }

  after_limit_joint_command_pub_->msg_.header.stamp = ros::Time::now();
  for (size_t i = 0; i < num_joints_; i++)
  {
    march::Joint& joint = march_robot_->getJoint(i);

    after_limit_joint_command_pub_->msg_.name[i] = joint.getName();
    after_limit_joint_command_pub_->msg_.position_command[i] = joint_position_command_[i];
    after_limit_joint_command_pub_->msg_.effort_command[i] = joint_effort_command_[i];
  }

  after_limit_joint_command_pub_->unlockAndPublish();
}

void MarchHardwareInterface::updateMotorControllerStates()
{
  if (!motor_controller_state_pub_->trylock())
  {
    return;
  }

  motor_controller_state_pub_->msg_.header.stamp = ros::Time::now();
  for (size_t i = 0; i < num_joints_; i++)
  {
    march::Joint& joint = march_robot_->getJoint(i);
    march::MotorControllerStates& motor_controller_state = joint.getMotorControllerStates();
    motor_controller_state_pub_->msg_.header.stamp = ros::Time::now();
    motor_controller_state_pub_->msg_.joint_names[i] = joint.getName();
    motor_controller_state_pub_->msg_.motor_current[i] = motor_controller_state.motorCurrent;
    motor_controller_state_pub_->msg_.controller_voltage[i] = motor_controller_state.controllerVoltage;
    motor_controller_state_pub_->msg_.motor_voltage[i] = motor_controller_state.motorVoltage;
    motor_controller_state_pub_->msg_.absolute_encoder_value[i] = motor_controller_state.absoluteEncoderValue;
    motor_controller_state_pub_->msg_.incremental_encoder_value[i] = motor_controller_state.incrementalEncoderValue;
    motor_controller_state_pub_->msg_.absolute_velocity[i] = motor_controller_state.absoluteVelocity;
    motor_controller_state_pub_->msg_.incremental_velocity[i] = motor_controller_state.incrementalVelocity;
    motor_controller_state_pub_->msg_.error_status[i] = motor_controller_state.getErrorStatus();
  }

  motor_controller_state_pub_->unlockAndPublish();
}

bool MarchHardwareInterface::motorControllerStateCheck(size_t joint_index)
{
  march::Joint& joint = march_robot_->getJoint(joint_index);
  march::MotorControllerStates& controller_states = joint.getMotorControllerStates();
  if (!controller_states.checkState())
  {
    std::string error_msg =
        "Motor controller of joint " + joint.getName() + " is in " + controller_states.getErrorStatus();
    throw std::runtime_error(error_msg);
    return false;
  }
  return true;
}

void MarchHardwareInterface::outsideLimitsCheck(size_t joint_index)
{
  march::Joint& joint = march_robot_->getJoint(joint_index);

  if (joint_position_[joint_index] < soft_limits_[joint_index].min_position ||
      joint_position_[joint_index] > soft_limits_[joint_index].max_position)
  {
    if (joint_position_[joint_index] < soft_limits_error_[joint_index].min_position ||
        joint_position_[joint_index] > soft_limits_error_[joint_index].max_position)
    {
      ROS_ERROR_THROTTLE(1, "Joint %s is outside of its error soft limits (%f, %f). Actual position: %f",
                         joint.getName().c_str(), soft_limits_error_[joint_index].min_position,
                         soft_limits_error_[joint_index].max_position, joint_position_[joint_index]);

      if (joint.canActuate())
      {
        std::ostringstream error_stream;
        error_stream << "Joint " << joint.getName() << " is out of its soft limits ("
                     << soft_limits_[joint_index].min_position << ", " << soft_limits_[joint_index].max_position
                     << "). Actual position: " << joint_position_[joint_index];
        throw std::runtime_error(error_stream.str());
      }
    }

    ROS_WARN_THROTTLE(1, "Joint %s is outside of its soft limits (%f, %f). Actual position: %f",
                      joint.getName().c_str(), soft_limits_[joint_index].min_position,
                      soft_limits_[joint_index].max_position, joint_position_[joint_index]);
  }
}

void MarchHardwareInterface::getSoftJointLimitsError(const std::string& name,
                                                     const urdf::JointConstSharedPtr& urdf_joint,
                                                     joint_limits_interface::SoftJointLimits& error_soft_limits)
{
  std::ostringstream param_name;
  std::ostringstream error_stream;

  param_name << "/march/controller/trajectory/constraints/" << name << "/margin_soft_limit_error";

  if (!ros::param::has(param_name.str()))
  {
    error_stream << "Margin soft limits error of joint: " << name << " could not be found";
    throw std::runtime_error(error_stream.str());
  }

  float margin;
  ros::param::param<float>(param_name.str(), margin, 0.0);

  if (!urdf_joint || !urdf_joint->safety || !urdf_joint->limits || margin <= 0.0 || margin > 1.0)
  {
    error_stream << "Could not construct the soft limits for joint: " << name;
    throw std::runtime_error(error_stream.str());
  }

  error_soft_limits.min_position =
      urdf_joint->limits->lower + ((urdf_joint->safety->soft_lower_limit - urdf_joint->limits->lower) * margin);
  error_soft_limits.max_position =
      urdf_joint->limits->upper - ((urdf_joint->limits->upper - urdf_joint->safety->soft_upper_limit) * margin);
}
