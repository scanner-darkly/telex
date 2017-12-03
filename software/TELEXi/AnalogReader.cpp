/*
 * TELEXi Eurorack Module
 * (c) 2016 Brendon Cassidy
 * MIT License
 */
 
#include "Arduino.h"
#include "AnalogReader.h"

#include <ResponsiveAnalogRead.h>

/*
 * simple constructor for unipolar things (like pots)
 */
AnalogReader::AnalogReader(int address){
  AnalogReader(address, false);
}

/*
 * advanced constructor for inverted, bipolar things (like CV)
 */
AnalogReader::AnalogReader(int address, bool reverse){
  _address = address;
  _reverse = reverse;
  if (!_reverse) _bottom = 0;
  
  _analog = new ResponsiveAnalogRead(0, false);
  _analog->setAnalogResolution(1<<13);
  
  _calibrationData[0] = -16384;
  _calibrationData[1] = 0;
  _calibrationData[2] = 16383;
}

/*
 *  reads the analog input
 */
int FASTRUN AnalogReader::Read() {
  // read the value from the pin
  _readValue = analogRead(_address) >> 3;
  _analog->update(_readValue);
  _readValue = _analog->getValue();

  // shift it, flip it and reverse it (_reverse is for CV)
  _readValue = _readValue << (_reverse ? 2 : 1);
  if (_reverse) _readValue = 16383 - _readValue;
  
  // scale if this input is actively calibrated
  if (_calibrated) _readValue = Scale(_readValue);

  // map it (if we are mapping values)
  if (_map){
    // Serial.printf("MAP %d",_readValue);
    _readValue = map(_readValue, _reverse ? BOTTOM : 0, TOP, _bottom, _top+1);
    // Serial.printf(" = %d\n",_readValue);
  }

  // store as latest value and return
  _latestValue = _readValue;
  return _latestValue;
  
}

/*
 * retuns the latest read value
 */
int AnalogReader::GetLatest() {
  return _latestValue;
}


void AnalogReader::SetTop(int top){
  SetMap(top, _bottom);
}

void AnalogReader::SetBottom(int bottom){
  SetMap(_top, bottom);
}

void AnalogReader::SetMap(int top, int bottom){
  if (_top != top) _top = top;
  if (_bottom != bottom) _bottom = bottom;
  _map = _top != TOP || _bottom != (_reverse ? BOTTOM : 0);
}

/*
 * returns if calibration is active or not
 */
bool AnalogReader::GetCalibrated(){
  return _calibrated;
}

/*
 * manually set the calibration state
 */
void AnalogReader::SetCalibrated(bool calibrated){
  _calibrated = calibrated;
}

/*
 * perform a calibration for a given measure
 * measure can be less than zero, zero, or greater than zero
 */
void AnalogReader::Calibrate(int measure){

    // set this reader as calibrated
    _calibrated = true;

    // pull the accumulated value
    int value = _analog->getValue();
    
    // flip and reverse
    value = value << (_reverse ? 2 : 1);
    if (_reverse) value = 16383 - value;

    // Serial.printf("CALIB [%d][[%d]]: %d\n", _address, measure, value);

    // set the calibration data
    // removed value protections so that fun things can be done with scaling
    if (measure == 0) {
      _calibrationData[1] = value;
    } else if (measure > 0) {
      _calibrationData[2] = value;
    } else if (measure < 0){
      _calibrationData[0] = value;
    }

}

/*
 * returns the calibration data (for eeprom parameter storage)
 */
void AnalogReader::GetCalibrationData(int data[3]){
  for (_i=0; _i < 3; _i++) data[_i] = _calibrationData[_i];
}

/*
 * manually sets the calibration data (for eeprom parameter retrieval)
 */
void AnalogReader::SetCalibrationData(int measure, int value){
  _calibrationData[measure] = value;
}


/*
 * performs the scaling function based on calibration
 */
int FASTRUN AnalogReader::Scale(int value) {

  // map the polar values
  if (value >= _calibrationData[1]){
    value = map(value, _calibrationData[1], _calibrationData[2], 0, 16383);
  } else {
    value = map(value, _calibrationData[0], _calibrationData[1], -16384, 0);
  }

  // constrain to our TI range
  value = constrain(value, -16384, 16383);
  
  return value;
}

