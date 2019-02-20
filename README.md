# FDOCT
Code for doing realtime FD-OCT. See usage.txt for keystroke list, also enumerated in the code as comments.

Basic build instructions for GCC using cmake:
1. Make sure the required USB and OpenCV libraries and camera SDKs are installed.
2. Modify the CMakeLists.txt file as required - if compiling for webcam, without QHY camera support, please remove the -lqhy dependency in CMakeLists.txt
3. cd to the build directory
4. cmake ..
5. make BscanFFTwebcam.bin

More documentation for basic compiling instructions etc is at https://github.com/hn-88/QHYCameratests/blob/master/readme.txt

Documentation for cross platform compiling is at http://hnsws.blogspot.com/2018/03/cross-platform-issues.html

Please also see the following:

http://hnsws.blogspot.com/2018/03/qhy-camera-glitch.html

http://hnsws.blogspot.com/2018/03/qhy-sdk-windows-and-linux.html

http://hnsws.blogspot.com/2018/03/windows-executable-slower.html


