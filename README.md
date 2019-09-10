# FDOCT is now re-christened ABC-OCT: Affordable Bscan Camera-based Optical Coherence Tomography
Code for doing realtime FD-OCT. See usage.txt for keystroke list, also enumerated in the code as comments.

Release including a windows binary and a Linux binary as an AppImage -

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.3403735.svg)](https://doi.org/10.5281/zenodo.3403735)

Basic build instructions for GCC using cmake:
1. Make sure the required USB and OpenCV libraries and camera SDKs are installed.
2. Modify the CMakeLists.txt file as required - if compiling for webcam, without QHY camera support, please remove the -lqhy dependency in CMakeLists.txt by renaming CMakeLists.txt.webcam as CMakeLists.txt, or, if using qhy support, rename CMakeLists.txt.qhy as CMakeLists.txt  
3. cd to the build directory
4. cmake ..
5. make BscanFFTwebcam.bin

-------------------------
Dependencies:
The following dependencies for OpenCV need to be installed - on Ubuntu, this can be done by 
sudo apt-get install build-essential cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-2 libusb-1.0-0-dev libusb-1.0-0 libopencv-dev


-------------------------

The .travis.yml file contains build instructions for Travis CI - travis-ci.org - which can be replicated on the commandline for building. 

-------------------------
File list:

 	BscanDark.cpp        - a test version with dark-frame subtraction
	
	BscanFFT.cpp 	     - the main software, with QHYCCD camera support
	
	BscanFFTpeak.cpp     - variant of the main software, with QHYCCD camera support and peak intensity display
	
	BscanFFTsim.cpp      - simulation using saved files, for testing and validation
	
	BscanFFTspin.cpp      - the main software, with spinnaker SDK (Point Grey camera, flir.com) support
	
	BscanFFTwebcam.cpp   - webcam demo version of the main software, which does not need a QHYCCD camera
	
	BscanFFTxml2m.cpp    - helper to convert output xml files to Matlab compatible m files
	
	BscanFFTxml2mm.cpp   - helper to convert output xml files to multiple Matlab compatible m files
	
	BscanFFTxml2ms.cpp   - helper to convert output xml files to multiple Matlab compatible m files, each with a single variable
	
	CMakeLists.txt       - configuration file for the cmake environment 
	
	CMakeLists.txt.qhy   - rename this as CMakeLists.txt if building with qhy support
	
	CMakeLists.txt.webcam   - rename this as CMakeLists.txt if building without qhy, supporting only a webcam
	
	CMakeLists.txt.spinnakermultithreaded   - rename this as CMakeLists.txt if building for spinnaker SDK - flir.com
	
	Displayspin.cpp   - demo to see frame rate with spinnaker SDK
	
	Displayspinmt.cpp   - demo to see frame rate with spinnaker SDK, old multithreaded trial not optimized
	
	ViewportSaver.cpp     - a simple interface to display save viewport images, and save on receipt of s or <spacebar>.
	
	ViewportSaverwebcam.cpp  - as above, written to use a webcam driver instead of QHY driver.
	
	multicamtest.cpp     - an attempt to use multiple QHY cameras at once - does not work.
  
  
--------------------------------------------

More documentation for basic compiling instructions etc is at https://github.com/hn-88/QHYCameratests/blob/master/readme.txt

Documentation for cross platform compiling is at http://hnsws.blogspot.com/2018/03/cross-platform-issues.html

Please also see the following:

http://hnsws.blogspot.com/2018/03/qhy-camera-glitch.html

http://hnsws.blogspot.com/2018/03/qhy-sdk-windows-and-linux.html

http://hnsws.blogspot.com/2018/03/windows-executable-slower.html

Documentation for integrating Spinnaker SDK is at 
http://hnsws.blogspot.com/2019/08/implementing-opencv-project-with.html 
and http://hnsws.blogspot.com/2019/08/visual-studio-specifics-of-opencv.html
