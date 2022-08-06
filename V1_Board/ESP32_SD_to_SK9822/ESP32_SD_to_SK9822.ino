/*
Micro CD Card PinOut for the Wemos D32 Pro as taken from 
https://www.wemos.cc/en/latest/_static/files/sch_d32_pro_v2.0.0.pdf

NS		1	NC
/CS		2	IO_13	TF_CS
DI		3	IO_15	MOSI
VDD		4	3.3V
CLK		5	IO_14	SCK
VSS		6	GND
D0		7	IO_02	MISO
RSV		8	NC
CD(1)	G	GND

PINOUT for APA102

HSPI CLK	IO_18	->	SN74HCT245N	->	CLOCK PIN of APA102
HSPI MOSI	IO_23	->	SN74HCT245N	->	DATA PIN of APA102
 */

#include <WiFi.h>
#include "SPI.h"
#include "apa102LEDStrip.h"
#include "FS.h"
#include "SD.h"
#include <string>
#include "upng.h"
#include "animationQue.h"

//Bitmaping Data
const short int numLeds = 255;
const byte bytesPerLed = 4;
const byte rows = 15, cols = 17;
byte bitmap[rows][cols][bytesPerLed-1];

//LED MAP
const short int maskMap[rows][cols]= {{14,15,44,45,74,75,104,105,134,135,164,165,194,195,224,225,254},{13,16,43,46,73,76,103,106,133,136,163,166,193,196,223,226,253},{12,17,42,47,72,77,102,107,132,137,162,167,192,197,222,227,252},{11,18,41,48,71,78,101,108,131,138,161,168,191,198,221,228,251},{10,19,40,49,70,79,100,109,130,139,160,169,190,199,220,229,250},{9,20,39,50,69,80,99,110,129,140,159,170,189,200,219,230,249},{8,21,38,51,68,81,98,111,128,141,158,171,188,201,218,231,248},{7,22,37,52,67,82,97,112,127,142,157,172,187,202,217,232,247},{6,23,36,53,66,83,96,113,126,143,156,173,186,203,216,233,246},{5,24,35,54,65,84,95,114,125,144,155,174,185,204,215,234,245},{4,25,34,55,64,85,94,115,124,145,154,175,184,205,214,235,244},{3,26,33,56,63,86,93,116,123,146,153,176,183,206,213,236,243},{2,27,32,57,62,87,92,117,122,147,152,177,182,207,212,237,242},{1,28,31,58,61,88,91,118,121,148,151,178,181,208,211,238,241},{0,29,30,59,60,89,90,119,120,149,150,179,180,209,210,239,240}};
const short int deadValue=-1;

//Colour Data
byte tempColour[3] = {0,0,0};
const byte maxBrightness = 255;

//Counters
unsigned short int cIndex=0, secondaryCIndex=0;
unsigned short int yCnt=0, xCnt=0;

//LEDs
apa102LEDStrip leds = apa102LEDStrip(numLeds, bytesPerLed, 64, maxBrightness);

//File Paths
char playListFile[] = "/playlist";
char integerCharString[] = "00000";

//PNG stuff
upng_t* upng;
unsigned short int xCount, yCount, imageSize, bufferIndex=0, testCnt=0;

//SPI
SPIClass HSPIPort(HSPI);
uint8_t cardType=0;
long cardSize=0;

//Pre Processor
File outputStream;
animationQue globalAnimationQue;

void setup()
{
  //Turn off WIFI we do not need it here
  WiFi.mode(WIFI_OFF); 
  
  //Init Serial
  Serial.begin(115200);
  Serial.printf("\r\n\r\n\r\nSystem Booting....\r\n");
  //Init File System
  initFS();

  //Populate Animation List from SD Card file /playlist
  compilePlaylist();
  
  //Pre Process Multiple Frames into 1 File Stream
  compileStream();
  
  //Start SPI
  //SPI.begin();
  SPI.begin(18, 19, 23, 04);
  SPI.setBitOrder(MSBFIRST);
  SPI.setFrequency(10000000);  
  
  //Set up the pixel map
  assignMapToLEDArray();
  
  //Clear bitmap memory
  clearBitmap();
  //Render black twice
  renderLEDs();
  renderLEDs();
}

void initFS()
{
  Serial.printf("\r\n\tSetting up SD Card FS...");
  
  HSPIPort.begin(14, 2, 15, 13);
  HSPIPort.setFrequency(40000000);
  if(! SD.begin(SS, HSPIPort, 40000000, "/sd", 5, false) )
  {
    Serial.printf("\r\n\tSD Card Mount Failed...");
    while(true)
    {
      yield();
    }
  }
  cardType = SD.cardType();
  if(cardType == CARD_NONE)
  {
    Serial.printf("\r\n\tNo SD card attached!");
    return;
  }
}

