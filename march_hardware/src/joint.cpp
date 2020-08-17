// Copyright 2019 Project March.
#include "march_hardware/ethercat/slave.h"
#include "march_hardware/joint.h"
#include "march_hardware/error/hardware_exception.h"
#include "march_hardware/error/motion_error.h"

#include <ros/ros.h>

#include <bitset>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

namespace march
{
Joint::Joint(std::string name, int net_number) : name_(std::move(name)), net_number_(net_number)
{
}

Joint::Joint(std::string name, int net_number, bool allow_actuation, std::unique_ptr<MotorController> controller)
  : name_(std::move(name))
  , net_number_(net_number)
  , allow_actuation_(allow_actuation)
  , controller_(std::move(controller))
{
}

Joint::Joint(std::string name, int net_number, bool allow_actuation, std::unique_ptr<MotorController> controller,
             std::unique_ptr<TemperatureGES> temperature_ges)
  : name_(std::move(name))
  , net_number_(net_number)
  , allow_actuation_(allow_actuation)
  , controller_(std::move(controller))
  , temperature_ges_(std::move(temperature_ges))
{
}

bool Joint::initialize(int cycle_time)
{
  bool reset = false;
  reset |= this->controller_->initialize(cycle_time);
  if (this->hasTemperatureGES())
  {
    reset |= this->temperature_ges_->initSdo(cycle_time);
  }
  return reset;
}

void Joint::prepareActuation()
{
  if (!this->canActuate())
  {
    throw error::HardwareException(error::ErrorType::NOT_ALLOWED_TO_ACTUATE, "Failed to prepare joint %s for actuation",
                                   this->name_.c_str());
  }
  ROS_INFO("[%s] Preparing for actuation", this->name_.c_str());
  this->controller_->prepareActuation();
  ROS_INFO("[%s] Successfully prepared for actuation", this->name_.c_str());

  this->incremental_position_ = this->controller_->getAngleRadIncremental();
  this->absolute_position_ = this->controller_->getAngleRadAbsolute();
  this->position_ = this->absolute_position_;
  this->velocity_ = 0;
}

void Joint::resetMotorController()
{
  this->controller_->reset();
}

void Joint::actuateRad(double target_position)
{
  if (!this->canActuate())
  {
    throw error::HardwareException(error::ErrorType::NOT_ALLOWED_TO_ACTUATE, "Joint %s is not allowed to actuate",
                                   this->name_.c_str());
  }
  this->controller_->actuateRad(target_position);
}

void Joint::readEncoders(const ros::Duration& elapsed_time)
{
  if (this->receivedDataUpdate())
  {
    const double incremental_position_change =
        this->controller_->getAngleRadIncremental() - this->incremental_position_;

    // Take the velocity and position from the encoder with the highest resolution.
    if (this->controller_->getIncrementalMorePrecise())
    {
      this->velocity_ = this->controller_->getVelocityRadIncremental();
      this->position_ += incremental_position_change;
    }
    else
    {
      this->velocity_ = this->controller_->getVelocityRadAbsolute();
      this->position_ = this->controller_->getAngleRadAbsolute();
    }
    this->incremental_position_ += incremental_position_change;
    this->absolute_position_ = this->controller_->getAngleRadAbsolute();
  }
  else
  {
    // Update positions with velocity from last time step
    this->position_ += this->velocity_ * elapsed_time.toSec();
    this->incremental_position_ += this->velocity_ * elapsed_time.toSec();
    this->absolute_position_ += this->velocity_ * elapsed_time.toSec();
  }
}

double Joint::getPosition() const
{
  return this->position_;
}

double Joint::getVelocity() const
{
  return this->velocity_;
}

double Joint::getVoltageVelocity() const
{
  // For the underlying calculations, see:
  // https://en.wikipedia.org/wiki/Motor_constants#Motor_velocity_constant,_back_EMF_constant
  // https://www.motioncontroltips.com/faq-whats-relationship-voltage-dc-motor-output-speed/
  const double resistance = 0.05;
  const double velocity_constant = 355;
  const double rpm_to_rad = M_PI / 30;
  const double electric_constant = velocity_constant * rpm_to_rad;
  return (this->controller_->getMotorVoltage() + this->controller_->getMotorCurrent() * resistance) / electric_constant;
}

double Joint::getIncrementalPosition() const
{
  return this->incremental_position_;
}

double Joint::getAbsolutePosition() const
{
  return this->absolute_position_;
}

void Joint::actuateTorque(double target_torque)
{
  if (!this->canActuate())
  {
    throw error::HardwareException(error::ErrorType::NOT_ALLOWED_TO_ACTUATE, "Joint %s is not allowed to actuate",
                                   this->name_.c_str());
  }
  this->controller_->actuateTorque(target_torque);
}

double Joint::getTorque()
{
  return this->controller_->getTorque();
}

float Joint::getTemperature()
{
  if (!this->hasTemperatureGES())
  {
    ROS_WARN("[%s] Has no temperature sensor", this->name_.c_str());
    return -1.0;
  }
  return this->temperature_ges_->getTemperature();
}

MotorControllerStates& Joint::getMotorControllerStates()
{
  return this->controller_->getStates();
}

void Joint::setAllowActuation(bool allow_actuation)
{
  this->allow_actuation_ = allow_actuation;
}

int Joint::getTemperatureGESSlaveIndex() const
{
  if (this->hasTemperatureGES())
  {
    return this->temperature_ges_->getSlaveIndex();
  }
  return -1;
}

int Joint::getMotorControllerSlaveIndex() const
{
  return this->controller_->getSlaveIndex();
}

int Joint::getNetNumber() const
{
  return this->net_number_;
}

std::string Joint::getName() const
{
  return this->name_;
}

bool Joint::hasMotorController() const
{
  return this->controller_ != nullptr;
}

bool Joint::hasTemperatureGES() const
{
  return this->temperature_ges_ != nullptr;
}

bool Joint::canActuate() const
{
  return this->allow_actuation_ && this->hasMotorController();
}

bool Joint::receivedDataUpdate()
{
  // If imc voltage, motor current, and both encoders positions and velocities did not change,
  // we probably did not receive an update for this joint.
  float new_controller_volt = this->controller_->getMotorControllerVoltage();
  float new_motor_volt = this->controller_->getMotorVoltage();
  float new_motor_current = this->controller_->getMotorCurrent();
  double new_absolute_position = this->controller_->getAngleRadAbsolute();
  double new_incremental_position = this->controller_->getAngleRadIncremental();
  double new_absolute_velocity = this->controller_->getVelocityRadAbsolute();
  double new_incremental_velocity = this->controller_->getVelocityRadIncremental();
  bool data_updated =
      (new_controller_volt != this->previous_controller_volt_ || new_motor_volt != this->previous_motor_volt_ ||
       new_motor_current != this->previous_motor_current_ ||
       new_absolute_position != this->previous_absolute_position_ ||
       new_incremental_position != this->previous_incremental_position_ ||
       new_absolute_velocity != this->previous_absolute_velocity_ ||
       new_incremental_velocity != this->previous_incremental_velocity_);

  this->previous_controller_volt_ = new_controller_volt;
  this->previous_motor_volt_ = new_motor_volt;
  this->previous_motor_current_ = new_motor_current;
  this->previous_absolute_position_ = new_absolute_position;
  this->previous_incremental_position_ = new_incremental_position;
  this->previous_absolute_velocity_ = new_absolute_velocity;
  this->previous_incremental_velocity_ = new_incremental_velocity;
  return data_updated;
}

ActuationMode Joint::getActuationMode() const
{
  return this->controller_->getActuationMode();
}

}  // namespace march
