#ifdef _WIN64
#include "stdafx.h"
#include "windows.h"
// anything before a precompiled header is ignored, 
// so no endif here! add #endif to compile on __unix__ !
#endif
#ifdef _WIN64
//#include <qhyccd.h>
#endif


/*
* checking FPS
* for FLIR Point Grey camera
* instead of QHY camera
*
* ESC, x or X key quits
*
*
*
* Hari Nandakumar
* 21 Aug 2019  *
*
*
*/

//#define _WIN64
//#define __unix__

#include <stdio.h>
#include <stdlib.h>

#ifdef __unix__
#include <unistd.h>
//#include <libqhy/qhyccd.h>
#endif

#include <string.h>

#include <time.h>
#include <sys/stat.h>
// this is for mkdir



#include <opencv2/opencv.hpp>
// used the above include when imshow was being shown as not declared
// removing
// #include <opencv/cv.h>
// #include <opencv/highgui.h>

////////////////////////////
//Spinnaker API for FLIR Point Grey camera
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;
///////////////////////////


using namespace cv;


int main(int argc, char *argv[])
{
	
	int num = 0;
	//qhyccd_handle *camhandle = NULL;
	int ret;

	int w = 1280;
	int h = 960;

	int  fps, key, doneflag;
	int t_start, t_end;

	Mat mraw;

	//ret = InitQHYCCDResource();
	SystemPtr system = System::GetInstance();
	CameraList camList = system->GetCameras();
	num = camList.GetSize();
	int result;
	/*
	if (ret != QHYCCD_SUCCESS)
	{
		printf("Init SDK not successful!\n");
	}

	num = ScanQHYCCD();*/
	if (num > 0)
	{
		//printf("Found camera,the num is %d \n", num);
	}
	else
	{
		printf("camera not found, please check the cable and power.\n");
		// Clear camera list before releasing system
        camList.Clear();

        // Release system
        system->ReleaseInstance();
		//goto failure;
	}
	
	CameraPtr pCam = nullptr;
	pCam = camList.GetByIndex(0); // open first camera



		doneflag = 0;

		
		t_start = time(NULL);
		fps = 0;

		
		try
		{
			// Retrieve TL device nodemap and print device information
			INodeMap & nodeMapTLDevice = pCam->GetTLDeviceNodeMap();

			//result = PrintDeviceInfo(nodeMapTLDevice);

			// Initialize camera
			pCam->Init();

			// Retrieve GenICam nodemap
			INodeMap & nodeMap = pCam->GetNodeMap();

			// Acquire images
			//result = result | AcquireImages(pCam, nodeMap, nodeMapTLDevice);
			// Deinitialize camera
			
			//put this here,
			CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode");
			if (!IsAvailable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode))
			{
				cout << "Unable to set continuous acquisition mode as ptr is not writable ..." << endl << endl;
				return -1;
			}

			// Retrieve entry node from enumeration node
			CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
			if (!IsAvailable(ptrAcquisitionModeContinuous) || !IsReadable(ptrAcquisitionModeContinuous))
			{
				cout << "Unable to set continuous acquisition mode as entry is not readable..." << endl << endl;
				return -1;
			}

			// Retrieve integer value from entry node
			const int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();

			// Set integer value from entry node as new value of enumeration node
			ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);
			ImagePtr pResultImage;
			ImagePtr convertedImage;
			// trying begin and end acq, to see if it will improve fps
			// this needs StreamBufferHandlingMode -> NewestOnly
			pCam->BeginAcquisition();
			//////////////////

		while (1)		//camera frames acquisition loop, which is inside the try
		{
			//ret = GetQHYCCDLiveFrame(camhandle, &w, &h, &bpp, &channels, mraw.data);
			
			ret = 0;
			
			
			
			
			while(1)
			{
				pResultImage = pCam->GetNextImage();
			 
				if(pResultImage->IsIncomplete())
				{
					ret=0;
				}
				else
				{
					ret=1;
					// assuming Mono8 1280 x 960 for now
					convertedImage = pResultImage->Convert(PixelFormat_Mono8, HQ_LINEAR);
					mraw = cv::Mat(960, 1280, CV_8UC1, convertedImage->GetData(), convertedImage->GetStride());
					break;
				}
			}	// end of while loop to get a non-InComplete Image
			// pResultImage has to be released to avoid buffer filling up
			pResultImage->Release();
			

			

			if (ret == 1)
			{
				
				imshow("show", mraw);
				
				fps++;
				t_end = time(NULL);
				if (t_end - t_start >= 5)
				{
					printf("fps = %d\n", fps / 5);
					/*
					opm.copyTo(opmvector);
					opmvector.reshape(0, 1);	//make it into a row array
					minMaxLoc(opmvector, &minVal, &maxVal);
					sprintf(textbuffer, "fps = %d  Max intensity = %d", fps / 5, int(floor(maxVal)));
					firstrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
					putText(statusimg, textbuffer, Point(0, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
					sprintf(textbuffer, "%03d images acq.", indextemp);
					secrowofstatusimgRHS = Mat::zeros(cv::Size(300, 50), CV_64F);
					putText(statusimg, textbuffer, Point(300, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
					imshow("Status", statusimg);
					if (ROIreport)
						printMinMaxAscan(bscandb, ascanat, numdisplaypoints, statusimg); */
					fps = 0;
					t_start = time(NULL);
				}

				  ////////////////////////////////////////////


				key = waitKey(3); // wait 30 milliseconds for keypress
								  // max frame rate at 1280x960 is 30 fps => 33 milliseconds

				switch (key)
				{

				case 27: //ESC key
				case 'x':
				case 'X':
					doneflag = 1;
					break;

				default:
					break;

				}

				if (doneflag == 1)
				{
					break;
				}

			}  // if ret success end
		} // inner while loop end
		
		//pResultImage->Release();
		pCam->EndAcquisition();
		pCam->DeInit();
		
		pCam = nullptr;	// without this, spinnaker complains
		// Clear camera list before releasing system
        camList.Clear();

        // Release system
        system->ReleaseInstance();
		

	} // end of try
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}


	return 0;

failure:
	printf("Fatal error !! \n");
	return 1;
}