void compilePlaylist()
{
  char* tempAnimationName;
  char* tempAnimationLength;
  char* tempAnimationDelay;
  char* playListFileBuffer;
  unsigned short int animationCount=0;
  unsigned short int aCnt=0, startI=0, endI=0, needleIndex=0, tempLength=0;
  PLNODE* queLocater;
  
  outputStream = SD.open(playListFile, "r");
  playListFileBuffer = new char[ outputStream.size() ];
  outputStream.readBytes(playListFileBuffer, outputStream.size());
  outputStream.close();

  //Grab number of animations in playlist
  animationCount = countNeedles(playListFileBuffer, ';');
  Serial.printf("\r\nAnimation Count in playlist file\t[%d]", animationCount);
  
  for(aCnt=0; aCnt<animationCount; aCnt++)
  {
    //Serial.printf("\r\nADD..");
    //locate 1st ','  animationName
    needleIndex++;
    endI = findNeedleCount(playListFileBuffer, ',', needleIndex);
    tempLength = endI-startI;
    tempAnimationName = new char[ tempLength+1 ];
    clearCharArray(tempAnimationName);
    memcpy(tempAnimationName, playListFileBuffer+startI, tempLength);
    tempAnimationName[tempLength]='\0';
    //locate 2nd ','  frameCount
    startI = endI+1;
    needleIndex++;
    endI = findNeedleCount(playListFileBuffer, ',', needleIndex);
    tempLength = endI-startI;
    tempAnimationLength = new char[ tempLength+1 ];
    clearCharArray(tempAnimationLength);
    memcpy(tempAnimationLength, playListFileBuffer+startI, tempLength);
    tempAnimationLength[tempLength]='\0';
    startI = endI+1;
    //locate 3rd ';'  delay
    endI = findNeedleCount(playListFileBuffer, ';', aCnt+1);
    tempLength = endI-startI;
    tempAnimationDelay = new char[ tempLength+1 ];
    clearCharArray(tempAnimationDelay);
    memcpy(tempAnimationDelay, playListFileBuffer+startI, tempLength);
    tempAnimationDelay[tempLength]='\0';
    startI = endI+1;
    globalAnimationQue.add(tempAnimationName, atoi(tempAnimationLength), atoi(tempAnimationDelay));
    //clear memory
    delete[] tempAnimationName;
    delete[] tempAnimationLength;
    delete[] tempAnimationDelay;
  }
  
  
  //Dump Animations
  Serial.printf("\r\n\tTotal Nodes\t%d", globalAnimationQue.totalNodes);
  for(aCnt=0; aCnt<globalAnimationQue.totalNodes; aCnt++)
  {
    queLocater = globalAnimationQue.findByID(aCnt);
    if(queLocater!=NULL)
    {
      Serial.printf("\r\nAnimation\t[%d][%d]\tName[", aCnt, queLocater->_nodeID );
      Serial.print(queLocater->_animationName);
      Serial.printf("]\tFolder Path[");
      Serial.print(queLocater->_folderPath);
      Serial.printf("]\tFull Path[");
      Serial.print(queLocater->_fullPath);
      Serial.printf("]\tFrame Count[");
      Serial.print(queLocater->_totalFrames);
      Serial.printf("]\tFrame Delay[");
      Serial.print(queLocater->_frameDelay);
      Serial.printf("]");
    }
  }

}
void clearCharArray(char* charArray)
{
  unsigned short int charCount=0;
  for(charCount=0; charCount<strlen(charArray); charCount++)
  {
    charArray[charCount]=0;
  }
}
short int findNeedleCount(char* haystack, char needle, unsigned short needleCount)
{
  unsigned short int hayCount=0, nCount=0;
  for(hayCount; hayCount<strlen(haystack); hayCount++)
  {
    if(haystack[hayCount]==needle)
    {
      nCount++;
      if(nCount==needleCount)
      {
        return hayCount;
      }
    }
  }
  return -1;
}
short int countNeedles(char* haystack, char needle)
{
  unsigned short int found;
  unsigned short int hayCount=0;
  
  for(hayCount; hayCount<strlen(haystack); hayCount++)
  {
    if(haystack[hayCount]==needle)
    {
      found++;
    }
  }
  return found;
}

