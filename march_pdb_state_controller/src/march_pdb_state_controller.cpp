// Copyright 2019 Project March.
#include <string>
#include <vector>

#include <march_pdb_state_controller/march_pdb_state_controller.h>

namespace march_pdb_state_controller
{
// TODO(TIM) Remove the callbacks and subscribers when they are not needed anymore
// These topics make it able to tests write to pdb commands.
bool MarchPdbStateController::serviceDisableEnableHighVoltage(std_srvs::SetBool::Request& req,
                                                              std_srvs::SetBool::Response& /* res */)
{
  if (pdb_state_.getHighVoltageEnabled() != req.data)
  {
    ROS_WARN("highVoltageDisableEnable is already %d", req.data);
    return false;
  }
  ROS_INFO("trying to set highVoltageNetEnableDisable to %d", req.data);
  pdb_state_.highVoltageNetEnableDisable(req.data);
  return true;
}

void MarchPdbStateController::masterShutdownAllowedCallback(const std_msgs::Bool::ConstPtr& msg)
{
  pdb_state_.setMasterShutdownAllowed(msg->data);
}
void MarchPdbStateController::turnHighVoltageNetOnOrOffCallBack(const std_msgs::Int8::ConstPtr& msg)
{
  int net_number = msg->data;
  // sign indicates on or off
  if (net_number > 0)
  {
    pdb_state_.turnNetOnOrOff(PowerNetType("high_voltage"), true, net_number);
  }
  else
  {
    net_number *= -1;
    pdb_state_.turnNetOnOrOff(PowerNetType("high_voltage"), false, net_number);
  }
}
void MarchPdbStateController::turnLowVoltageNetOnOrOffCallBack(const std_msgs::Int8::ConstPtr& msg)
{
  int net_number = msg->data;
  // sign indicates on or off
  if (net_number > 0)
  {
    pdb_state_.turnNetOnOrOff(PowerNetType("low_voltage"), true, net_number);
  }
  else
  {
    net_number *= -1;
    pdb_state_.turnNetOnOrOff(PowerNetType("low_voltage"), false, net_number);
  }
}

bool MarchPdbStateController::init(MarchPdbStateInterface* hw, ros::NodeHandle& root_nh, ros::NodeHandle& controller_nh)
{
  // Get all temperature_sensors from the hardware interface
  const std::vector<std::string>& pdb_state_names = hw->getNames();

  // get publishing period
  if (!controller_nh.getParam("publish_rate", publish_rate_))
  {
    ROS_ERROR("Parameter 'publish_rate' not set");
    return false;
  }

  if (pdb_state_names.size() == 1)
  {
    // sensor handle
    pdb_state_ = (hw->getHandle(pdb_state_names[0]));
    // realtime publisher
    RtPublisherPtr rt_pub(new realtime_tools::RealtimePublisher<march_shared_resources::PowerDistributionBoardState>(
        root_nh, "/march/pdb/" + pdb_state_names[0], 4));
    realtime_pubs_ = rt_pub;
  }

  enable_disable_high_voltage_service = controller_nh.advertiseService(
      "enable_all_high_voltage", &MarchPdbStateController::serviceDisableEnableHighVoltage, this);

  sub_master_shutdown_allowed =
      controller_nh.subscribe("shutdown_allowed", 1000, &MarchPdbStateController::masterShutdownAllowedCallback, this);

  sub_turn_low_net_on_or_off = controller_nh.subscribe(
      "low_voltage_net/on_or_off", 1000, &MarchPdbStateController::turnLowVoltageNetOnOrOffCallBack, this);

  sub_turn_high_net_on_or_off = controller_nh.subscribe(
      "high_voltage_net/on_or_off", 1000, &MarchPdbStateController::turnHighVoltageNetOnOrOffCallBack, this);

  return true;
}

void MarchPdbStateController::starting(const ros::Time& time)
{
  // initialize time
  last_publish_times_ = time;
}

std::vector<march_shared_resources::HighVoltageNet>
MarchPdbStateController::createHighVoltageNetsMessage(march::HighVoltage high_voltage)
{
  std::vector<march_shared_resources::HighVoltageNet> highVoltageNetMsgs = {};
  for (int i = 1; i < 9; i++)
  {
    march_shared_resources::HighVoltageNet highVoltageNetMsg;
    highVoltageNetMsg.name = boost::lexical_cast<std::string>(i);
    highVoltageNetMsg.operational = high_voltage.getNetOperational(i);
    highVoltageNetMsg.overcurrent_triggered = high_voltage.getOvercurrentTrigger(i);
    highVoltageNetMsgs.push_back(highVoltageNetMsg);
  }
  return highVoltageNetMsgs;
}
std::vector<march_shared_resources::LowVoltageNet>
MarchPdbStateController::createLowVoltageNetsMessage(march::LowVoltage low_voltage)
{
  std::vector<march_shared_resources::LowVoltageNet> lowVoltageNetMsgs = {};
  march_shared_resources::LowVoltageNet power_net_msg;
  for (int i = 1; i < 3; i++)
  {
    march_shared_resources::LowVoltageNet lowVoltageNetMsg;
    lowVoltageNetMsg.name = boost::lexical_cast<std::string>(i);
    lowVoltageNetMsg.operational = low_voltage.getNetOperational(i);
    lowVoltageNetMsg.current = low_voltage.getNetCurrent(i);
    lowVoltageNetMsgs.push_back(lowVoltageNetMsg);
  }
  return lowVoltageNetMsgs;
}

void MarchPdbStateController::update(const ros::Time& time, const ros::Duration& /*period*/)
{
  // limit rate of publishing
  if (publish_rate_ > 0.0 && last_publish_times_ + ros::Duration(1.0 / publish_rate_) < time)
  {
    // try to publish
    if (realtime_pubs_->trylock())
    {
      // we're actually publishing, so increment time
      last_publish_times_ = last_publish_times_ + ros::Duration(1.0 / publish_rate_);
      march::PowerDistributionBoard* pBoard = pdb_state_.getPowerDistributionBoard();
      realtime_pubs_->msg_.header.stamp = ros::Time::now();
      realtime_pubs_->msg_.low_voltage_nets = createLowVoltageNetsMessage(pBoard->getLowVoltage());
      realtime_pubs_->msg_.high_voltage_nets = createHighVoltageNetsMessage(pBoard->getHighVoltage());
      realtime_pubs_->msg_.master_shutdown_requested = pBoard->getMasterShutdownRequested();
      realtime_pubs_->msg_.power_distribution_board_current = pBoard->getPowerDistributionBoardCurrent();
      realtime_pubs_->msg_.high_voltage_enabled = pBoard->getHighVoltage().getHighVoltageEnabled();
      realtime_pubs_->unlockAndPublish();
    }
  }
}

void MarchPdbStateController::stopping(const ros::Time& /*time*/)
{
}
}  // namespace march_pdb_state_controller

PLUGINLIB_EXPORT_CLASS(march_pdb_state_controller::MarchPdbStateController, controller_interface::ControllerBase)
