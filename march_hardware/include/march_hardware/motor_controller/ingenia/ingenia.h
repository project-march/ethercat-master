// Copyright 2019 Project March.

#ifndef MARCH_HARDWARE_INGENIA_H
#define MARCH_HARDWARE_INGENIA_H
#include "march_hardware/motor_controller/actuation_mode.h"
#include "march_hardware/ethercat/pdo_map.h"
#include "march_hardware/ethercat/pdo_types.h"
#include "march_hardware/ethercat/sdo_interface.h"
#include "march_hardware/ethercat/slave.h"
#include "march_hardware/motor_controller/motor_controller.h"
#include "march_hardware/motor_controller/motor_controller_states.h"
#include "ingenia_target_state.h"
#include "march_hardware/encoder/absolute_encoder.h"
#include "march_hardware/encoder/incremental_encoder.h"

#include <memory>
#include <unordered_map>
#include <string>

namespace march
{
class Ingenia : public MotorController, public Slave
{
public:
  /**
   * Constructs an Ingenia with an incremental and absolute encoder.
   *
   * @param slave slave of the Ingenia
   * @param absolute_encoder pointer to absolute encoder, required so cannot be nullptr
   * @param incremental_encoder pointer to incremental encoder, required so cannot be nullptr
   * @param actuation_mode actuation mode in which the Ingenia must operate
   * @throws std::invalid_argument When an absolute or incremental encoder is nullptr.
   */
  Ingenia(const Slave& slave, std::unique_ptr<AbsoluteEncoder> absolute_encoder,
              std::unique_ptr<IncrementalEncoder> incremental_encoder, ActuationMode actuation_mode);
  Ingenia(const Slave& slave, std::unique_ptr<AbsoluteEncoder> absolute_encoder,
              std::unique_ptr<IncrementalEncoder> incremental_encoder, std::string& sw_stream,
              ActuationMode actuation_mode);

  virtual ~Ingenia() noexcept override = default;

  /* Delete copy constructor/assignment since the unique_ptrs cannot be copied */
  Ingenia(const Ingenia&) = delete;
  Ingenia& operator=(const Ingenia&) = delete;

  virtual double getAngleRadAbsolute() override;
  virtual double getAngleRadIncremental() override;
  double getAbsoluteRadPerBit() const;
  double getIncrementalRadPerBit() const;
  bool getIncrementalMorePrecise() const override;
  int16_t getTorque() override;
  int32_t getAngleIUAbsolute();
  int32_t getAngleIUIncremental();
  double getVelocityIUAbsolute();
  double getVelocityIUIncremental();
  virtual double getVelocityRadAbsolute() override;
  virtual double getVelocityRadIncremental() override;

  uint16_t getStatusWord();
  uint16_t getMotionError();
  uint16_t getDetailedError();
  uint16_t getSecondDetailedError();

  ActuationMode getActuationMode() const override;

  virtual float getMotorCurrent() override;
  virtual float getMotorControllerVoltage() override;
  virtual float getMotorVoltage() override;

  MotorControllerStates& getStates() override;

  void setControlWord(uint16_t control_word);
  virtual void actuateRad(double target_rad) override;
  virtual void actuateTorque(int16_t target_torque) override;

  bool initialize(int cycle_time) override;
  void goToTargetState(IngeniaTargetState target_state);
  virtual void prepareActuation() override;

  uint16_t getSlaveIndex() const override;
  virtual void reset() override;

  /** @brief Override comparison operator */
  friend bool operator==(const Ingenia& lhs, const Ingenia& rhs)
  {
    return lhs.getSlaveIndex() == rhs.getSlaveIndex() && *lhs.absolute_encoder_ == *rhs.absolute_encoder_ &&
           *lhs.incremental_encoder_ == *rhs.incremental_encoder_;
  }
  /** @brief Override stream operator for clean printing */
  friend std::ostream& operator<<(std::ostream& os, const Ingenia& imc)
  {
    return os << "slaveIndex: " << imc.getSlaveIndex() << ", "
              << "incrementalEncoder: " << *imc.incremental_encoder_ << ", "
              << "absoluteEncoder: " << *imc.absolute_encoder_;
  }

  constexpr static double MAX_TARGET_DIFFERENCE = 0.393;
  // This value is slightly larger than the current limit of the
  // linear joints defined in the URDF.
  const static int16_t MAX_TARGET_TORQUE = 23500;

  // Watchdog base time = 1 / 25 MHz * (2498 + 2) = 0.0001 seconds=100 µs
  static const uint16_t WATCHDOG_DIVIDER = 2498;
  // 500 * 100us = 50 ms = watchdog timer
  static const uint16_t WATCHDOG_TIME = 500;

protected:
  bool initSdo(SdoSlaveInterface& sdo, int cycle_time) override;

  void reset(SdoSlaveInterface& sdo) override;

private:
  void actuateIU(int32_t target_iu);

  void mapMisoPDOs(SdoSlaveInterface& sdo);
  void mapMosiPDOs(SdoSlaveInterface& sdo);
  /**
   * Initializes all iMC by checking the setup on the drive and writing necessary SDO registers.
   * @param sdo SDO interface to write to
   * @param cycle_time the cycle time of the EtherCAT
   * @return 1 if reset is necessary, otherwise it returns 0
   */
  bool writeInitialSettings(SdoSlaveInterface& sdo, int cycle_time);
  /**
   * Calculates checksum on .sw file passed in string format in sw_string_ by simple summation until next empty line.
   * Start_address and end_address are filled in the method and used for downloading the .sw file to the drive.
   * @param start_address the found start address of the setup in the .sw file
   * @param end_address the found end address of the setup in the .sw file
   * @return the computed checksum in the .sw file
   */
  uint16_t computeSWCheckSum(uint16_t& start_address, uint16_t& end_address);
  /**
   * Compares the checksum of the .sw file and the setup on the drive. If both are equal 1 is returned.
   * This method makes use of the computeSWCheckSum(int&, int&) method. The start and end addresses are used in
   * conjunction with the registers 0x2069 and 0x206A (described in the CoE manual fro Technosoft(2019) in par. 16.2.5
   * and 16.2.6) to determine the checksum on the drive.
   * @return true or 1 if the setup is verified and therefore correct, otherwise returns 0
   */
  bool verifySetup(SdoSlaveInterface& sdo);
  /**
   * Downloads the setup on the .sw file onto the drive using 32bit SDO write functions.
   * The general process is specified in chapter 16.4 of the CoE manual from Technosoft(2019).
   */
  void downloadSetupToDrive(SdoSlaveInterface& sdo);

  // Use of smart pointers are necessary here to make dependency injection
  // possible and thus allow for mocking the encoders. A unique pointer is
  // chosen since the Ingenia should be the owner and the encoders
  // do not need to be passed around.
  std::unique_ptr<AbsoluteEncoder> absolute_encoder_;
  std::unique_ptr<IncrementalEncoder> incremental_encoder_;
  std::string sw_string_;
  ActuationMode actuation_mode_;

  std::unordered_map<IMCObjectName, uint8_t> miso_byte_offsets_;
  std::unordered_map<IMCObjectName, uint8_t> mosi_byte_offsets_;
};

}  // namespace march
#endif  // MARCH_HARDWARE_Ingenia_H
