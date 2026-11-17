#include "omnidrive_stm32/diffbot_system.hpp"

#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

namespace omnidrive_stm32
{
  hardware_interface::CallbackReturn OmniDriveSTM32::on_init(
      const hardware_interface::HardwareInfo &info)
  {
    if (
        hardware_interface::SystemInterface::on_init(info) !=
        hardware_interface::CallbackReturn::SUCCESS)
    {
      return hardware_interface::CallbackReturn::ERROR;
    }

    cfg_.left_wheel_name = info_.hardware_parameters["left_wheel_name"];
    cfg_.right_wheel_name = info_.hardware_parameters["right_wheel_name"];
    cfg_.loop_rate = std::stof(info_.hardware_parameters["loop_rate"]);
    cfg_.device = info_.hardware_parameters["device"];
    cfg_.baud_rate = std::stoi(info_.hardware_parameters["baud_rate"]);
    cfg_.timeout_ms = std::stoi(info_.hardware_parameters["timeout_ms"]);
    cfg_.enc_counts_per_rev = std::stoi(info_.hardware_parameters["enc_counts_per_rev"]);

    wheel_l_.setup(cfg_.left_wheel_name, cfg_.enc_counts_per_rev);
    wheel_r_.setup(cfg_.right_wheel_name, cfg_.enc_counts_per_rev);

    for (const hardware_interface::ComponentInfo &joint : info_.joints)
    {
      if (joint.command_interfaces.size() != 1)
      {
        return hardware_interface::CallbackReturn::ERROR;
      }
      if (joint.state_interfaces.size() != 2)
      {
        return hardware_interface::CallbackReturn::ERROR;
      }
    }

    return hardware_interface::CallbackReturn::SUCCESS;
  }

  std::vector<hardware_interface::StateInterface> OmniDriveSTM32::export_state_interfaces()
  {
    std::vector<hardware_interface::StateInterface> state_interfaces;

    state_interfaces.emplace_back(hardware_interface::StateInterface(
        wheel_l_.name, hardware_interface::HW_IF_POSITION, &wheel_l_.pos));
    state_interfaces.emplace_back(hardware_interface::StateInterface(
        wheel_l_.name, hardware_interface::HW_IF_VELOCITY, &wheel_l_.vel));

    state_interfaces.emplace_back(hardware_interface::StateInterface(
        wheel_r_.name, hardware_interface::HW_IF_POSITION, &wheel_r_.pos));
    state_interfaces.emplace_back(hardware_interface::StateInterface(
        wheel_r_.name, hardware_interface::HW_IF_VELOCITY, &wheel_r_.vel));

    return state_interfaces;
  }

  std::vector<hardware_interface::CommandInterface> OmniDriveSTM32::export_command_interfaces()
  {
    std::vector<hardware_interface::CommandInterface> command_interfaces;

    command_interfaces.emplace_back(hardware_interface::CommandInterface(
        wheel_l_.name, hardware_interface::HW_IF_VELOCITY, &wheel_l_.cmd));

    command_interfaces.emplace_back(hardware_interface::CommandInterface(
        wheel_r_.name, hardware_interface::HW_IF_VELOCITY, &wheel_r_.cmd));

    return command_interfaces;
  }

  hardware_interface::CallbackReturn OmniDriveSTM32::on_configure(
      const rclcpp_lifecycle::State & /*previous_state*/)
  {
    if (comms_.connected())
    {
      comms_.disconnect();
    }
    comms_.connect(cfg_.device, cfg_.baud_rate, cfg_.timeout_ms);
    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::CallbackReturn OmniDriveSTM32::on_cleanup(
      const rclcpp_lifecycle::State & /*previous_state*/)
  {
    if (comms_.connected())
    {
      comms_.disconnect();
    }
    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::CallbackReturn OmniDriveSTM32::on_activate(
      const rclcpp_lifecycle::State & /*previous_state*/)
  {
    if (!comms_.connected())
      comms_.connect(cfg_.device, cfg_.baud_rate, cfg_.timeout_ms);
    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::CallbackReturn OmniDriveSTM32::on_deactivate(
      const rclcpp_lifecycle::State & /*previous_state*/)
  {
    comms_.disconnect();
    return hardware_interface::CallbackReturn::SUCCESS;
  }

  hardware_interface::return_type OmniDriveSTM32::read(
      const rclcpp::Time & /*time*/, const rclcpp::Duration &period)
  {
    if (!comms_.connected())
    {
      return hardware_interface::return_type::ERROR;
    }

    int enc_l = wheel_l_.enc;
    int enc_r = wheel_r_.enc;
    comms_.read_encoder_values(enc_l, enc_r);

    double delta_seconds = period.seconds();
    if (delta_seconds <= 0.001)
    {
      delta_seconds = 0.033;
    }

    static bool first_read = true;
    static int last_raw_l = 0;
    static int last_raw_r = 0;

    if (first_read)
    {
      last_raw_l = enc_l;
      last_raw_r = enc_r;
      wheel_l_.enc = enc_l;
      wheel_r_.enc = enc_r;
      wheel_l_.pos = wheel_l_.calc_enc_angle();
      wheel_r_.pos = wheel_r_.calc_enc_angle();
      wheel_l_.vel = 0.0;
      wheel_r_.vel = 0.0;
      first_read = false;
      return hardware_interface::return_type::OK;
    }

    int diff_l = enc_l - last_raw_l;
    int diff_r = enc_r - last_raw_r;
    last_raw_l = enc_l;
    last_raw_r = enc_r;

    // Aceita variações reais de movimento (até 20000 pulsos por ciclo), descartando apenas corrupção serial extrema
    if (std::abs(diff_l) < 20000 && std::abs(diff_r) < 20000)
    {
      double pos_prev_l = wheel_l_.pos;
      wheel_l_.enc += diff_l;
      wheel_l_.pos = wheel_l_.calc_enc_angle();
      wheel_l_.vel = (wheel_l_.pos - pos_prev_l) / delta_seconds;

      double pos_prev_r = wheel_r_.pos;
      wheel_r_.enc += diff_r;
      wheel_r_.pos = wheel_r_.calc_enc_angle();
      wheel_r_.vel = (wheel_r_.pos - pos_prev_r) / delta_seconds;
    }

    return hardware_interface::return_type::OK;
  }

  hardware_interface::return_type OmniDriveSTM32::write(
      const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
  {
    if (!comms_.connected())
    {
      return hardware_interface::return_type::ERROR;
    }

    int motor_l_counts_per_second = wheel_l_.cmd / wheel_l_.rads_per_count;
    int motor_r_counts_per_second = wheel_r_.cmd / wheel_r_.rads_per_count;

    comms_.set_motor_values(motor_l_counts_per_second, motor_r_counts_per_second);
    return hardware_interface::return_type::OK;
  }

} // namespace omnidrive_stm32

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(
    omnidrive_stm32::OmniDriveSTM32, hardware_interface::SystemInterface)
