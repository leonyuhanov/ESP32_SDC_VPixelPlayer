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
const byte rows = 17, cols = 106;
byte bitmap[rows][cols][bytesPerLed];

//LED MAP
const short int maskMap[rows][cols]= {{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,304,305,339,338,337,336,335,334,333,332,331,330,329,328,327,326,325,324,323,322,321,320,319,318,317,316,315,314,313,312,311,310,309,308,307,306,916,917,1257,1256,1255,1254,1253,1252,1251,1250,1249,1248,1247,1246,1245,1244,1243,1242,1241,1240,1239,1238,1237,1236,1235,1234,1233,1232,1231,1230,1229,1228,1227,1226,1225,1224,2446,2447,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,303,302,301,300,340,341,342,343,344,345,346,347,348,349,350,351,352,353,354,355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,915,914,913,912,1258,1259,1260,1261,1262,1263,1264,1265,1266,1267,1268,1269,1270,1271,1272,1273,1274,1275,1276,1277,1278,1279,1280,1281,1282,1283,1284,1285,1286,1287,1288,1289,2445,2444,2443,2442,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,294,295,296,297,298,299,401,400,399,398,397,396,395,394,393,392,391,390,389,388,387,386,385,384,383,382,381,380,379,378,377,376,375,374,373,372,906,907,908,909,910,911,1319,1318,1317,1316,1315,1314,1313,1312,1311,1310,1309,1308,1307,1306,1305,1304,1303,1302,1301,1300,1299,1298,1297,1296,1295,1294,1293,1292,1291,1290,2436,2437,2438,2439,2440,2441,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,293,292,291,290,289,288,287,286,402,403,404,405,406,407,408,409,410,411,412,413,414,415,416,417,418,419,420,421,422,423,424,425,426,427,428,429,905,904,903,902,901,900,899,898,1320,1321,1322,1323,1324,1325,1326,1327,1328,1329,1330,1331,1332,1333,1334,1335,1336,1337,1338,1339,1340,1341,1342,1343,1344,1345,1346,1347,2435,2434,2433,2432,2431,2430,2429,2428,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,276,277,278,279,280,281,282,283,284,285,455,454,453,452,451,450,449,448,447,446,445,444,443,442,441,440,439,438,437,436,435,434,433,432,431,430,888,889,890,891,892,893,894,895,896,897,1373,1372,1371,1370,1369,1368,1367,1366,1365,1364,1363,1362,1361,1360,1359,1358,1357,1356,1355,1354,1353,1352,1351,1350,1349,1348,2418,2419,2420,2421,2422,2423,2424,2425,2426,2427,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,275,274,273,272,271,270,269,268,267,266,265,264,456,457,458,459,460,461,462,463,464,465,466,467,468,469,470,471,472,473,474,475,476,477,478,479,887,886,885,884,883,882,881,880,879,878,877,876,1374,1375,1376,1377,1378,1379,1380,1381,1382,1383,1384,1385,1386,1387,1388,1389,1390,1391,1392,1393,1394,1395,1396,1397,2417,2416,2415,2414,2413,2412,2411,2410,2409,2408,2407,2406,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,250,251,252,253,254,255,256,257,258,259,260,261,262,263,501,500,499,498,497,496,495,494,493,492,491,490,489,488,487,486,485,484,483,482,481,480,862,863,864,865,866,867,868,869,870,871,872,873,874,875,1419,1418,1417,1416,1415,1414,1413,1412,1411,1410,1409,1408,1407,1406,1405,1404,1403,1402,1401,1400,1399,1398,2392,2393,2394,2395,2396,2397,2398,2399,2400,2401,2402,2403,2404,2405,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1,-1,249,248,247,246,245,244,243,242,241,240,239,238,237,236,235,234,502,503,504,505,506,507,508,509,510,511,512,513,514,515,516,517,518,519,520,521,861,860,859,858,857,856,855,854,853,852,851,850,849,848,847,846,1420,1421,1422,1423,1424,1425,1426,1427,1428,1429,1430,1431,1432,1433,1434,1435,1436,1437,1438,1439,2391,2390,2389,2388,2387,2386,2385,2384,2383,2382,2381,2380,2379,2378,2377,2376,-1,-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,-1,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,539,538,537,536,535,534,533,532,531,530,529,528,527,526,525,524,523,522,828,829,830,831,832,833,834,835,836,837,838,839,840,841,842,843,844,845,1457,1456,1455,1454,1453,1452,1451,1450,1449,1448,1447,1446,1445,1444,1443,1442,1441,1440,2358,2359,2360,2361,2362,2363,2364,2365,2366,2367,2368,2369,2370,2371,2372,2373,2374,2375,-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,-1,215,214,213,212,211,210,209,208,207,206,205,204,203,202,201,200,199,198,197,196,540,541,542,543,544,545,546,547,548,549,550,551,552,553,554,555,827,826,825,824,823,822,821,820,819,818,817,816,815,814,813,812,811,810,809,808,1458,1459,1460,1461,1462,1463,1464,1465,1466,1467,1468,1469,1470,1471,1472,1473,2357,2356,2355,2354,2353,2352,2351,2350,2349,2348,2347,2346,2345,2344,2343,2342,2341,2340,2339,2338,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,569,568,567,566,565,564,563,562,561,560,559,558,557,556,786,787,788,789,790,791,792,793,794,795,796,797,798,799,800,801,802,803,804,805,806,807,1487,1486,1485,1484,1483,1482,1481,1480,1479,1478,1477,1476,1475,1474,2316,2317,2318,2319,2320,2321,2322,2323,2324,2325,2326,2327,2328,2329,2330,2331,2332,2333,2334,2335,2336,2337,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,173,172,171,170,169,168,167,166,165,164,163,162,161,160,159,158,157,156,155,154,153,152,151,150,570,571,572,573,574,575,576,577,578,579,580,581,785,784,783,782,781,780,779,778,777,776,775,774,773,772,771,770,769,768,767,766,765,764,763,762,1488,1489,1490,1491,1492,1493,1494,1495,1496,1497,1498,1499,2315,2314,2313,2312,2311,2310,2309,2308,2307,2306,2305,2304,2303,2302,2301,2300,2299,2298,2297,2296,2295,2294,2293,2292,-1,-1,-1,-1,-1},{-1,-1,-1,-1,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,591,590,589,588,587,586,585,584,583,582,736,737,738,739,740,741,742,743,744,745,746,747,748,749,750,751,752,753,754,755,756,757,758,759,760,761,1509,1508,1507,1506,1505,1504,1503,1502,1501,1500,2266,2267,2268,2269,2270,2271,2272,2273,2274,2275,2276,2277,2278,2279,2280,2281,2282,2283,2284,2285,2286,2287,2288,2289,2290,2291,-1,-1,-1,-1},{-1,-1,-1,123,122,121,120,119,118,117,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,100,99,98,97,96,592,593,594,595,596,597,598,599,735,734,733,732,731,730,729,728,727,726,725,724,723,722,721,720,719,718,717,716,715,714,713,712,711,710,709,708,1510,1511,1512,1513,1514,1515,1516,1517,2265,2264,2263,2262,2261,2260,2259,2258,2257,2256,2255,2254,2253,2252,2251,2250,2249,2248,2247,2246,2245,2244,2243,2242,2241,2240,2239,2238,-1,-1,-1},{-1,-1,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,605,604,603,602,601,600,678,679,680,681,682,683,684,685,686,687,688,689,690,691,692,693,694,695,696,697,698,699,700,701,702,703,704,705,706,707,1523,1522,1521,1520,1519,1518,2208,2209,2210,2211,2212,2213,2214,2215,2216,2217,2218,2219,2220,2221,2222,2223,2224,2225,2226,2227,2228,2229,2230,2231,2232,2233,2234,2235,2236,2237,-1,-1},{-1,65,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,606,607,608,609,677,676,675,674,673,672,671,670,669,668,667,666,665,664,663,662,661,660,659,658,657,656,655,654,653,652,651,650,649,648,647,646,1524,1525,1526,1527,2207,2206,2205,2204,2203,2202,2201,2200,2199,2198,2197,2196,2195,2194,2193,2192,2191,2190,2189,2188,2187,2186,2185,2184,2183,2182,2181,2180,2179,2178,2177,2176,-1},{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,611,610,612,613,614,615,616,617,618,619,620,621,622,623,624,625,626,627,628,629,630,631,632,633,634,635,636,637,638,639,640,641,642,643,644,645,1529,1528,2142,2143,2144,2145,2146,2147,2148,2149,2150,2151,2152,2153,2154,2155,2156,2157,2158,2159,2160,2161,2162,2163,2164,2165,2166,2167,2168,2169,2170,2171,2172,2173,2174,2175}};
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
