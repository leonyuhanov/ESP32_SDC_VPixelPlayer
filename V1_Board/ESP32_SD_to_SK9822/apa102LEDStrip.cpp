#include "apa102LEDStrip.h"

apa102LEDStrip::apa102LEDStrip(short int numLEDs, byte bytesPerLED, byte maxValue, byte globalBrightness)
{  
  _numLEDs = numLEDs;
  _bytesPerLED = bytesPerLED;
  _maxValue = maxValue;
  _rainbowSize = maxValue*6;
  _endFrameLength = 1;//round( (numLEDs/2)/8 );
  _frameLength = (1+numLEDs+_endFrameLength)*bytesPerLED;
  _globalBrightness = globalBrightness;
  LEDs = new byte*[_frameLength];
  SPIFrame = new byte[_frameLength];
}

void apa102LEDStrip::renderFrame()
{
  for(_index=0; _index<_frameLength; _index++)
  {
    SPIFrame[_index] = *LEDs[_index];
  }
}

void apa102LEDStrip::setGlobalBrightness(byte globalBrightness)
{
  _globalBrightness = globalBrightness;
  for(_index=_bytesPerLED*2; _index<(_frameLength-(_endFrameLength*_bytesPerLED)); _index+=_bytesPerLED)
  {
    LEDs[_index] = &_globalBrightness;
  }  
}

