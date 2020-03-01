//#ifdef _WIN64
//#include "stdafx.h"
// VS2015 says stdafx.h not found, so remove it.
// Then we can put the windows.h in the block below. 
//#include "windows.h"
// anything before a precompiled header is ignored, 
// so no endif here! add #endif to compile on __unix__ !
//#endif
#ifdef _WIN64
#include "windows.h"
#include <qhyccd.h>
#endif


/*
* modified from ViewportSaver.cpp

* Saving viewport images
* with inputs from ini file.
* Written for colour QHY cameras.
*
* Saves frames on receipt of s key or spacebar
*
* r key toggles Red channel only mode
* g key toggles Green channel only mode
* b key toggles Blue channel only mode
* 
* + (or =) key increases exposure time by 0.1 ms
* - (or _) key decreases exposure time by 0.1 ms
* u key increases exposure time by 1 ms
* d key decreases exposure time by 1 ms
* U key increases exposure time by 10 ms
* D key decreases exposure time by 10 ms
*
* ESC, x or X key quits
*
* Hari Nandakumar
* 02 Feb 2020  *
*
*
*/

//#define _WIN64
//#define __unix__

#include <stdio.h>
#include <stdlib.h>

#ifdef __unix__
#include <unistd.h>
#include <libqhy/qhyccd.h>
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


using namespace cv;


inline void savematasimage(char* p, char* d, char* f, Mat m)
{
	// saves a Mat m using imwrite as filename f appending .png, both windows and unix versions
	// p=pathname, d=dirname, f=filename

#ifdef __unix__
	strcpy(p, d);
	strcat(p, "/");
	strcat(p, f);
	strcat(p, ".png");
	imwrite(p, m);
#else

	strcpy(p, d);
	strcat(p, "\\");		// imwrite needs path with \\ separators, not /, on windows
	strcat(p, f);
	strcat(p, ".png");
	imwrite(p, m);
#endif	

}



