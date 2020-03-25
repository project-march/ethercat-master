// Copyright 2019 Project March.
#include "march_hardware_interface/march_hardware_interface.h"

#include <controller_manager/controller_manager.h>
#include <ros/ros.h>

#include <march_hardware/error/hardware_exception.h>
#include <march_hardware_builder/hardware_builder.h>

int main(int argc, char** argv)
{
  ros::init(argc, argv, "march_hardware_interface");
  ros::NodeHandle nh;
  ros::AsyncSpinner spinner(2);

  if (argc < 2)
  {
    ROS_FATAL("Missing robot argument\nusage: march_hardware_interface_node ROBOT");
    return 1;
  }
  AllowedRobot selected_robot = AllowedRobot(argv[1]);
  ROS_INFO_STREAM("Selected robot: " << selected_robot);

  spinner.start();

  HardwareBuilder builder(selected_robot);
  MarchHardwareInterface march(builder.createMarchRobot());

  try
  {
    bool success = march.init(nh, nh);
    if (!success)
    {
      return 1;
    }
  }
  catch (const std::exception& e)
  {
    ROS_FATAL("Hardware interface caught an exception during init");
    ROS_FATAL("%s", e.what());
    return 1;
  }

  controller_manager::ControllerManager controller_manager(&march, nh);
  ros::Time last_update_time = ros::Time::now() - ros::Duration(march.getEthercatCycleTime() / 1000.0);

  while (ros::ok())
  {
    try
    {
      march.waitForPdo();

      const ros::Time now = ros::Time::now();
      ros::Duration elapsed_time = now - last_update_time;
      last_update_time = now;

      march.read(now, elapsed_time);
      march.validate();
      controller_manager.update(now, elapsed_time);
      march.write(now, elapsed_time);
    }
    catch (const std::exception& e)
    {
      ROS_FATAL("Hardware interface caught an exception during update");
      ROS_FATAL("%s", e.what());
      return 1;
    }
  }

  return 0;
}
