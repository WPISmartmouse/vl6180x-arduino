#include <VL6180X.h>
#include <Wire.h>

// Defines /////////////////////////////////////////////////////////////////////


// Constructors ////////////////////////////////////////////////////////////////

VL6180X::VL6180X(int _enablePin, uint8_t new_addr)
{
  address = new_addr;
  enable_pin = _enablePin;
  io_timeout = 0;  // 0 = no timeout
  did_timeout = false;
  lastReadTime = 0;
  lastReading = 0;
  measurement_period = 0;
}

// Public Methods //////////////////////////////////////////////////////////////

void VL6180X::setAddress(uint8_t new_addr)
{
  writeReg(I2C_SLAVE__DEVICE_ADDRESS, new_addr & 0x7F);
  address = new_addr;
}

// Initialize sensor with settings from ST application note AN4545, section 9 -
// "Mandatory : private registers"
void VL6180X::init()
{
  writeReg(0x207, 0x01);
  writeReg(0x208, 0x01);
  writeReg(0x096, 0x00);
  writeReg(0x097, 0xFD);
  writeReg(0x0E3, 0x00);
  writeReg(0x0E4, 0x04);
  writeReg(0x0E5, 0x02);
  writeReg(0x0E6, 0x01);
  writeReg(0x0E7, 0x03);
  writeReg(0x0F5, 0x02);
  writeReg(0x0D9, 0x05);
  writeReg(0x0DB, 0xCE);
  writeReg(0x0DC, 0x03);
  writeReg(0x0DD, 0xF8);
  writeReg(0x09F, 0x00);
  writeReg(0x0A3, 0x3C);
  writeReg(0x0B7, 0x00);
  writeReg(0x0BB, 0x3C);
  writeReg(0x0B2, 0x09);
  writeReg(0x0CA, 0x09);
  writeReg(0x198, 0x01);
  writeReg(0x1B0, 0x17);
  writeReg(0x1AD, 0x00);
  writeReg(0x0FF, 0x05);
  writeReg(0x100, 0x05);
  writeReg(0x199, 0x05);
  writeReg(0x1A6, 0x1B);
  writeReg(0x1AC, 0x3E);
  writeReg(0x1A7, 0x1F);
  writeReg(0x030, 0x00);
}

int VL6180X::initMouse(){
  digitalWrite(enable_pin, LOW);
  pinMode(enable_pin, INPUT); // pull up by an ext resistor to 2.8V

  delay(1);

  // Check if the address is already changed
  Wire.beginTransmission(address); 
  int e = Wire.endTransmission();
  
  if (e != 0) { // No device found on this address
    int tempAddress = address;    
    address = 0x29; // temporarily set to default address

    Wire.beginTransmission(address); 
    int e2 = Wire.endTransmission();

    if (e2 != 0) return -1; // I CANT FIND IT

    setAddress(tempAddress);

    Wire.beginTransmission(address); 
    int e3 = Wire.endTransmission();

    if (e3 != 0) return -2; // I CANT FIND IT AFTER SETTING THE ADDRESS
  } 

  init();

  // "Recommended : Public registers"

  // readout__averaging_sample_period = 48
  writeReg(READOUT__AVERAGING_SAMPLE_PERIOD, 0x30);

  // sysals__analogue_gain_light = 6 (ALS gain = 1 nominal, actually 1.01 according to Table 14 in datasheet)
  writeReg(SYSALS__ANALOGUE_GAIN, 0x46);

  // sysrange__vhv_repeat_rate = 255 (auto Very High Voltage temperature recalibration after every 255 range measurements)
  writeReg(SYSRANGE__VHV_REPEAT_RATE, 0xFF);

  // sysals__integration_period = 99 (100 ms)
  // AN4545 incorrectly recommends writing to register 0x040; 0x63 should go in the lower byte, which is register 0x041.
  writeReg16Bit(SYSALS__INTEGRATION_PERIOD, 0x0063);

  // sysrange__vhv_recalibrate = 1 (manually trigger a VHV recalibration)
  writeReg(SYSRANGE__VHV_RECALIBRATE, 0x01);


  // "Optional: Public registers"

  // sysrange__intermeasurement_period = 9 (100 ms)
  writeReg(SYSRANGE__INTERMEASUREMENT_PERIOD, 0x05);

  // sysals__intermeasurement_period = 49 (500 ms)
  writeReg(SYSALS__INTERMEASUREMENT_PERIOD, 0x31);

  // als_int_mode = 4 (ALS new sample ready interrupt); range_int_mode = 4 (range new sample ready interrupt)
  writeReg(SYSTEM__INTERRUPT_CONFIG_GPIO, 0x24);


  // Reset other settings to power-on defaults

  // sysrange__max_convergence_time = 49 (49 ms)
  writeReg(VL6180X::SYSRANGE__MAX_CONVERGENCE_TIME, 15);

  // disable interleaved mode
  writeReg(INTERLEAVED_MODE__ENABLE, 0);

  return 0;

}


