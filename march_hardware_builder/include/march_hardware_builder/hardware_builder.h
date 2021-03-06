// Copyright 2019 Project March.
#ifndef MARCH_HARDWARE_BUILDER_HARDWARE_BUILDER_H
#define MARCH_HARDWARE_BUILDER_HARDWARE_BUILDER_H
#include "march_hardware_builder/allowed_robot.h"

#include <memory>
#include <string>
#include <vector>

#include <urdf/model.h>
#include <yaml-cpp/yaml.h>

#include <march_hardware/imotioncube/actuation_mode.h>
#include <march_hardware/encoder/absolute_encoder.h>
#include <march_hardware/encoder/incremental_encoder.h>
#include <march_hardware/ethercat/pdo_interface.h>
#include <march_hardware/ethercat/sdo_interface.h>
#include <march_hardware/imotioncube/imotioncube.h>
#include <march_hardware/joint.h>
#include <march_hardware/march_robot.h>
#include <march_hardware/power/power_distribution_board.h>
#include <march_hardware/temperature/temperature_ges.h>

/**
 * @brief Creates a MarchRobot from a robot yaml and URDF.
 */
class HardwareBuilder
{
public:
  /**
   * @brief Initialises a HardwareBuilder with a robotName enumerator.
   * @details Grabs the .yaml file associated with the robot name.
   */
  explicit HardwareBuilder(AllowedRobot robot);

  /**
   * @brief Initialises with a robot name and URDF.
   */
  HardwareBuilder(AllowedRobot robot, urdf::Model urdf);

  /**
   * @brief Initialises a HardwareBuilder with a path to a .yaml file.
   */
  explicit HardwareBuilder(const std::string& yaml_path);

  /**
   * @brief Initialises with a path to yaml and URDF.
   */
  HardwareBuilder(const std::string& yaml_path, urdf::Model urdf);

  /**
   * @brief Creates a MarchRobot. Loads a URDF from the parameter server
   * on topic `/robot_description` when no URDF was given in the constructor.
   *
   * @throws HardwareConfigException When the urdf could not be loaded from the parameter server
   * @throws MissingKeyException When a required key is missing from the given config
   */
  std::unique_ptr<march::MarchRobot> createMarchRobot();

  /**
   * @brief Loops over all keys in the keyList and check if they exist in the
   * config.
   *
   * @throws MissingKeyException when required keys are missing.
   */
  static void validateRequiredKeysExist(const YAML::Node& config, const std::vector<std::string>& key_list,
                                        const std::string& object_name);

  static march::Joint createJoint(const YAML::Node& joint_config, const std::string& joint_name,
                                  const urdf::JointConstSharedPtr& urdf_joint, march::PdoInterfacePtr pdo_interface,
                                  march::SdoInterfacePtr sdo_interface);
  static std::unique_ptr<march::AbsoluteEncoder> createAbsoluteEncoder(const YAML::Node& absolute_encoder_config,
                                                                       const urdf::JointConstSharedPtr& urdf_joint);
  static std::unique_ptr<march::IncrementalEncoder>
  createIncrementalEncoder(const YAML::Node& incremental_encoder_config);
  static std::unique_ptr<march::IMotionCube> createIMotionCube(const YAML::Node& imc_config, march::ActuationMode mode,
                                                               const urdf::JointConstSharedPtr& urdf_joint,
                                                               march::PdoInterfacePtr pdo_interface,
                                                               march::SdoInterfacePtr sdo_interface);
  static std::unique_ptr<march::TemperatureGES> createTemperatureGES(const YAML::Node& temperature_ges_config,
                                                                     march::PdoInterfacePtr pdo_interface,
                                                                     march::SdoInterfacePtr sdo_interface);
  static std::unique_ptr<march::PowerDistributionBoard>
  createPowerDistributionBoard(const YAML::Node& power_distribution_board_config, march::PdoInterfacePtr pdo_interface,
                               march::SdoInterfacePtr sdo_interface);

  static const std::vector<std::string> INCREMENTAL_ENCODER_REQUIRED_KEYS;
  static const std::vector<std::string> ABSOLUTE_ENCODER_REQUIRED_KEYS;
  static const std::vector<std::string> IMOTIONCUBE_REQUIRED_KEYS;
  static const std::vector<std::string> TEMPERATUREGES_REQUIRED_KEYS;
  static const std::vector<std::string> POWER_DISTRIBUTION_BOARD_REQUIRED_KEYS;
  static const std::vector<std::string> JOINT_REQUIRED_KEYS;

private:
  /**
   * Initializes the URDF if necessary.
   */
  void initUrdf();

  /**
   * Returns all joints found in the given config.
   * Warns when joints are defined as FIXED in the URDF and when a non-FIXED
   * joint is not contained in the config.
   * @param joints_config YAML node that contains a sequence of joint objects
   * @return list of created joints
   */
  std::vector<march::Joint> createJoints(const YAML::Node& joints_config, march::PdoInterfacePtr pdo_interface,
                                         march::SdoInterfacePtr sdo_interface) const;

  YAML::Node robot_config_;
  urdf::Model urdf_;
  bool init_urdf_ = true;
};

/**
 * Converts the input filestream object to a stringstream object so that is easier to test for in IMotionCUbe.cpp
 */
std::string convertSWFileToString(std::ifstream& sw_file);

#endif  // MARCH_HARDWARE_BUILDER_HARDWARE_BUILDER_H