int main(int argc, char *argv[])
{
	int num = 0;
	qhyccd_handle *camhandle = NULL;
	int ret;
	char id[32];
	//char camtype[16];
	int found = 0;
	unsigned int w, h, bpp = 8, channels = 3, cambitdepth = 8;
	unsigned int offsetx = 0, offsety = 0;
	unsigned int indexi, manualindexi, averages = 1, opw, oph;
	uint  indextemp;
	
	int camtime = 1, camgain = 1, camspeed = 1, cambinx = 1, cambiny = 1, usbtraffic = 10;
	int binvalue = 1, normfactor = 1, normfactorforsave = 25;
	double camgamma = 1.0;
	
	int WhiteBalanceR = 20, WhiteBalanceG = 15, WhiteBalanceB = 20;
	
	bool doneflag = 0, skeypressed = 0;
	
	w = 640;
	h = 480;

	int  fps, key;
	int t_start, t_end;

	std::ifstream infile("ViewportSaverc.ini");
	std::string tempstring;
	char dirdescr[60];
	sprintf(dirdescr, "_");

	//namedWindow("linearized",0); // 0 = WINDOW_NORMAL
	//moveWindow("linearized", 20, 500);

	//namedWindow("Bscanl",0); // 0 = WINDOW_NORMAL
	//moveWindow("Bscanl", 400, 0);

	char dirname[80];
	char filename[20];
	char filenamec[20];
	char pathname[140];
	char gammastr[40];
	char lambdamaxstr[40];
	char lambdaminstr[40];
	struct tm *timenow;

	time_t now = time(NULL);

	// inputs from ini file
	if (infile.is_open())
	{

		infile >> tempstring;
		infile >> tempstring;
		infile >> tempstring;
		// first three lines of ini file are comments
		infile >> camgain;
		infile >> tempstring;
		infile >> gammastr;
		infile >> tempstring;
		infile >> camtime;
		infile >> tempstring;
		infile >> bpp;
		infile >> tempstring;
		infile >> w;
		infile >> tempstring;
		infile >> h;
		infile >> tempstring;
		infile >> offsetx;
		infile >> tempstring;
		infile >> offsety;
		infile >> tempstring;
		infile >> camspeed;
		infile >> tempstring;
		infile >> cambinx;
		infile >> tempstring;
		infile >> cambiny;
		infile >> tempstring;
		infile >> usbtraffic;
		infile >> tempstring;
		infile >> binvalue;
		infile >> tempstring;
		infile >> dirdescr;
		infile >> tempstring;
		infile >> WhiteBalanceR;
		infile >> tempstring;
		infile >> WhiteBalanceG;
		infile >> tempstring;
		infile >> WhiteBalanceB;
		infile.close();

		camgamma = atof(gammastr);
	}

	else std::cout << "Unable to open ini file, using defaults." << std::endl;
	
	std::cout << "Camera binning has to be disabled for colour..." << std::endl;
	
	//cambinx = 1; cambiny = 1;

	namedWindow("show", 0); // 0 = WINDOW_NORMAL
	moveWindow("show", 0, 0);

	namedWindow("Status", 0); // 0 = WINDOW_NORMAL
	moveWindow("Status", 0, 500);

	
	/////////////////////////////////////
	// init camera, variables, etc

	cambitdepth = bpp;
	opw = w / binvalue;
	oph = h / binvalue;
	

	unsigned int vertposROI = 10, widthROI = 10;

	
	
	int nr, nc;

	Mat m, opm, opmvector;
	Mat tempmat;
	 
	Mat mraw, BGR[3];
	Mat statusimg = Mat::zeros(cv::Size(600, 300), CV_64F);
	Mat firstrowofstatusimg = statusimg(Rect(0, 0, 600, 50)); // x,y,width,height
	Mat secrowofstatusimg = statusimg(Rect(0, 50, 600, 50));
	Mat secrowofstatusimgRHS = statusimg(Rect(300, 50, 300, 50));
	char textbuffer[80];
	bool Ronly=0, Bonly=0, Gonly=0;

	timenow = localtime(&now);

	strftime(dirname, sizeof(dirname), "%Y-%m-%d_%H_%M_%S-", timenow);

	strcat(dirname, dirdescr);
#ifdef _WIN64
	CreateDirectoryA(dirname, NULL);
#else
	mkdir(dirname, 0755);
#endif

	ret = InitQHYCCDResource();
	if (ret != QHYCCD_SUCCESS)
	{
		printf("Init SDK not successful!\n");
	}

	num = ScanQHYCCD();
	if (num > 0)
	{
		printf("Found QHYCCD,the num is %d \n", num);
	}
	else
	{
		printf("QHYCCD camera not found, please check the usb cable.\n");
		goto failure;
	}

	for (int i = 0; i < num; i++)
	{
		ret = GetQHYCCDId(i, id);
		if (ret == QHYCCD_SUCCESS)
		{
			//printf("connected to the first camera from the list,id is %s\n",id);
			found = 1;
			break;
		}
	}

	if (found != 1)
	{
		printf("The camera is not QHYCCD or other error \n");
		goto failure;
	}

	if (found == 1)
	{
		camhandle = OpenQHYCCD(id);
		if (camhandle != NULL)
		{
			//printf("Open QHYCCD success!\n");
		}
		else
		{
			printf("Open QHYCCD failed \n");
			goto failure;
		}
		ret = SetQHYCCDStreamMode(camhandle, 1);


		ret = InitQHYCCD(camhandle);
		if (ret == QHYCCD_SUCCESS)
		{
			//printf("Init QHYCCD success!\n");
		}
		else
		{
			printf("Init QHYCCD fail code:%d\n", ret);
			goto failure;
		}
		
		ret = IsQHYCCDControlAvailable(camhandle,CAM_COLOR);
        if(ret == BAYER_GB || ret == BAYER_GR || ret == BAYER_BG || ret == BAYER_RG)
        {
            //printf("This is a Color Cam\n");
            SetQHYCCDDebayerOnOff(camhandle,true);
            SetQHYCCDParam(camhandle,CONTROL_WBR,WhiteBalanceR);
            SetQHYCCDParam(camhandle,CONTROL_WBG,WhiteBalanceG);
            SetQHYCCDParam(camhandle,CONTROL_WBB,WhiteBalanceB);
        }
        else  printf("This is not a colour Camera! Use ViewportSaver instead of ViewportSaverc.\n");
        
		ret = IsQHYCCDControlAvailable(camhandle, CONTROL_TRANSFERBIT);
		if (ret == QHYCCD_SUCCESS)
		{
			ret = SetQHYCCDBitsMode(camhandle, cambitdepth);
			if (ret != QHYCCD_SUCCESS)
			{
				printf("SetQHYCCDBitsMode failed\n");

				getchar();
				return 1;
			}



		}


		ret = SetQHYCCDResolution(camhandle, offsetx, offsety, w, h); //handle, xpos,ypos,xwidth,ywidth
		if (ret == QHYCCD_SUCCESS)
		{
			printf("Resolution set - width = %d height = %d\n", w, h);
		}
		else
		{
			printf("SetQHYCCDResolution fail\n");
			goto failure;
		}




		ret = SetQHYCCDParam(camhandle, CONTROL_USBTRAFFIC, usbtraffic); //handle, parameter name, usbtraffic (which can be 0..100 perhaps)
		if (ret == QHYCCD_SUCCESS)
		{
			//printf("CONTROL_USBTRAFFIC success!\n");
		}
		else
		{
			printf("CONTROL_USBTRAFFIC fail\n");
			goto failure;
		}

		ret = SetQHYCCDParam(camhandle, CONTROL_SPEED, camspeed); //handle, parameter name, speed (which can be 0,1,2)
		if (ret == QHYCCD_SUCCESS)
		{
			//printf("CONTROL_CONTROL_SPEED success!\n");
		}
		else
		{
			printf("CONTROL_CONTROL_SPEED fail\n");
			goto failure;
		}

		ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
		if (ret == QHYCCD_SUCCESS)
		{
			//printf("CONTROL_EXPOSURE success!\n");
		}
		else
		{
			printf("CONTROL_EXPOSURE fail\n");
			goto failure;
		}

		ret = SetQHYCCDParam(camhandle, CONTROL_GAIN, camgain); //handle, parameter name, gain (which can be 0..99)
		if (ret == QHYCCD_SUCCESS)
		{
			//printf("CONTROL_GAIN success!\n");
		}
		else
		{
			printf("CONTROL_GAIN fail\n");
			goto failure;
		}

		ret = SetQHYCCDParam(camhandle, CONTROL_GAMMA, camgamma); //handle, parameter name, gamma (which can be 0..2 perhaps)
		if (ret == QHYCCD_SUCCESS)
		{
			//printf("CONTROL_GAMMA success!\n");
		}
		else
		{
			printf("CONTROL_GAMMA fail\n");
			goto failure;
		}


		if (cambitdepth == 8)
		{

			m = Mat::zeros(cv::Size(w, h), CV_8UC3);
			mraw = Mat::zeros(cv::Size(w, h), CV_8UC3);
		}
		else // is 16 bit
		{
			m = Mat::zeros(cv::Size(w, h), CV_16UC3);
			mraw = Mat::zeros(cv::Size(w, h), CV_16UC3);
		}


		ret = BeginQHYCCDLive(camhandle);
		if (ret == QHYCCD_SUCCESS)
		{
			printf("BeginQHYCCDLive success!\n");
			key = waitKey(300);
		}
		else
		{
			printf("BeginQHYCCDLive failed\n");
			goto failure;
		}

		/////////////////////////////////////////
		/////////////////////////////////////////
		//outfile<<"%Data cube in MATLAB compatible format - m(h,w,slice)"<<std::endl;


		doneflag = 0;

		ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
		if (ret == QHYCCD_SUCCESS)
		{
			sprintf(textbuffer, "Exp time = %d ", camtime);
			secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
			putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
			imshow("Status", statusimg);
		}
		else
		{
			sprintf(textbuffer, "CONTROL_EXPOSURE failed");
			secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
			putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
			imshow("Status", statusimg);
			goto failure;
		}
		t_start = time(NULL);
		fps = 0;

		indexi = 0;
		manualindexi = 0;
		indextemp = 0;
		
		
		while (1)		//camera frames acquisition loop
		{
			ret = GetQHYCCDLiveFrame(camhandle, &w, &h, &bpp, &channels, mraw.data);

			if (ret == QHYCCD_SUCCESS)
			{
				if (Ronly == 0 && Gonly == 0 && Bonly == 0 )
				mraw.copyTo(m);
				
				if (Ronly == 1)
				{
					split(mraw, BGR);
					//by default opencv puts channels in BGR order
					BGR[1] = Mat::zeros(mraw.rows, mraw.cols, CV_8UC1); // green channel is set to 0
					BGR[0] = Mat::zeros(mraw.rows, mraw.cols, CV_8UC1); // blue channel is set to 0
					merge(BGR,3,m);
				}
				
				if (Gonly == 1)
				{
					split(mraw, BGR);
					//by default opencv puts channels in BGR order
					BGR[2] = Mat::zeros(mraw.rows, mraw.cols, CV_8UC1); // red channel is set to 0
					BGR[0] = Mat::zeros(mraw.rows, mraw.cols, CV_8UC1); // blue channel is set to 0
					merge(BGR,3,m);
				}
				
				if (Bonly == 1)
				{
					split(mraw, BGR);
					//by default opencv puts channels in BGR order
					BGR[1] = Mat::zeros(mraw.rows, mraw.cols, CV_8UC1); // green channel is set to 0
					BGR[2] = Mat::zeros(mraw.rows, mraw.cols, CV_8UC1); // red channel is set to 0
					merge(BGR,3,m);
				}

				resize(m, opm, Size(), 1.0 / binvalue, 1.0 / binvalue, INTER_AREA);	// binning (averaging)
				imshow("show", opm);

				fps++;
				t_end = time(NULL);
				if (t_end - t_start >= 5)
				{
					//printf("fps = %d\n", fps / 5);
					opm.copyTo(opmvector);
					opmvector.reshape(0, 1);	//make it into a row array
					//minMaxLoc(opmvector, &minVal, &maxVal);
					sprintf(textbuffer, "fps = %d ", fps / 5);
					firstrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
					putText(statusimg, textbuffer, Point(0, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
					sprintf(textbuffer, "%03d images acq.", indextemp);
					secrowofstatusimgRHS = Mat::zeros(cv::Size(300, 50), CV_64F);
					putText(statusimg, textbuffer, Point(300, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
					imshow("Status", statusimg);
					fps = 0;
					t_start = time(NULL);
				}

				
					if (skeypressed == 1)

					{

						indexi++;
						sprintf(filename, "vp%03d", indexi);
						savematasimage(pathname, dirname, filename, m);
						
						sprintf(textbuffer, "vp%03d saved.", indexi);
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
						skeypressed = 0; 
					} // end if skeypressed

					
				key = waitKey(30); // wait 30 milliseconds for keypress
								  // max frame rate at 1280x960 is 30 fps => 33 milliseconds

				switch (key)
				{

				case 27: //ESC key
				case 'x':
				case 'X':
					doneflag = 1;
					break;

				case '+':
				case '=':

					camtime = camtime + 100;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						sprintf(textbuffer, "Exp time = %d ", camtime);
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);

					}
					else
					{
						sprintf(textbuffer, "CONTROL_EXPOSURE failed");
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
						goto failure;
					}
					break;

				case '-':
				case '_':

					camtime = camtime - 100;
					if (camtime < 0)
						camtime = 0;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						sprintf(textbuffer, "Exp time = %d ", camtime);
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
					}
					else
					{
						sprintf(textbuffer, "CONTROL_EXPOSURE failed");
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
						goto failure;
					}
					break;

				case 'U':

					camtime = camtime + 10000;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						sprintf(textbuffer, "Exp time = %d ", camtime);
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
					}
					else
					{
						sprintf(textbuffer, "CONTROL_EXPOSURE failed");
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
						goto failure;
					}
					break;
				case 'D':

					camtime = camtime - 10000;
					if (camtime < 0)
						camtime = 0;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						sprintf(textbuffer, "Exp time = %d ", camtime);
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
					}
					else
					{
						sprintf(textbuffer, "CONTROL_EXPOSURE failed");
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
						goto failure;
					}
					break;
				case 'u':

					camtime = camtime + 1000;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						sprintf(textbuffer, "Exp time = %d ", camtime);
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
					}
					else
					{
						sprintf(textbuffer, "CONTROL_EXPOSURE failed");
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
						goto failure;
					}
					break;
				case 'd':

					camtime = camtime - 1000;
					if (camtime < 0)
						camtime = 0;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						sprintf(textbuffer, "Exp time = %d ", camtime);
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
					}
					else
					{
						sprintf(textbuffer, "CONTROL_EXPOSURE failed");
						secrowofstatusimg = Mat::zeros(cv::Size(600, 50), CV_64F);
						putText(statusimg, textbuffer, Point(0, 80), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3, 1);
						imshow("Status", statusimg);
						goto failure;
					}
					break;

				case 's':
				case 'S':
				case ' ':

					skeypressed = 1;
					break;

				
				default:
					break;

				} // switch case end

				if (doneflag == 1)
				{
					break;
				}
				
				if (channels > 1)
				{
					// if it is a monochrome camera, channels=1 - 
					// only if it is a colour camera do we process
					// the code below.
					switch (key)
					{

					case 'r':
					case 'R':
						if (Ronly == 0)
							Ronly = 1;
						else
							Ronly = 0;
						break;
						
					case 'g':
					case 'G':
						if (Gonly == 0)
							Gonly = 1;
						else
							Gonly = 0;
						break;
						
					case 'b':
					case 'B':
						if (Bonly == 0)
							Bonly = 1;
						else
							Bonly = 0;
						break;
					}	// end switch case for RGB
				}	// end if channels > 1
					

			}  // if ret success end
		} // inner while loop end

	} // end of if found 


	if (camhandle)
	{
		StopQHYCCDLive(camhandle);

		ret = CloseQHYCCD(camhandle);
		if (ret == QHYCCD_SUCCESS)
		{
			printf("Closed QHYCCD.\n");
		}
		else
		{
			goto failure;
		}
	}



	ret = ReleaseQHYCCDResource();
	if (ret == QHYCCD_SUCCESS)
	{
		printf("SDK Resource released successfully.\n");
	}
	else
	{
		goto failure;
	}


	return 0;

failure:
	printf("Fatal error !! \n");
	return 1;
}

