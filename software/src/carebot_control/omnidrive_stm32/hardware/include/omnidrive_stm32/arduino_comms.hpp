#ifndef omnidrive_stm32_ARDUINO_COMMS_HPP
#define omnidrive_stm32_ARDUINO_COMMS_HPP

#include <sstream>
#include <libserial/SerialPort.h>
#include <iostream>
#include <algorithm>

LibSerial::BaudRate convert_baud_rate(int baud_rate)
{
  switch (baud_rate)
  {
    case 1200: return LibSerial::BaudRate::BAUD_1200;
    case 1800: return LibSerial::BaudRate::BAUD_1800;
    case 2400: return LibSerial::BaudRate::BAUD_2400;
    case 4800: return LibSerial::BaudRate::BAUD_4800;
    case 9600: return LibSerial::BaudRate::BAUD_9600;
    case 19200: return LibSerial::BaudRate::BAUD_19200;
    case 38400: return LibSerial::BaudRate::BAUD_38400;
    case 57600: return LibSerial::BaudRate::BAUD_57600;
    case 115200: return LibSerial::BaudRate::BAUD_115200;
    case 230400: return LibSerial::BaudRate::BAUD_230400;
    default:
      std::cout << "Error! Baud rate " << baud_rate << " not supported! Default to 57600" << std::endl;
      return LibSerial::BaudRate::BAUD_57600;
  }
}

class ArduinoComms
{
public:
  ArduinoComms() = default;

  void connect(const std::string &serial_device, int32_t baud_rate, int32_t timeout_ms)
  {  
    timeout_ms_ = timeout_ms;
    serial_conn_.Open(serial_device);
    serial_conn_.SetBaudRate(convert_baud_rate(baud_rate));
    serial_conn_.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);
    serial_conn_.SetStopBits(LibSerial::StopBits::STOP_BITS_1);
    serial_conn_.SetParity(LibSerial::Parity::PARITY_NONE);
  }

  void disconnect()
  {
    serial_conn_.Close();
  }

  bool connected() const
  {
    return serial_conn_.IsOpen();
  }

  void read_encoder_values(int &val_1, int &val_2)
  {
    // Lê instantaneamente todos os bytes que já chegaram da STM32 sem travar o loop de 30Hz
    while (serial_conn_.IsDataAvailable())
    {
      char c = 0;
      try
      {
        serial_conn_.ReadByte(c, 1);
      }
      catch (const LibSerial::ReadTimeout&)
      {
        break;
      }

      // Ignora bytes nulos (\0) de preenchimento do buffer de 40 bytes da STM32 e \r
      if (c == '\0' || c == '\r')
      {
        continue;
      }

      if (c == '\n')
      {
        if (!rx_line_.empty())
        {
          size_t del_pos1 = rx_line_.find(' ');
          if (del_pos1 != std::string::npos)
          {
            std::string token_1 = rx_line_.substr(0, del_pos1);
            std::string token_2 = rx_line_.substr(del_pos1 + 1);
            val_1 = std::atoi(token_1.c_str());
            val_2 = std::atoi(token_2.c_str());
          }
          rx_line_.clear();
        }
      }
      else
      {
        rx_line_ += c;
        if (rx_line_.size() > 64)
        {
          rx_line_.clear();
        }
      }
    }
  }

  void set_motor_values(int val_1, int val_2)
  {
    char msg[40] = {0};
    sprintf(msg, "m %d %d\n", val_1, val_2);
    serial_conn_.Write(std::string(msg));
  }

private:
  LibSerial::SerialPort serial_conn_;
  int timeout_ms_;
  std::string rx_line_ = "";
};

#endif // omnidrive_stm32_ARDUINO_COMMS_HPP