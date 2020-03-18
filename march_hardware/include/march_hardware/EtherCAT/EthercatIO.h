// Copyright 2019 Project March.
#ifndef MARCH_HARDWARE_ETHERCAT_ETHERCATIO_H
#define MARCH_HARDWARE_ETHERCAT_ETHERCATIO_H

#include <cstdint>
#include <cstdio>

#include <soem/ethercattype.h>
#include <soem/nicdrv.h>
#include <soem/ethercatbase.h>
#include <soem/ethercatmain.h>
#include <soem/ethercatdc.h>
#include <soem/ethercatcoe.h>
#include <soem/ethercatfoe.h>
#include <soem/ethercatconfig.h>
#include <soem/ethercatprint.h>

namespace march
{
// struct used to easily get specific bytes from a 64 bit variable
struct packed_bit64
{
  unsigned char b0;
  unsigned char b1;
  unsigned char b2;
  unsigned char b3;
  unsigned char b4;
  unsigned char b5;
  unsigned char b6;
  unsigned char b7;
};

// union used for int64 and uint64 in combination with the above struct
union bit64
{
  int64_t i;
  uint64_t ui;
  packed_bit64 p;
};

// struct used to send the x2 variable to the servo drive in the format that is required
struct packed_sd_x2
{
  uint16_t time;
  unsigned char reserved;    // empty but reserved space
  uint8_t integrityCounter;  // represents a 7bit int where the last bit is also part of reserved
};

// struct used to easily get specific bytes from a 32 bit variable
struct packed_bit32
{
  unsigned char b0;
  unsigned char b1;
  unsigned char b2;
  unsigned char b3;
};

// union used for int32, uint32, float and the x2 variable for the servo drive (sd) in combination with the above struct
union bit32
{
  int32_t i;
  uint32_t ui;
  float f;
  packed_sd_x2 ppt;
  packed_bit32 p;
};

// struct used to easily get specific bytes from a 16 bit variable
struct packed_bit16
{
  unsigned char b0;
  unsigned char b1;
};

// union used for int16 and uint16 in combination with the above struct
union bit16
{
  int16_t i;
  uint16_t ui;
  packed_bit16 p;
};

// union used for int8 and uint8 in combination with the above struct, unsigned char is used to read single byte
// unbiased
union bit8
{
  int8_t i;
  uint8_t ui;
  unsigned char b0;
};

// functions to write #bit amounts of data to the EtherCAT train.
// Takes the slave number where 0 is the master and the first slave is 1.
// Takes a module index, which is the variable index for either the input or the output. (nth variable has index n)
union bit32 get_input_bit32(uint16_t slave_no, uint8_t module_index);
void set_output_bit32(uint16_t slave_no, uint8_t module_index, union bit32 value);
union bit32 get_output_bit32(uint16_t slave_no, uint8_t module_index);

union bit16 get_input_bit16(uint16_t slave_no, uint8_t module_index);
void set_output_bit16(uint16_t slave_no, uint8_t module_index, union bit16 value);

union bit8 get_input_bit8(uint16_t slave_no, uint8_t module_index);
void set_output_bit8(uint16_t slave_no, uint8_t module_index, union bit8 value);
union bit8 get_output_bit8(uint16_t slave_no, uint8_t module_index);

void set_output_bit(uint16_t slave_no, uint8_t module_index, uint8_t value);

}  // namespace march
#endif  // MARCH_HARDWARE_ETHERCAT_ETHERCATIO_H
