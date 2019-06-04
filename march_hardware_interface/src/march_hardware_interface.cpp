// Copyright 2019 Project March.

#include <sstream>
#include <joint_limits_interface/joint_limits_interface.h>
#include <joint_limits_interface/joint_limits.h>
#include <joint_limits_interface/joint_limits_urdf.h>
#include <joint_limits_interface/joint_limits_rosparam.h>
#include <march_hardware/ActuationMode.h>
#include <march_hardware/MarchRobot.h>

#include <march_hardware_interface/march_hardware_interface.h>

using joint_limits_interface::JointLimits;
using joint_limits_interface::SoftJointLimits;
using joint_limits_interface::PositionJointSoftLimitsHandle;
using joint_limits_interface::PositionJointSoftLimitsInterface;

namespace march_hardware_interface
{
MarchHardwareInterface::MarchHardwareInterface(ros::NodeHandle& nh, AllowedRobot robotName)
  : nh_(nh), marchRobot(HardwareBuilder(robotName).createMarchRobot())
{
  init();
  controller_manager_.reset(new controller_manager::ControllerManager(this, nh_));
  nh_.param("/march/hardware_interface/loop_hz", loop_hz_, 0.1);
  ros::Duration update_freq = ros::Duration(1.0 / loop_hz_);
  non_realtime_loop_ = nh_.createTimer(update_freq, &MarchHardwareInterface::update, this);
}

MarchHardwareInterface::~MarchHardwareInterface()
{
}

void MarchHardwareInterface::init()
{
  // Start ethercat cycle in the hardware
  this->marchRobot.startEtherCAT();

  if (!this->marchRobot.isEthercatOperational())
  {
    ROS_FATAL("EtherCAT is not operational");
    exit(0);
  }

  // Get joint names
  nh_.getParam("/march/hardware_interface/joints", joint_names_);
  num_joints_ = joint_names_.size();

//  this->contlist_client = new ContListClient("/march/controller_manager/list_controllers", true);

  // Resize vectors
  joint_position_.resize(num_joints_);
  joint_velocity_.resize(num_joints_);
  joint_effort_.resize(num_joints_);
  joint_temperature_.resize(num_joints_);
  joint_temperature_variance_.resize(num_joints_);
  joint_position_command_.resize(num_joints_);
  joint_velocity_command_.resize(num_joints_);
  joint_effort_command_.resize(num_joints_);

  // Print all joint positions on startup in case initialization fails.
  this->read();
  for (int i = 0; i < num_joints_; ++i)
  {
    ROS_INFO("Joint %s: first read position: %f", joint_names_[i].c_str(), joint_position_[i]);
  }

  // Initialize interfaces for each joint
  for (int i = 0; i < num_joints_; ++i)
  {
    march4cpp::Joint joint = marchRobot.getJoint(joint_names_[i]);
    // Create joint state interface
    JointStateHandle jointStateHandle(joint.getName(), &joint_position_[i], &joint_velocity_[i], &joint_effort_[i]);
    joint_state_interface_.registerHandle(jointStateHandle);

    // Create position joint interface
    JointHandle jointPositionHandle(jointStateHandle, &joint_position_command_[i]);

    // Create effort joint interface
    JointHandle jointEffortHandle(jointStateHandle, &joint_effort_command_[i]);

    // Retrieve joint (soft) limits from the parameter server
    JointLimits limits;
    getJointLimits(joint.getName(), nh_, limits);
    SoftJointLimits softLimits;
    getSoftJointLimits(joint.getName(), nh_, softLimits);

    // Create joint limit interface
    PositionJointSoftLimitsHandle jointLimitsHandle(jointPositionHandle, limits, softLimits);
    positionJointSoftLimitsInterface.registerHandle(jointLimitsHandle);
    position_joint_interface_.registerHandle(jointPositionHandle);

    EffortJointSoftLimitsHandle effortLimitsHandle(jointEffortHandle, limits, softLimits);
    effortJointSoftLimitsInterface.registerHandle(effortLimitsHandle);
    effort_joint_interface_.registerHandle(jointEffortHandle);

    // Set the first target as the current position
    this->read();
    joint_velocity_[i] = 0;
    joint_effort_[i] = 0;
    joint_position_command_[i] = joint_position_[i];

    if (joint_position_[i] < softLimits.min_position || joint_position_[i] > softLimits.max_position)
    {
      ROS_FATAL("Joint %s is outside of its softLimits (%f, %f). Actual position: %f", joint_names_[i].c_str(),
                softLimits.min_position, softLimits.max_position, joint_position_[i]);

      std::ostringstream errorStream;
      errorStream << "Joint " << joint_names_[i].c_str() << " is out of its softLimits (" << softLimits.min_position
                  << ", " << softLimits.max_position << "). Actual position: " << joint_position_[i];
      throw ::std::invalid_argument(errorStream.str());
    }

    // Create velocity joint interface
    JointHandle jointVelocityHandle(jointStateHandle, &joint_velocity_command_[i]);
    velocity_joint_interface_.registerHandle(jointVelocityHandle);

    // Create march_state interface
    MarchTemperatureSensorHandle marchTemperatureSensorHandle(joint_names_[i], &joint_temperature_[i],
                                                              &joint_temperature_variance_[i]);
    march_state_interface.registerHandle(marchTemperatureSensorHandle);

    // Enable high voltage on the IMC
    if (joint.canActuate())
    {
      joint.prepareActuation();
    }
  }

  registerInterface(&march_state_interface);

  registerInterface(&joint_state_interface_);
  registerInterface(&position_joint_interface_);
  registerInterface(&effort_joint_interface_);
  registerInterface(&positionJointSoftLimitsInterface);
}

void MarchHardwareInterface::update(const ros::TimerEvent& e)
{
  elapsed_time_ = ros::Duration(e.current_real - e.last_real);
  read(elapsed_time_);
  controller_manager_->update(ros::Time::now(), elapsed_time_);
  write(elapsed_time_);
}

void MarchHardwareInterface::read(ros::Duration elapsed_time)
{
  for (int i = 0; i < num_joints_; i++)
  {
    float oldPosition = joint_position_[i];

    joint_position_[i] = marchRobot.getJoint(joint_names_[i]).getAngleRad();

    if (marchRobot.getJoint(joint_names_[i]).hasTemperatureGES())
    {
        joint_temperature_[i] = marchRobot.getJoint(joint_names_[i]).getTemperature();
    }

    // Get velocity from encoder position
    float joint_velocity = (joint_position_[i] - oldPosition) * 1 / elapsed_time.toSec();

    // Apply exponential smoothing to velocity obtained from encoder with alpha=0.2
    joint_velocity_[i] = filters::exponentialSmoothing(joint_velocity, joint_velocity_[i], 0.2);

    ROS_DEBUG("Joint %s: read position %f", joint_names_[i].c_str(), joint_position_[i]);
    joint_effort_[i] = marchRobot.getJoint(joint_names_[i]).getTorque();
  }
}

void MarchHardwareInterface::write(ros::Duration elapsed_time)
{
  ROS_INFO_THROTTLE(0.1, "Before limits: Trying to actuate joint %s, to %lf rad, %f speed, %f effort.",
                    joint_names_[0].c_str(), joint_position_command_[0], joint_velocity_command_[0],
                    joint_effort_command_[0]);
  positionJointSoftLimitsInterface.enforceLimits(elapsed_time);
  effortJointSoftLimitsInterface.enforceLimits(elapsed_time);

  for (int i = 0; i < num_joints_; i++)
  {
    march4cpp::Joint singleJoint = marchRobot.getJoint(joint_names_[i]);
    if (marchRobot.getJoint(joint_names_[i]).canActuate())
    {
      ROS_INFO("After limits: Trying to actuate joint %s, to %lf rad, %f speed, %f effort.", joint_names_[i].c_str(),
                joint_position_command_[i], joint_velocity_command_[i], joint_effort_command_[i]);

      if (singleJoint.getActuationMode() == ActuationMode::position)
      {
        singleJoint.actuateRad(static_cast<float>(joint_position_command_[i]));
      }
      //    marchRobot.getJoint(joint_names_[i]).actuateRad(static_cast<float>(joint_position_command_[i]));

      if (singleJoint.getActuationMode() == ActuationMode::torque)
      {
        singleJoint.actuateCurrent(static_cast<float>(joint_effort_command_[i]));
      }
    }
  }
}
}  // namespace march_hardware_interface