void compileStream()
{
  unsigned short int plCounter=0, postFrameCounter=0, charArrayIndex=0, bytesWriten=0;
  byte dirExists=0, streamExists=0;
  char * tempPathArray;
  char endOfFileChars[] = "_00000.png";
  PLNODE* queLocater;
  
  for(plCounter=0; plCounter<globalAnimationQue.totalNodes; plCounter++)
  {
    queLocater = globalAnimationQue.findByID(plCounter);
    if(queLocater!=NULL)
    {
      //Details of animationa re in 
      //  queLocater->_animationName
      //  queLocater->_fullPath     - full path of stream file to output to
      charArrayIndex=0;
      tempPathArray = new char[strlen(queLocater->_animationName)+2+strlen(queLocater->_animationName)+strlen(endOfFileChars)+1];
      clearCharArray(tempPathArray);
      tempPathArray[charArrayIndex] = '/';
      // [/]
      charArrayIndex++;
      memcpy(tempPathArray+charArrayIndex, queLocater->_animationName, strlen(queLocater->_animationName));
      // [/name]
      charArrayIndex+=strlen(queLocater->_animationName);
      // [/name/]
      tempPathArray[charArrayIndex] = '/';
      charArrayIndex++;
      memcpy(tempPathArray+charArrayIndex, queLocater->_animationName, strlen(queLocater->_animationName));
      charArrayIndex+=strlen(queLocater->_animationName);
      // [/name/name]
      memcpy(tempPathArray+charArrayIndex, endOfFileChars, strlen(endOfFileChars));
      // [/name/name_00000.png]
      charArrayIndex+=strlen(endOfFileChars);
      tempPathArray[charArrayIndex] = '\0';
      
      Serial.printf("\r\nProceessing frames in[");
      Serial.print(tempPathArray);
      Serial.printf("]");
      //check if output stream file exists
      if(!SD.exists(queLocater->_fullPath))
      {
          SD.mkdir(queLocater->_folderPath);
          outputStream = SD.open(queLocater->_fullPath, FILE_WRITE);
          if(!outputStream)
          {
            Serial.printf("\r\n\tERROR - Can not open stream file for writing....");
            outputStream.close();
          }
          else
          {
            //build the stream file
            for(postFrameCounter=0; postFrameCounter<queLocater->_totalFrames; postFrameCounter++)
            {
              fileNameIncremeneter(postFrameCounter, tempPathArray);
              upng = upng_new_from_file(SD, tempPathArray);
              if (upng != NULL)
              {
                upng_decode(upng);
                Serial.printf("\r\nFrame\t%d\t%d\tBytes\t", postFrameCounter, upng_get_size(upng));
                bytesWriten = outputStream.write(upng_get_buffer(upng), upng_get_size(upng));
                upng_free(upng);
                Serial.printf("Written\t%d bytes", bytesWriten);
              }
            }
            outputStream.close();
          }
      }
      else  
      {
        Serial.printf("\r\n\tStream already exists for this item.");
      }
      delete[] tempPathArray; 
    }
  }
}

void fileNameIncremeneter(unsigned short int fileNameIndex, char* filePathString)
{
  char renameAtPointer=0, renameCounter=0;
  char namePlacer = 95; //95="_"

  //find the "_" in the file name fileNamePointer
  for(renameCounter=0; renameCounter<strlen(filePathString); renameCounter++)
  {
    if(filePathString[renameCounter]==namePlacer)
    {
      renameAtPointer = renameCounter+1;
      break;
    }
  }
  itoa(fileNameIndex, integerCharString, 10);  
  if(fileNameIndex<10)
  {
    filePathString[renameAtPointer] = 48;
    filePathString[renameAtPointer+1] = 48;
    filePathString[renameAtPointer+2] = 48;
    filePathString[renameAtPointer+3] = 48;
    filePathString[renameAtPointer+4] = integerCharString[0];
  }
  else if(fileNameIndex<100)
  {
    filePathString[renameAtPointer] = 48;
    filePathString[renameAtPointer+1] = 48;
    filePathString[renameAtPointer+2] = 48;
    filePathString[renameAtPointer+3] = integerCharString[0];
    filePathString[renameAtPointer+4] = integerCharString[1];
  }
  else if(fileNameIndex<1000)
  {
    filePathString[renameAtPointer] = 48;
    filePathString[renameAtPointer+1] = 48;
    filePathString[renameAtPointer+2] = integerCharString[0];
    filePathString[renameAtPointer+3] = integerCharString[1];
    filePathString[renameAtPointer+4] = integerCharString[2];
  }
  else if(fileNameIndex<10000)
  {
    filePathString[renameAtPointer] = 48;
    filePathString[renameAtPointer+1] = integerCharString[0];
    filePathString[renameAtPointer+2] = integerCharString[1];
    filePathString[renameAtPointer+3] = integerCharString[2];
    filePathString[renameAtPointer+4] = integerCharString[3];
  }
  else if(fileNameIndex<100000)
  {
    filePathString[renameAtPointer] = integerCharString[0];
    filePathString[renameAtPointer+1] = integerCharString[1];
    filePathString[renameAtPointer+2] = integerCharString[2];
    filePathString[renameAtPointer+3] = integerCharString[3];
    filePathString[renameAtPointer+4] = integerCharString[4];
  }
}

