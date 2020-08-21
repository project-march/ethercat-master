#include "march_hardware/motor_controller/odrive/odrive_motor.h"
#include "march_hardware/motor_controller/odrive/odrive_states.h"

#include <utility>

namespace march
{
OdriveMotor::OdriveMotor(const std::string& axisNumber, std::shared_ptr<OdriveEndpoint> odriveEndpoint,
                         ActuationMode mode, std::string json_config_file_path)
  : Odrive(axisNumber, std::move(odriveEndpoint), false), mode_(mode), json_config_file_path_(json_config_file_path)
{
}

OdriveMotor::~OdriveMotor()
{
  if (this->setState(States::AXIS_STATE_IDLE) == 1)
  {
    ROS_FATAL("Not set to idle when closed");
    return;
  }
}

bool OdriveMotor::initialize(int cycle_time)
{
  return cycle_time;
}

void OdriveMotor::prepareActuation()
{
  this->importOdriveJson();

  if (this->setConfigurations(this->json_config_file_path_) == 1)
  {
    ROS_FATAL("Setting configurations was not finished successfully");
  }

  this->reset();

  if (this->setState(States::AXIS_STATE_FULL_CALIBRATION_SEQUENCE) == 1)
  {
    ROS_FATAL("Calibration sequence was not finished successfully");
    return;
  }

  this->waitForIdleState();

  if (this->setState(States::AXIS_STATE_CLOSED_LOOP_CONTROL) == 1)
  {
    ROS_FATAL("Setting closed loop control was not finished successfully");
    return;
  }
  this->readValues();
}

bool OdriveMotor::waitForIdleState(float timeout)
{
  float current_time = 0;
  while (this->getState() != States::AXIS_STATE_IDLE)
  {
    ros::Duration(0.5).sleep();
    current_time += 0.5;

    if (current_time == timeout)
    {
      ROS_FATAL("Odrive axis did not return to idle state, current state is %i", this->getState());
      return false;
    }
  }
  return true;
}

// to be implemented
void OdriveMotor::reset()
{
  uint16_t axis_error = 0;
  std::string command_name_ = this->create_command(O_PM_AXIS_ERROR);
  if (this->write(command_name_, axis_error) == 1)
  {
    ROS_ERROR("Could not reset axis");
  }

  uint16_t axis_motor_error = 0;
  command_name_ = this->create_command(O_PM_AXIS_MOTOR_ERROR);
  if (this->write(command_name_, axis_motor_error) == 1)
  {
    ROS_ERROR("Could not reset motor axis");
  }

  uint8_t axis_encoder_error = 0;
  command_name_ = this->create_command(O_PM_AXIS_ENCODER_ERROR);
  if (this->write(command_name_, axis_encoder_error) == 1)
  {
    ROS_ERROR("Could not reset encoder axis");
  }

  uint8_t axis_controller_error = 0;
  command_name_ = this->create_command(O_PM_AXIS_CONTROLLER_ERROR);
  if (this->write(command_name_, axis_controller_error) == 1)
  {
    ROS_ERROR("Could not reset controller axis");
  }
}

void OdriveMotor::actuateRad(double target_rad)
{
  ROS_INFO("Actuating rad %f", target_rad);
  return;
}

void OdriveMotor::actuateTorque(double target_torque_ampere)
{
  float target_torque_ampere_float = (float)target_torque_ampere;

  std::string command_name_ = this->create_command(O_PM_DESIRED_MOTOR_CURRENT);
  if (this->write(command_name_, target_torque_ampere_float) == 1)
  {
    ROS_ERROR("Could net set target torque; %f to the axis", target_torque_ampere);
  }

  this->readValues();
}

MotorControllerStates& OdriveMotor::getStates()
{
  static OdriveStates states;

  states.motorCurrent = this->getMotorCurrent();
  states.controllerVoltage = this->getMotorControllerVoltage();
  states.motorVoltage = this->getMotorVoltage();

  states.absoluteEncoderValue = this->getAngleCountsAbsolute();
  states.incrementalEncoderValue = this->getAngleCountsIncremental();
  states.absoluteVelocity = this->getVelocityRadAbsolute();
  states.incrementalVelocity = this->getVelocityRadIncremental();

  states.axisError = this->getAxisError();
  states.axisMotorError = this->getAxisMotorError();
  states.axisEncoderError = this->getAxisEncoderError();
  states.axisControllerError = this->getAxisControllerError();

  states.state = States(this->getState());

  return states;
}

void OdriveMotor::readValues()
{
  this->axis_error = this->readAxisError();
  this->axis_motor_error = this->readAxisMotorError();
  this->axis_encoder_error = this->readAxisEncoderError();
  this->axis_controller_error = this->readAxisControllerError();

  this->motor_controller_voltage = this->readMotorControllerVoltage();
  this->motor_current = this->readMotorCurrent();
  this->motor_voltage = this->readMotorVoltage();

  this->angle_counts_absolute = this->readAngleCountsAbsolute();
  this->velocity_rad_absolute = this->readVelocityRadAbsolute();
  this->angle_counts_incremental = this->readAngleCountsIncremental();
  this->velocity_rad_incremental = this->readVelocityRadIncremental();
}

uint16_t OdriveMotor::getAxisError()
{
  return this->axis_error;
}

uint16_t OdriveMotor::getAxisMotorError()
{
  return this->axis_motor_error;
}

uint8_t OdriveMotor::getAxisEncoderError()
{
  return this->axis_encoder_error;
}

uint8_t OdriveMotor::getAxisControllerError()
{
  return this->axis_controller_error;
}

float OdriveMotor::getMotorControllerVoltage()
{
  return this->motor_controller_voltage;
}

float OdriveMotor::getMotorCurrent()
{
  return this->motor_current;
}

float OdriveMotor::getMotorVoltage()
{
  return this->motor_voltage;
}

double OdriveMotor::getTorque()
{
  return this->getMotorCurrent();
}

int OdriveMotor::getAngleCountsAbsolute()
{
  return this->angle_counts_absolute;
}

double OdriveMotor::getAngleRadAbsolute()
{
  double angle_rad = this->getAngleCountsAbsolute() * PI_2 / (std::pow(2, 12) * 101);
  return angle_rad;
}

double OdriveMotor::getVelocityRadAbsolute()
{
  return this->velocity_rad_absolute;
}

bool OdriveMotor::getIncrementalMorePrecise() const
{
  return true;
}

int OdriveMotor::getAngleCountsIncremental()
{
  return this->angle_counts_incremental;
}

double OdriveMotor::getAngleRadIncremental()
{
  double angle_rad = this->getAngleCountsIncremental() * PI_2 / (std::pow(2, 12) * 101);
  return angle_rad;
}

double OdriveMotor::getVelocityRadIncremental()
{
  double velocity_rad_incremental_double = this->velocity_rad_incremental * -1;
  return velocity_rad_incremental_double;
}

int OdriveMotor::setState(uint8_t state)
{
  std::string command_name_ = this->create_command(O_PM_REQUEST_STATE);
  if (this->write(command_name_, state) == 1)
  {
    ROS_ERROR("Could net set state; %i to the axis", state);
    return ODRIVE_ERROR;
  }
  return ODRIVE_OK
}

uint8_t OdriveMotor::getState()
{
  uint8_t axis_state;
  std::string command_name_ = this->create_command(O_PM_CURRENT_STATE);
  if (this->read(command_name_, axis_state) == 1)
  {
    ROS_ERROR("Could not retrieve state of the motor");
    return ODRIVE_ERROR;
  }

  return axis_state;
}

}  // namespace march