// Configure some settings for the sensor's default behavior from AN4545 -
// "Recommended : Public registers" and "Optional: Public registers"
//
// Note that this function does not set up GPIO1 as an interrupt output as
// suggested, though you can do so by calling:
// writeReg(SYSTEM__MODE_GPIO1, 0x10);
void VL6180X::configureDefault(void)
{
  // "Recommended : Public registers"

  // readout__averaging_sample_period = 48
  writeReg(READOUT__AVERAGING_SAMPLE_PERIOD, 0x30);

  // sysals__analogue_gain_light = 6 (ALS gain = 1 nominal, actually 1.01 according to Table 14 in datasheet)
  writeReg(SYSALS__ANALOGUE_GAIN, 0x46);

  // sysrange__vhv_repeat_rate = 255 (auto Very High Voltage temperature recalibration after every 255 range measurements)
  writeReg(SYSRANGE__VHV_REPEAT_RATE, 0xFF);

  // sysals__integration_period = 99 (100 ms)
  // AN4545 incorrectly recommends writing to register 0x040; 0x63 should go in the lower byte, which is register 0x041.
  writeReg16Bit(SYSALS__INTEGRATION_PERIOD, 0x0063);

  // sysrange__vhv_recalibrate = 1 (manually trigger a VHV recalibration)
  writeReg(SYSRANGE__VHV_RECALIBRATE, 0x01);


  // "Optional: Public registers"

  // sysrange__intermeasurement_period = 9 (100 ms)
  writeReg(SYSRANGE__INTERMEASUREMENT_PERIOD, 0x09);

  // sysals__intermeasurement_period = 49 (500 ms)
  writeReg(SYSALS__INTERMEASUREMENT_PERIOD, 0x31);

  // als_int_mode = 4 (ALS new sample ready interrupt); range_int_mode = 4 (range new sample ready interrupt)
  writeReg(SYSTEM__INTERRUPT_CONFIG_GPIO, 0x24);


  // Reset other settings to power-on defaults

  // sysrange__max_convergence_time = 49 (49 ms)
  writeReg(VL6180X::SYSRANGE__MAX_CONVERGENCE_TIME, 0x31);

  // disable interleaved mode
  writeReg(INTERLEAVED_MODE__ENABLE, 0);
}

// Writes an 8-bit register
void VL6180X::writeReg(uint16_t reg, uint8_t value)
{
  Wire.beginTransmission(address);
  Wire.write((reg >> 8) & 0xff);  // reg high byte
  Wire.write(reg & 0xff);         // reg low byte
  Wire.write(value);
  last_status = Wire.endTransmission();
}

// Writes a 16-bit register
void VL6180X::writeReg16Bit(uint16_t reg, uint16_t value)
{
  Wire.beginTransmission(address);
  Wire.write((reg >> 8) & 0xff);  // reg high byte
  Wire.write(reg & 0xff);         // reg low byte
  Wire.write((value >> 8) & 0xff);  // value high byte
  Wire.write(value & 0xff);         // value low byte
  last_status = Wire.endTransmission();
}

// Writes a 32-bit register
void VL6180X::writeReg32Bit(uint16_t reg, uint32_t value)
{
  Wire.beginTransmission(address);
  Wire.write((reg >> 8) & 0xff);  // reg high byte
  Wire.write(reg & 0xff);         // reg low byte
  Wire.write((value >> 24) & 0xff); // value highest byte
  Wire.write((value >> 16) & 0xff);
  Wire.write((value >> 8) & 0xff);
  Wire.write(value & 0xff);         // value lowest byte
  last_status = Wire.endTransmission();
}

// Reads an 8-bit register
uint8_t VL6180X::readReg(uint16_t reg)
{
  uint8_t value;

  Wire.beginTransmission(address);
  Wire.write((reg >> 8) & 0xff);  // reg high byte
  Wire.write(reg & 0xff);         // reg low byte
  last_status = Wire.endTransmission();

  Wire.requestFrom(address, (uint8_t)1);
  value = Wire.read();
  Wire.endTransmission();

  return value;
}

// Reads a 16-bit register
uint16_t VL6180X::readReg16Bit(uint16_t reg)
{
  uint16_t value;

  Wire.beginTransmission(address);
  Wire.write((reg >> 8) & 0xff);  // reg high byte
  Wire.write(reg & 0xff);         // reg low byte
  last_status = Wire.endTransmission();

  Wire.requestFrom(address, (uint8_t)2);
  value = (uint16_t)Wire.read() << 8; // value high byte
  value |= Wire.read();               // value low byte
  Wire.endTransmission();

  return value;
}

// Reads a 32-bit register
uint32_t VL6180X::readReg32Bit(uint16_t reg)
{
  uint32_t value;

  Wire.beginTransmission(address);
  Wire.write((reg >> 8) & 0xff);  // reg high byte
  Wire.write(reg & 0xff);         // reg low byte
  last_status = Wire.endTransmission();

  Wire.requestFrom(address, (uint8_t)4);
  value = (uint32_t)Wire.read() << 24;  // value highest byte
  value |= (uint32_t)Wire.read() << 16;
  value |= (uint16_t)Wire.read() << 8;
  value |= Wire.read();                 // value lowest byte
  Wire.endTransmission();

  return value;
}