void loop()
{
	unsigned short int streamCount=0;
	while(true)
	{
	  for(streamCount=0; streamCount<globalAnimationQue.totalNodes; streamCount++)
	  {
		playStream(streamCount);
	  }
	  yield();
	}
}

void playStream(unsigned short int streamID)
{
  unsigned short int frameIndex=0, bufferIndex=0;
  char* frameBuffer = new char[rows*cols*3];

  PLNODE* queLocater = globalAnimationQue.findByID(streamID);
  if(queLocater!=NULL)
  {
    Serial.printf("\r\nPlaying [");
    Serial.print(queLocater->_fullPath);
    Serial.printf("]");
    outputStream = SD.open(queLocater->_fullPath, "r");
    for(frameIndex=0; frameIndex<queLocater->_totalFrames; frameIndex++)
    {
      outputStream.readBytes(frameBuffer, rows*cols*3);
      for(yCount=0; yCount<rows; yCount++)
      {
        for(xCount=0; xCount<cols; xCount++)
        {
          memcpy(tempColour, frameBuffer+bufferIndex, 3);
          drawPixel(xCount, yCount, tempColour);
          bufferIndex+=3;
        }
      }
      bufferIndex=0;
      renderLEDs();
      clearBitmap();
      if(queLocater->_frameDelay==0){}
      else{delay(queLocater->_frameDelay);}
    }
    outputStream.close();
  } 
}


void assignMapToLEDArray()
{
  short int tempAddress=0;
  byte bitMapX=0, bitMapY=0;
 
  //init each pointer spot to SOMETHING
  //start frame
  leds.LEDs[0] = &leds._emptyByte;
  leds.LEDs[1] = &leds._emptyByte;
  leds.LEDs[2] = &leds._emptyByte;
  leds.LEDs[3] = &leds._emptyByte;
  //leds  
  for (xCnt=4; xCnt<leds._frameLength-(leds._endFrameLength*4); xCnt+=4)
  {
      leds.LEDs[xCnt] = &leds._globalBrightness;
      leds.LEDs[xCnt+1] = &leds._emptyByte;
      leds.LEDs[xCnt+2] = &leds._emptyByte;
      leds.LEDs[xCnt+3] = &leds._emptyByte;
  }
  //end frames
  for (xCnt; xCnt<leds._frameLength; xCnt+=4)
  {
      leds.LEDs[xCnt] = &leds._fullByte;
      leds.LEDs[xCnt+1] = &leds._fullByte;
      leds.LEDs[xCnt+2] = &leds._fullByte;
      leds.LEDs[xCnt+3] = &leds._fullByte;
  }
  //attach the mask map to each pointer
  for(bitMapY=0; bitMapY<rows; bitMapY++)
  {
    for(bitMapX=0; bitMapX<cols; bitMapX++)
    {
      tempAddress = getRealAddress(bitMapX, bitMapY);
      if(tempAddress!=deadValue)
      {
        leds.LEDs[ (4*(tempAddress+1))+1 ] = &bitmap[bitMapY][bitMapX][2];
        leds.LEDs[ (4*(tempAddress+1))+2 ] = &bitmap[bitMapY][bitMapX][1];
        leds.LEDs[ (4*(tempAddress+1))+3 ] = &bitmap[bitMapY][bitMapX][0];
      } 
    }
  } 
}

short int getRealAddress(byte xPos, byte yPos)
{
   return  maskMap[yPos][xPos];
}

void renderLEDs()
{
  leds.renderFrame();
  SPI.writeBytes(leds.SPIFrame, leds._frameLength);

}

void clearBitmap()
{
  byte clearYCnt=0, clearXCnt=0;
  
  for(clearYCnt=0; clearYCnt<rows; clearYCnt++)
  {
    for(clearXCnt=0; clearXCnt<cols; clearXCnt++)
    {
        bitmap[clearYCnt][clearXCnt][0] = 0;    
        bitmap[clearYCnt][clearXCnt][1] = 0;    
        bitmap[clearYCnt][clearXCnt][2] = 0;    
    }
  }
}

void drawPixel(byte pixX, byte pixY, byte *pixelColour)
{
  if(pixX>=0 && pixX<cols && pixY>=0 && pixY<rows)
  {
    bitmap[pixY][pixX][0] = pixelColour[0];   
    bitmap[pixY][pixX][1] = pixelColour[1];   
    bitmap[pixY][pixX][2] = pixelColour[2];      
  }
}
