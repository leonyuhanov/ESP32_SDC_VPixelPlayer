#ifndef apa102LEDStrip_h
#define apa102LEDStrip_h
#include "Arduino.h"

class apa102LEDStrip
{
  public:
    apa102LEDStrip(short int numLEDs, byte bytesPerLED, byte maxValue, byte globalBrightness);
    void renderFrame();
    void setGlobalBrightness(byte globalBrightness);

    byte **LEDs;
    byte* SPIFrame;
    
    short int _rainbowSize;
    short int _frameLength;
    short int _endFrameLength;
    short int _numLEDs;
    byte _bytesPerLED;
    short int _maxValue;
    short int _counter;
    byte _fullByte=255;
    byte _emptyByte=0;
    byte _globalBrightness;
    
  private:
  unsigned short int _index;
};

#endif