// Performs a single-shot ranging measurement
uint8_t VL6180X::readRangeSingle()
{
  writeReg(SYSRANGE__START, 0x01);
  return readRangeContinuous();
}

// Performs a single-shot ambient light measurement
uint16_t VL6180X::readAmbientSingle()
{
  writeReg(SYSALS__START, 0x01);
  return readAmbientContinuous();
}

// Starts continuous ranging measurements with the given period in ms
// (10 ms resolution; defaults to 100 ms if not specified).
//
// The period must be greater than the time it takes to perform a
// measurement. See section 2.4.4 ("Continuous mode limits") in the datasheet
// for details.
void VL6180X::startRangeContinuous(uint16_t period)
{

  writeReg(SYSRANGE__START, 0x01);
  delay(50);

  int16_t period_reg = (int16_t)(period / 10) - 1;
  period_reg = constrain(period_reg, 0, 254);

  writeReg(SYSRANGE__INTERMEASUREMENT_PERIOD, period_reg);
  writeReg(SYSRANGE__START, 0x03);

  measurement_period = (long) period;
}

// Starts continuous ambient light measurements with the given period in ms
// (10 ms resolution; defaults to 500 ms if not specified).
//
// The period must be greater than the time it takes to perform a
// measurement. See section 2.4.4 ("Continuous mode limits") in the datasheet
// for details.
void VL6180X::startAmbientContinuous(uint16_t period)
{
  int16_t period_reg = (int16_t)(period / 10) - 1;
  period_reg = constrain(period_reg, 0, 254);

  writeReg(SYSALS__INTERMEASUREMENT_PERIOD, period_reg);
  writeReg(SYSALS__START, 0x03);
}

// Starts continuous interleaved measurements with the given period in ms
// (10 ms resolution; defaults to 500 ms if not specified). In this mode, each
// ambient light measurement is immediately followed by a range measurement.
//
// The datasheet recommends using this mode instead of running "range and ALS
// continuous modes simultaneously (i.e. asynchronously)".
//
// The period must be greater than the time it takes to perform both
// measurements. See section 2.4.4 ("Continuous mode limits") in the datasheet
// for details.
void VL6180X::startInterleavedContinuous(uint16_t period)
{
  int16_t period_reg = (int16_t)(period / 10) - 1;
  period_reg = constrain(period_reg, 0, 254);

  measurement_period = (long) period;

  writeReg(INTERLEAVED_MODE__ENABLE, 1);
  writeReg(SYSALS__INTERMEASUREMENT_PERIOD, period_reg);
  writeReg(SYSALS__START, 0x03);
}

// Stops continuous mode. This will actually start a single measurement of range
// and/or ambient light if continuous mode is not active, so it's a good idea to
// wait a few hundred ms after calling this function to let that complete
// before starting continuous mode again or taking a reading.
void VL6180X::stopContinuous()
{

  writeReg(SYSRANGE__START, 0x01);
  writeReg(SYSALS__START, 0x01);

  writeReg(INTERLEAVED_MODE__ENABLE, 0);
}

// Returns a range reading when continuous mode is activated
// (readRangeSingle() also calls this function after starting a single-shot
// range measurement)
uint8_t VL6180X::readRangeContinuous()
{
  /*
  uint16_t millis_start = millis();
  while ((readReg(RESULT__INTERRUPT_STATUS_GPIO) & 0x04) == 0)
  {
    if (io_timeout > 0 && ((uint16_t)millis() - millis_start) > io_timeout)
    {
      did_timeout = true;
      return 255;
    }
  }
  */
  long now = millis();
  if (now - lastReadTime > measurement_period){
    lastReadTime = now;
    lastReading = readReg(RESULT__RANGE_VAL);
  }

  //writeReg(SYSTEM__INTERRUPT_CLEAR, 0x01);

  return lastReading;
}

// Returns an ambient light reading when continuous mode is activated
// (readAmbientSingle() also calls this function after starting a single-shot
// ambient light measurement)
uint16_t VL6180X::readAmbientContinuous()
{
  uint16_t millis_start = millis();
  while ((readReg(RESULT__INTERRUPT_STATUS_GPIO) & 0x20) == 0)
  {
    if (io_timeout > 0 && ((uint16_t)millis() - millis_start) > io_timeout)
    {
      did_timeout = true;
      return 0;
    }
  }

  uint16_t ambient = readReg16Bit(RESULT__ALS_VAL);
  writeReg(SYSTEM__INTERRUPT_CLEAR, 0x02);

  return ambient;
}

// Did a timeout occur in one of the read functions since the last call to
// timeoutOccurred()?
bool VL6180X::timeoutOccurred()
{
  bool tmp = did_timeout;
  did_timeout = false;
  return tmp;
}

void VL6180X::setTimeout(uint16_t timeout)
{
  io_timeout = timeout;
}

uint16_t VL6180X::getTimeout()
{
  return io_timeout;
}