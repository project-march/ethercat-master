// Copyright 2019 Project March
#ifndef MARCH_HARDWARE_INTERFACE_MARCH_HARDWARE_INTERFACE_H
#define MARCH_HARDWARE_INTERFACE_MARCH_HARDWARE_INTERFACE_H
#include <vector>

#include <control_toolbox/filters.h>
#include <march_hardware_interface/march_hardware.h>
#include <ros/ros.h>
#include <realtime_tools/realtime_publisher.h>
#include <march_shared_resources/ImcErrorState.h>
#include <march_shared_resources/AfterLimitJointCommand.h>
#include <march_hardware_builder/hardware_builder.h>

#include <march_hardware/MarchRobot.h>

using hardware_interface::JointStateHandle;
using hardware_interface::PositionJointInterface;
using joint_limits_interface::EffortJointSoftLimitsHandle;
using joint_limits_interface::EffortJointSoftLimitsInterface;
using joint_limits_interface::JointLimits;
using joint_limits_interface::PositionJointSoftLimitsHandle;
using joint_limits_interface::PositionJointSoftLimitsInterface;
using joint_limits_interface::SoftJointLimits;

static const double POSITION_STEP_FACTOR = 10;
static const double VELOCITY_STEP_FACTOR = 10;
static const int LOWER_BOUNDARY_ANGLE_IU = -2;
static const int UPPER_BOUNDARY_ANGLE_IU = 2;

/**
 * @brief HardwareInterface to allow ros_control to actuate our hardware.
 * @details Register an interface for each joint such that they can be actuated
 *     by a controller via ros_control.
 */
class MarchHardwareInterface : public MarchHardware
{
public:
  MarchHardwareInterface(ros::NodeHandle& nh, AllowedRobot robotName);

  /**
   * @brief Initialize the HardwareInterface by registering position interfaces
   * for each joint.
   */
  void init();
  void update(const ros::Duration& elapsed_time);

  /**
   * @brief Read actual postion from the hardware.
   */
  void read(const ros::Duration& elapsed_time = ros::Duration(0.01));

  /**
   * @brief Perform all safety checks that might crash the exoskeleton.
   */
  void validate();

  /**
   * @brief Write position commands to the hardware.
   * @param elapsed_time Duration since last write action
   */
  void write(const ros::Duration& elapsed_time);

protected:
  ::march::MarchRobot marchRobot;
  ros::NodeHandle nh_;
  ros::Duration control_period_;
  ros::Duration elapsed_time_;
  PositionJointInterface positionJointInterface;
  PositionJointSoftLimitsInterface positionJointSoftLimitsInterface;
  EffortJointSoftLimitsInterface effortJointSoftLimitsInterface;
  bool hasPowerDistributionBoard = false;
  boost::shared_ptr<controller_manager::ControllerManager> controller_manager_;
  typedef boost::shared_ptr<realtime_tools::RealtimePublisher<march_shared_resources::ImcErrorState> > RtPublisherPtr;
  typedef boost::shared_ptr<realtime_tools::RealtimePublisher<march_shared_resources::AfterLimitJointCommand> >
      RtPublisherAfterLimitJointCommandPtr;
  RtPublisherAfterLimitJointCommandPtr after_limit_joint_command_pub_;
  RtPublisherPtr imc_state_pub_;
  double p_error_, v_error_, e_error_;
  std::vector<SoftJointLimits> soft_limits_;

private:
  void updatePowerNet();
  void updateHighVoltageEnable();
  void updatePowerDistributionBoard();
  void updateAfterLimitJointCommand();
  void updateIMotionCubeState();
  void initiateIMC();
  void outsideLimitsCheck(int joint_index);
  void iMotionCubeStateCheck(int joint_index);
};

#endif  // MARCH_HARDWARE_INTERFACE_MARCH_HARDWARE_INTERFACE_H
