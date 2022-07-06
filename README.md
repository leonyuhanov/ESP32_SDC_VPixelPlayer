# ESP32_SDC_VPixelPlayer
ESP32 CD Card Animation Player for APA102/SK9822 Pixels 

# Libraries Used

APA102 SPI Driver - https://github.com/leonyuhanov/esp8266apa102driver modified with advaced pixel mapping functions

ESP32 PNG Library - https://github.com/lagunax/ESP32-upng - modified to work with SD card instead of SPIFFS


# SDCARD Pre requisites

Format the SDCARD as one FAT32 Volume. Size isnt important. The root of the SDCard needs to have the following structure:

````
[DIR] AnimationOne
[DIR] AnimationTwo
[DIR] AnimationThree
[FILE] playlist
````

Example layout of one of the animation folders:

````
/AnimationOne
AnimationOne_00000.png
AnimationOne_00001.png
AnimationOne_00002.png
AnimationOne_00003.png
AnimationOne_00004.png
AnimationOne_..............
AnimationOne_00100.png
````

The Playlist file is set up as follows:
````
[Case Senstive Animation Name],[Number of Frame Files],[Delay in MILISECONDS between each frame];
[Case Senstive Animation Name],[Number of Frame Files],[Delay in MILISECONDS between each frame];
[Case Senstive Animation Name],[Number of Frame Files],[Delay in MILISECONDS between each frame];
....
````

Example layout of the "playlist" file

````
AnimationOne,100,10;AnimationTwo,200,20;AnimationThree,50,100;
````

# Boot Up Flow

1.  Insert the SD CARD and power on
2.  The Pre prosessor will RUN over the playlist and compile a .STREAM file for each animation. You will see a new folder for each animation called "AnimationNameSTREAM" and inside it will be teh stream file named "AnimationName.STREAM"
3.  After all animation folders are processed the ESP32 will start playing the streams in the order listed in the file 'playlist'. You can put multiple entries for the same animation. Each animation has 2 options. Number of frames in the animation folder and the delay in MILISECONDS between each frame. 10ms delay is good for very smooth frame rates.

