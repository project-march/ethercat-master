#pragma once
#include "march_imu_manager/mtw.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <ros/ros.h>
#include <xsensdeviceapi.h>
#include <xstypes.h>

/**
 * The wireless master class that connects to MTws and publishes the data on
 * '/march/imu' ROS topic.
 */
class WirelessMaster : public XsCallback
{
public:
  explicit WirelessMaster(ros::NodeHandle& node);
  ~WirelessMaster() override;

  /**
   * Finds and constructs a wireless master.
   * This method must be called first before everything else.
   */
  int init();

  /**
   * Configures the wireless master with given settings.
   * Can only be configured once init() succeeded.
   *
   * @param update_rate the desired update rate of the master in Hz.
   * @param channel the desired radio channel, defaults to 25.
   * @returns error code, 0 if successfull, -1 otherwise.
   */
  int configure(int update_rate, int channel = 25);

  /**
   * Waits for the given amount of MTws to connect.
   * Blocks the thread until the amount has connected.
   *
   * @param connections the amount of connections to wait for, defaults to 1.
   */
  void waitForConnections(size_t connections = 1);

  /**
   * Starts the measurement of the MTw. This is required in order to
   * publish anything in the update loop. Once the measurement is started
   * no MTws will be able to connect.
   *
   * @returns true if successful, false otherwise.
   */
  bool startMeasurement();

  /**
   * Returns whether the MTws are measuring.
   */
  bool isMeasuring() const;

  /**
   * Publishes all data from the MTws. This is supposed to be called in
   * an update loop.
   */
  void update();

  /**
   * Finds the closest supported update rate to the given desired rate.
   *
   * @param supported_update_rates rates that are supported by the master.
   * @param desired_update_rate rate that is desired.
   */
  static int findClosestUpdateRate(const XsIntArray& supported_update_rates, int desired_update_rate);

protected:
  /**
   * Callback for when new MTws connect or disconnect.
   * Runs in a separate thread.
   */
  void onConnectivityChanged(XsDevice* dev, XsConnectivityState new_state) override;

private:
  ros::Publisher imu_pub_;

  std::mutex mutex_;
  std::condition_variable cv_;

  XsControl* control_ = nullptr;
  XsDevicePtr master_ = nullptr;

  std::unordered_map<uint32_t, std::unique_ptr<Mtw>> connected_mtws_;
};
