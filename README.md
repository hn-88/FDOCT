# FDOCT
Code for doing realtime FD-OCT. See usage.txt for keystroke list, also enumerated in the code as comments.

Basic build instructions for GCC using cmake:
1. Make sure the required USB and OpenCV libraries and camera SDKs are installed.
2. Modify the CMakeLists.txt file as required - if compiling for webcam, without QHY camera support, please remove the -lqhy dependency in CMakeLists.txt
3. cd to the build directory
4. cmake ..
5. make BscanFFTwebcam.bin

-------------------------
File list:
 	BscanDark.cpp        - a test version with dark-frame subtraction
	
	BscanFFT.cpp 	       - the main software, with QHYCCD camera support
	
	BscanFFTsim.cpp      - simulation using saved files, for testing and validation
	
	BscanFFTwebcam.cpp   - webcam demo version of the main software, which does not need a QHYCCD camera
	
	BscanFFTxml2m.cpp    - helper to convert output xml files to Matlab compatible m files
	
	BscanFFTxml2mm.cpp   - helper to convert output xml files to multiple Matlab compatible m files
	
	BscanFFTxml2ms.cpp   - helper to convert output xml files to multiple Matlab compatible m files, each with a single variable
	
	CMakeLists.txt       - configuration file for the cmake environment
	
  multicamtest.cpp     - an attempt to use multiple QHY cameras at once - does not work.
  
  
--------------------------------------------

More documentation for basic compiling instructions etc is at https://github.com/hn-88/QHYCameratests/blob/master/readme.txt

Documentation for cross platform compiling is at http://hnsws.blogspot.com/2018/03/cross-platform-issues.html

Please also see the following:

http://hnsws.blogspot.com/2018/03/qhy-camera-glitch.html

http://hnsws.blogspot.com/2018/03/qhy-sdk-windows-and-linux.html

http://hnsws.blogspot.com/2018/03/windows-executable-slower.html


