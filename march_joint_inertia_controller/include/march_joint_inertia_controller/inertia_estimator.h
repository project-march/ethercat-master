// Copyright 2020 Project March.

#ifndef MARCH_WS_INERTIA_ESTIMATOR_H
#define MARCH_WS_INERTIA_ESTIMATOR_H

#include <hardware_interface/joint_command_interface.h>
#include <ros/ros.h>
#include <trajectory_interface/quintic_spline_segment.h>
#include <urdf/model.h>

/**
 * \brief determines the absolute value of vector a and stores it into b so that a remains unchanged
 *
 * @param[in] a input vector with values that need to be converted to absolutes
 * @param[out] b output vector with copies of the absolute values of a.
 */
void absolute(std::vector<double> a, std::vector<double>& b);

/**
 * \brief determiness the mean value of the given vector
 *
 * @param[in] a input vector from which the mean gets calculated
 * @returns the mean value of a
 */
double mean(std::vector<double> a);

/**
 * \brief Class that bundles functionality to estimate inertia on a Revolute Joint.
 *
 * @param[in] joint the jointhandle used for obtaining current speed and effort.
 * @param[in] lambda sets the lambda used in the RLS filter defaulted to 0.96
 * @param[in] acc_size the size of the vector wherein the calculated acceleration is stored. This vector length is also
 *  used in determining the length of the velocity vector and the filtered acceleration vector. It is of specific
 *  size to calculate the alpha value. Defaulted to 12.
 */
class InertiaEstimator
{
public:
  /**
   * \brief Default constructor
   */
  InertiaEstimator(double lambda = 0.96, size_t acc_size = 12);

  /**
   * \brief Constructor that accepts a joint
   */
  InertiaEstimator(hardware_interface::JointHandle joint, double lambda = 0.96, size_t acc_size = 12);

  /**
   * \brief Destructor
   */
  ~InertiaEstimator();

  /**
   * \brief Get the position of the joint stored in the private variable
   */
  double getPosition();

  void setJoint(hardware_interface::JointHandle joint);
  void setLambda(double lambda);
  void setAcc_size(size_t acc_size);
  void setNodeHandle(ros::NodeHandle& nh);
  /**
   * \brief Initialize the publisher with a name
   */
  void configurePublisher(std::string name);
  void publishInertia();

  std::string getJointName();

  /**
   * \brief Applies the Butterworth filter over the last two samples and returns the resulting filtered value
   */
  void apply_butter();

  /**
   * \brief Estimate the inertia using the acceleration and torque
   */
  void inertia_estimate();
  /**
   * \brief Calculate a discrete derivative of the speed measurements
   */
  void discrete_speed_derivative(double velocity, const ros::Duration&);
  /**
   * \brief Calculate the alpha coefficient for the inertia estimate
   */
  double alpha_calculation();
  /**
   * \brief Calculate the inertia gain for the inertia estimate
   */
  double gain_calculation();
  /**
   * \brief Calculate the correlation coefficient of the acceleration buffer
   */
  void correlation_calculation();
  /**
   * \brief Calculate the vibration based on the acceleration
   */
  double vibration_calculation();

  /**
   * \brief Fill the rolling buffers with corresponding values.
   */
  bool fill_buffers(double velocity, double effort, const ros::Duration& period);

  urdf::JointConstSharedPtr joint_urdf_;
  std::string joint_name;

private:
  ros::NodeHandle nh_;
  ros::Publisher pub_;

  hardware_interface::JointHandle joint_;

  float min_alpha_ = 0.4;  // You might want to be able to adjust this value from a yaml/launch file
  float max_alpha_ = 0.9;  // You might want to be able to adjust this value from a yaml/launch file

  double sos_[3][6] = {
    { 2.31330497e-05, 4.62660994e-05, 2.31330497e-05, 1.00000000e+00, -1.37177561e+00, 4.75382129e-01 },
    { 1.00000000e+00, 2.00000000e+00, 1.00000000e+00, 1.00000000e+00, -1.47548044e+00, 5.86919508e-01 },
    { 1.00000000e+00, 2.00000000e+00, 1.00000000e+00, 1.00000000e+00, -1.69779140e+00, 8.26021017e-01 }
  };
  // namespace joint_inertia_controller

  // Default length 12 for the alpha calculation
  size_t acc_size_;
  std::vector<double> acceleration_array_;
  // Equal to atwo, because two values are needed to calculate the derivative
  size_t vel_size_ = 2;
  std::vector<double> velocity_array_;
  std::vector<double> filtered_acceleration_array_;
  // Of length 3 for the butterworth filter
  size_t torque_size_ = 2;
  std::vector<double> joint_torque_;
  // Of length 2 for the error calculation
  size_t fil_tor_size_ = 2;
  std::vector<double> filtered_joint_torque_;

  // Correlation coefficient used to calculate the inertia gain
  double corr_coeff_;
  double K_a_;
  double K_i_;
  double moa_;
  double aom_;
  double joint_inertia_;
  double lambda_;
};

#endif  // MARCH_WS_INERTIA_ESTIMATOR_H
