// Copyright 2019 Project March.
#include <march_hardware/PDOmap.h>

#include <map>
#include <utility>

namespace march
{
std::unordered_map<IMCObjectName, IMCObject> PDOmap::all_objects = {
  { IMCObjectName::StatusWord, IMCObject(0x6041, 16) },
  { IMCObjectName::ActualPosition, IMCObject(0x6064, 32) },
  { IMCObjectName::MotionErrorRegister, IMCObject(0x2000, 16) },
  { IMCObjectName::DetailedErrorRegister, IMCObject(0x2002, 16) },
  { IMCObjectName::DCLinkVoltage, IMCObject(0x2055, 16) },
  { IMCObjectName::DriveTemperature, IMCObject(0x2058, 16) },
  { IMCObjectName::ActualTorque, IMCObject(0x6077, 16) },
  { IMCObjectName::CurrentLimit, IMCObject(0x207F, 16) },
  { IMCObjectName::MotorPosition, IMCObject(0x2088, 32) },
  { IMCObjectName::ControlWord, IMCObject(0x6040, 16) },
  { IMCObjectName::TargetPosition, IMCObject(0x607A, 32) },
  { IMCObjectName::TargetTorque, IMCObject(0x6071, 16) },
  { IMCObjectName::QuickStopDeceleration, IMCObject(0x6085, 32) },
  { IMCObjectName::QuickStopOption, IMCObject(0x605A, 16) }
};

void PDOmap::addObject(IMCObjectName object_name)
{
  if (PDOmap::all_objects.find(object_name) == PDOmap::all_objects.end())
  {
    ROS_WARN("IMC does not exists in in initialised objects so can not be added to PDO");
    return;
  }

  if (this->PDO_objects.count(object_name) != 0)
  {
    ROS_WARN("IMC object %i is already added to PDO map", object_name);
    return;
  }

  this->PDO_objects.insert({ object_name, PDOmap::all_objects[object_name] });  // NOLINT(whitespace/braces)

  int total_used_bits = 0;
  for (const std::pair<IMCObjectName, IMCObject> PDO_object : this->PDO_objects)
  {
    total_used_bits = total_used_bits + PDO_object.second.length;
  }

  if (total_used_bits > this->nr_of_regs * this->bits_per_register)
  {
    ROS_FATAL("Too many objects in PDO Map (total bits %d, only %d allowed), PDO object: %i could not be added",
              total_used_bits, this->nr_of_regs * this->bits_per_register, object_name);
    throw std::exception();
  }
}

std::map<IMCObjectName, int> PDOmap::map(int slave_index, enum dataDirection direction)
{
  if (direction == dataDirection::miso)
  {
    return configurePDO(slave_index, 0x1A00, 0x1C13);
  }
  else if (direction == dataDirection::mosi)
  {
    return configurePDO(slave_index, 0x1600, 0x1C12);
  }
  else
  {
    throw std::runtime_error("Invalid dataDirection argument in PDO mapping");
  }
}

std::map<IMCObjectName, int> PDOmap::configurePDO(int slave_index, int base_register, int base_sync_manager)
{
  int counter = 1;
  int current_register = base_register;
  int size_left = this->bits_per_register;

  std::map<IMCObjectName, int> byte_offsets;
  std::vector<std::pair<IMCObjectName, IMCObject>> sorted_PDO_objects = this->sortPDOObjects();

  sdo_bit8(slave_index, current_register, 0, 0);
  for (const auto& nextObject : sorted_PDO_objects)
  {
    size_left -= nextObject.second.length;
    if (size_left < 0)
    {
      // PDO is filled so it can be enabled again
      sdo_bit8(slave_index, current_register, 0, counter - 1);

      // Update the sync manager with the just configured PDO
      sdo_bit8(slave_index, base_sync_manager, 0, 0);
      int currentPDONr = (current_register - base_register) + 1;
      sdo_bit16(slave_index, base_sync_manager, currentPDONr, current_register);

      // Move to the next PDO register by incrementing with one
      current_register++;
      if (current_register > (base_register + nr_of_regs))
      {
        ROS_ERROR("Amount of registers was overwritten, amount of parameters does not fit in the PDO messages.");
      }

      size_left = this->bits_per_register - nextObject.second.length;
      counter = 1;

      sdo_bit8(slave_index, current_register, 0, 0);
    }

    int byteOffset = (bits_per_register - (size_left + nextObject.second.length)) / 8;
    byte_offsets[nextObject.first] = byteOffset;

    sdo_bit32(slave_index, current_register, counter, nextObject.second.combined_address);
    counter++;
  }

  // Make sure the last PDO is activated
  sdo_bit8(slave_index, current_register, 0, counter - 1);

  // Deactivated the sync manager and configure with the new PDO
  sdo_bit8(slave_index, base_sync_manager, 0, 0);
  int currentPDONr = (current_register - base_register) + 1;
  sdo_bit16(slave_index, base_sync_manager, currentPDONr, current_register);

  // Explicitly disable PDO registers which are not used
  current_register++;
  if (current_register < (base_register + nr_of_regs))
  {
    for (int unusedRegister = current_register; unusedRegister < (base_register + this->nr_of_regs); unusedRegister++)
    {
      sdo_bit8(slave_index, unusedRegister, 0, 0);
    }
  }

  // Activate the sync manager again
  int totalAmountPDO = (current_register - base_register);
  sdo_bit8(slave_index, base_sync_manager, 0, totalAmountPDO);

  return byte_offsets;
}

std::vector<std::pair<IMCObjectName, IMCObject>> PDOmap::sortPDOObjects()
{
  std::vector<std::pair<IMCObjectName, IMCObject>> sorted_PDO_objects;

  for (int objectSize : this->object_sizes)
  {
    for (const auto& object : PDO_objects)
    {
      if (object.second.length == objectSize)
      {
        sorted_PDO_objects.emplace_back(object);
      }
    }
  }
  return sorted_PDO_objects;
}
}  // namespace march
