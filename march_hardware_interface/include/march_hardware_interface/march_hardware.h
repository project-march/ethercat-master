#ifndef ROS_CONTROL__MARCH_HARDWARE_H
#define ROS_CONTROL__MARCH_HARDWARE_H

#include <boost/scoped_ptr.hpp>
#include <controller_manager/controller_manager.h>
#include <hardware_interface/joint_command_interface.h>
#include <hardware_interface/joint_state_interface.h>
#include <hardware_interface/robot_hw.h>
#include <joint_limits_interface/joint_limits.h>
#include <joint_limits_interface/joint_limits_interface.h>
#include <joint_limits_interface/joint_limits_rosparam.h>
#include <joint_limits_interface/joint_limits_urdf.h>
#include <march_hardware_interface/march_pdb_state_interface.h>
#include <march_hardware_interface/march_temperature_sensor_interface.h>
#include <march_hardware_interface/PowerNetType.h>
#include <ros/ros.h>

namespace march_hardware_interface {
/// \brief Hardware interface for a robot
class MarchHardware : public hardware_interface::RobotHW {
protected:
  // Interfaces
  hardware_interface::JointStateInterface joint_state_interface_;
  hardware_interface::PositionJointInterface position_joint_interface_;
  hardware_interface::VelocityJointInterface velocity_joint_interface_;
  hardware_interface::EffortJointInterface effort_joint_interface_;

  march_hardware_interface::MarchTemperatureSensorInterface
      march_temperature_interface;
  march_hardware_interface::MarchPdbStateInterface march_pdb_interface;

  joint_limits_interface::EffortJointSaturationInterface
      effort_joint_saturation_interface_;
  joint_limits_interface::EffortJointSoftLimitsInterface
      effort_joint_limits_interface_;
  joint_limits_interface::PositionJointSaturationInterface
      position_joint_saturation_interface_;
  joint_limits_interface::PositionJointSoftLimitsInterface
      position_joint_limits_interface_;
  joint_limits_interface::VelocityJointSaturationInterface
      velocity_joint_saturation_interface_;
  joint_limits_interface::VelocityJointSoftLimitsInterface
      velocity_joint_limits_interface_;

  // Custom or available transmissions
  // transmission_interface::RRBOTTransmission rrbot_trans_;
  // std::vector<transmission_interface::SimpleTransmission> simple_trans_;

  // Shared memory
  int num_joints_;
  int joint_mode_; // position, velocity, or effort
  std::vector<std::string> joint_names_;
  std::vector<int> joint_types_;
  std::vector<double> joint_position_;
  std::vector<double> joint_velocity_;
  std::vector<double> joint_effort_;
  std::vector<double> joint_position_command_;
  std::vector<double> joint_velocity_command_;
  std::vector<double> joint_effort_command_;

  march4cpp::PowerDistributionBoard power_distribution_board_read_;
  bool master_shutdown_allowed_command = false;
  //TODO(TIM) check if this is working::
  bool enable_high_voltage_command = true;
  PowerNetOnOffCommand power_net_on_off_command_;

  std::vector<double> joint_temperature_;
  std::vector<double> joint_temperature_variance_;

  std::vector<double> joint_lower_limits_;
  std::vector<double> joint_upper_limits_;
  std::vector<double> joint_effort_limits_;

}; // class

} // namespace

#endif