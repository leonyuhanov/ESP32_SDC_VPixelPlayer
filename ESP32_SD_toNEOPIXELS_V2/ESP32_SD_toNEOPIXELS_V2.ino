/*
Micro CD Card PinOut for the Wemos D32 Pro as taken from 
https://www.wemos.cc/en/latest/_static/files/sch_d32_pro_v2.0.0.pdf

NS		1	NC
/CS		2	IO 04	TF_CS
DI		3	IO23	MOSI
VDD		4	3.3V
CLK		5	IO 18	SCK
VSS		6	GND
D0		7	IO 19	MISO
RSV		8	NC
CD(1)	G	GND

PINOUT for Neopixels/WS812/SK6805/SK6812

HSPI MOSI	IO13	->	SN74HCT245N	->	DATA PIN of Neopixel
 */

#include <WiFi.h>
#include "SPI.h"
#include "NeoViaSPI.h"
#include "FS.h"
#include "SD.h"
#include <string>
#include "upng.h"
#include "animationQue.h"

//Bitmaping Data
const short int numLeds = 1539;
const byte bytesPerLed = 3;
byte rows, cols;    //rows = HEIGHT(Y)  cols = WIDTH(X)
byte*** bitmap;

//LED MAP
short int** maskMap;
const short int deadValue=-1;

//Colour Data
byte tempColour[3] = {0,0,0};

//Counters
unsigned short int cIndex=0, secondaryCIndex=0;
unsigned short int yCnt=0, xCnt=0;

//LEDs
NeoViaSPI leds(numLeds);

//File Paths
char playListFile[] = "/playlist";
char pixelMapFile[] = "/pixelmap";
char integerCharString[] = "00000";

//PNG stuff
upng_t* upng;
unsigned short int xCount, yCount, imageSize, bufferIndex=0, testCnt=0;

//SPI
SPIClass * hspi = NULL;
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

  //Load Pixel map from /pixelmap file
  loadPixelMap();
  
  //Populate Animation List from SD Card file /playlist
  compilePlaylist();
  
  //Pre Process Multiple Frames into 1 File Stream
  compileStream();
  
  //Start SPI
  hspi = new SPIClass(HSPI);
  hspi->begin();
  hspi->setBitOrder(MSBFIRST);
  hspi->setFrequency(2900000);
  
   //Clear bitmap memory
  clearBitmap();
  //Render black twice
  renderLEDs();
}

void initFS()
{
  Serial.printf("\r\n\tSetting up SD Card FS...");
  
  SPI.begin();
  //begin(18, 19, 23, 4)
  SPI.setFrequency(40000000);
  if(! SD.begin(SS, SPI, 40000000, "/sd", 5, false) )
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

void loadPixelMap()
{
  char* tempInt;
  char* pixelMapBuffer;
  short int bufferLength, dataValue;
  unsigned short int width = 0, height = 0;
  unsigned short int rCounter=0, cCounter=0, charLength=0, needleIndex=1;
  unsigned short int charTrack[2] = {0,0};

  //read the pixel map file into a local buffer
  outputStream = SD.open(pixelMapFile, "r");
  pixelMapBuffer = new char[outputStream.size()];
  outputStream.readBytes(pixelMapBuffer, outputStream.size());
  bufferLength = outputStream.size();
  outputStream.close();

  height = countNeedles(pixelMapBuffer, '|') + 1;
  width  = ( countNeedles(pixelMapBuffer, ',') + height ) / height;
  Serial.printf("\r\nPixel map is\t%d\tWide\t%d\tTall\r\n", width, height);
  cols = width;
  rows = height;

  //set up pixelmap array
  maskMap = new short int*[rows];
  for(rCounter=0; rCounter<rows; rCounter++)
  {
    maskMap[rCounter] = new short int[cols];
    for(cCounter=0; cCounter<cols; cCounter++)
    {
      maskMap[rCounter][cCounter]=0;
    }
  }
  Serial.printf("\r\nPixelmap array is set up in memory...");
  
  //set up bitmap
  bitmap = new byte**[rows];
  for(rCounter=0; rCounter<rows; rCounter++)
  {
    bitmap[rCounter] = new byte*[cols];
    for(cCounter=0; cCounter<cols; cCounter++)
    {
      bitmap[rCounter][cCounter] = new byte[bytesPerLed];
    }
  }
  Serial.printf("\r\nBitmap buffer is ready...");
  
  //set up the pixelmap array
  Serial.printf("\r\nPopulate Pixel map array...");
  charTrack[0] = 0;
  for(rCounter=0; rCounter<rows; rCounter++)
  {
    for(cCounter=0; cCounter<cols; cCounter++)
    {
      if(cCounter+1<cols)
      {
        charTrack[1] = findNeedleCount(pixelMapBuffer, ',', needleIndex);
      }
      else
      {
        charTrack[1] = findNeedleCount(pixelMapBuffer, '|', rCounter+1);
        needleIndex--;
      }
      charLength = charTrack[1]-charTrack[0];
      //Serial.printf("\r\n[%d]", charLength);
      tempInt = new char[ charLength+1 ];
      clearCharArray( tempInt );
      memcpy(tempInt, pixelMapBuffer+charTrack[0], charLength);
      tempInt[ charLength ]=0;
      maskMap[rCounter][cCounter] = atoi(tempInt);
      needleIndex++;
      charTrack[0]=charTrack[1]+1;
      //Serial.printf("\t[%d][%d]\t[%d]", rCounter, cCounter, maskMap[rCounter][cCounter]);
    }
  }
  Serial.printf("\r\nPixel map is ready!");

  delete[] pixelMapBuffer;
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

unsigned short int countFiles(char* dirName)
{
    unsigned short fileCount=0;
    char* folderPath = new char[strlen(dirName)+2];
    
    folderPath[0] = '/';
    memcpy(folderPath+1, dirName, strlen(dirName));
    folderPath[strlen(dirName)+1] = 0;
    
    
    File root = SD.open(folderPath);
    File file = root.openNextFile();
    while(file)
    {
        fileCount++;
        file = root.openNextFile();
    }
    return fileCount;
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
  unsigned short int hayCount;
  
  for(hayCount=0; hayCount<strlen(haystack); hayCount++)
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


short int getRealAddress(byte xPos, byte yPos)
{
   return  maskMap[yPos][xPos];
}

void renderLEDs()
{
  renderBitmap();
  leds.encode();
  hspi->writeBytes(leds.neoBits, leds._NeoBitsframeLength);

}

void renderBitmap()
{
  short int tempAddress=0;
  byte bitMapX=0, bitMapY=0;
  byte innerColour[3];
  
  for(bitMapY=0; bitMapY<rows; bitMapY++)
  {
    for(bitMapX=0; bitMapX<cols; bitMapX++)
    {
      tempAddress = getRealAddress(bitMapX, bitMapY);
      if(tempAddress!=deadValue)
      {
        memcpy(innerColour, bitmap[bitMapY][bitMapX], bytesPerLed);
        leds.setPixel(tempAddress, innerColour);
      } 
    }
  }
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
