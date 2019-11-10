# WFC to PNG converter

WFC2PNG is a command line tool to convert a WiiFlow cache file(wfc) back to a PNG image.

A batch file called GO.bat is provided for Windows users. Drag 'n drop your file or folder containing your .wfc files.
The png files should be saved in the 'OUT' folder.

For linux, you can use the GO.sh script. ./GO.sh "myfile.wfc" or ./GO.sh "myfolder".
You can modify the width & height in the scripts. The default values are 1024*680.

By default, a wfc file is a lossy compressed texture. In this format, the maximum resolution is 1024*1024.
As a result, the png will be a bit degraded. Resizing at 1090*680 will degrade quality even more.

With a tool like ImageMagick or similar, you can improve the sharpness:
mogrify "Super Aleste (E).png" -unsharp 0x1
or
mogrify "Super Aleste (E).png" -unsharp 1.5x1+0.7+0.02

You may consult the log file 'log_wfc2png.txt' for any errors. Refer to the Return codes in pngu.h to see what the pngu error number means.


## Usage

wfc2png.exe Path Width Height

#Path#

The path to the WiiFlow cache file.

#Width/Height#

The width and height of the expected png image in pixels.


Examples :

  wfc2png.exe e:\WiiFlow\cache\snes9xgx\Mr. Nutz.wfc" 1024 680



