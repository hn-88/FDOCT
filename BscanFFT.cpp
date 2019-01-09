#ifdef _WIN64
#include "stdafx.h"
#include "windows.h"
// anything before a precompiled header is ignored, 
// so no endif here! add #endif to compile on __unix__ !
#endif
#ifdef _WIN64
#include <qhyccd.h>
#endif


/*
* modified from ASKlive2.cpp
* and LiveFrameSampleFFT.cpp
*
*
* Implementing line scan FFT
* for SD OCT
* with binning
* and inputs from ini file.
*
* Captures (background) spectrum on receipt of b key
* Captures pi shifted or J0 frame on receipt of p key
* Saves frames on receipt of s key
* Saves J0 null frame for thresholding on receipt of j key
* Clears the J0 thresholding mask on c key
*
* Do manual frame by frame averaging with ini file option
*
* Save individual frames on averaging if option chosen in ini file
*
* + (or =) key increases exposure time by 0.1 ms
* - (or _) key decreases exposure time by 0.1 ms
* u key increases exposure time by 1 ms
* d key decreases exposure time by 1 ms
* U key increases exposure time by 10 ms
* D key decreases exposure time by 10 ms
* A key toggles averaging
* ] key increases thresholding in final Bscan
* [ key decreases thresholding in final Bscan
* 9 or ( key decreases the index of the reported ascan max value
* 0 or ) key increases the index of the reported ascan max value 
* 
* w decreases width of ROI for which avg val is reported, W increases width
* h decreases the height (position), H increases
* location of ROI is to the right of the index of reported ascan
* 
* ESC, x or X key quits
*
*
*
* Hari Nandakumar
* 15 Sep 2018  *
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

inline void normalizerows(Mat& src, Mat& dst, double lowerlim, double upperlim)
{
	// https://stackoverflow.com/questions/10673715/how-to-normalize-rows-of-an-opencv-mat-without-a-loop
	// for syntax _OutputArray(B.ptr(i), B.cols))
	for(uint ii=0; ii<src.rows; ii++)
	{
		normalize(src.row(ii), dst.row(ii), lowerlim, upperlim, NORM_MINMAX);
	}
	 
}

inline void printAvgROI(Mat bscandb, uint ascanat, uint vertposROI, uint widthROI)
{
	Mat AvgROI;
	Scalar meanVal;
	uint heightROI = 3;
	if(ascanat+widthROI<bscandb.cols)
	{
		bscandb(Rect(ascanat, vertposROI, widthROI, heightROI)).copyTo(AvgROI);
		//imshow("ROI",AvgROI);
		AvgROI.reshape(0, 1);		// make it into a 1D array
		meanVal = mean(AvgROI);
		printf("Mean of ROI at %d = %f dB\n", ascanat, meanVal(0));
	}
	else
		printf("ascanat+widthROI>oph!\n");
}

inline void printMinMaxAscan(Mat bscandb, uint ascanat, int numdisplaypoints)
{
	Mat ascan, ascandisp;
	double minVal, maxVal;
	bscandb.col(ascanat).copyTo(ascan);
	ascan.row(4).copyTo(ascan.row(1));	// masking out the DC in the display
	ascan.row(4).copyTo(ascan.row(0));
	ascan.row(4).copyTo(ascan.row(2));
	ascan.row(4).copyTo(ascan.row(3));
	ascandisp = ascan.rowRange(0, numdisplaypoints);
	//debug
	//normalize(ascan, ascandebug, 0, 1, NORM_MINMAX);
	//imshow("debug", ascandebug);
	minMaxLoc(ascandisp, &minVal, &maxVal);
	printf("Max of Ascan at %d = %f dB\n", ascanat, maxVal);
	printf("Min of Ascan at %d = %f dB\n", ascanat, minVal);
}
					
inline void makeonlypositive(Mat& src, Mat& dst)
{
	// from https://stackoverflow.com/questions/48313249/opencv-convert-all-negative-values-to-zero
    max(src, 0, dst);
     
}

inline Mat zeropadrowwise(Mat sm, int sn)
{
	// increase fft points sn times 
	// newnumcols = numcols*sn;
	// by fft, zero padding, and then inv fft
	
	// returns CV_64F
	
	// guided by https://stackoverflow.com/questions/10269456/inverse-fourier-transformation-in-opencv
	// inspired by Drexler & Fujimoto 2nd ed Section 5.1.10
	
	// needs fftshift implementation for the zero pad to work correctly if done on borders.
	// or else adding zeros directly to the higher frequencies. 
	
	// freqcomplex=fftshift(fft(signal));
	// zp2=4*ifft(ifftshift(zpfreqcomplex));
	
	// result of this way of zero padding in the fourier domain is to resample the same min / max range
	// at a higher sampling rate in the initial domain.
	// So this improves the k linear interpolation.
	
	Mat origimage;
	Mat fouriertransform, fouriertransformzp;
	Mat inversefouriertransform;
	
	int numrows = sm.rows;
	int numcols = sm.cols;
	int newnumcols = numcols*sn;
	
	sm.convertTo(origimage, CV_32F);
	
	dft(origimage, fouriertransform, DFT_SCALE|DFT_COMPLEX_OUTPUT|DFT_ROWS);
	
	// implementing fftshift row-wise
	// like https://docs.opencv.org/2.4/doc/tutorials/core/discrete_fourier_transform/discrete_fourier_transform.html
	int cx = fouriertransform.cols/2;
	
	// here we assume fouriertransform.cols is even
	
	Mat LHS(fouriertransform, Rect(0, 0, cx, fouriertransform.rows));   // Create a ROI per half
	Mat RHS(fouriertransform, Rect(cx, 0, cx, fouriertransform.rows)); //  Rect(topleftx, toplefty, w, h), 
	// OpenCV typically assumes that the top and left boundary of the rectangle are inclusive, while the right and bottom boundaries are not. 
	// https://docs.opencv.org/3.2.0/d2/d44/classcv_1_1Rect__.html
	
	Mat tmp;                           // swap LHS & RHS
    LHS.copyTo(tmp);
    RHS.copyTo(LHS);
    tmp.copyTo(RHS);
	
	copyMakeBorder( fouriertransform, fouriertransformzp, 0, 0, floor((newnumcols-numcols)/2), floor((newnumcols-numcols)/2), BORDER_CONSTANT, 0.0 );
			// this does the zero pad - copyMakeBorder(src, dest, top, bottom, left, right, borderType, value)
	
	// Now we do the ifftshift before ifft
	cx = fouriertransformzp.cols/2;
	Mat LHSzp(fouriertransformzp, Rect(0, 0, cx, fouriertransformzp.rows));   // Create a ROI per half
	Mat RHSzp(fouriertransformzp, Rect(cx, 0, cx, fouriertransformzp.rows)); //  Rect(topleftx, toplefty, w, h)
	
	LHSzp.copyTo(tmp);
    RHSzp.copyTo(LHSzp);
    tmp.copyTo(RHSzp);
	
	dft(fouriertransformzp, inversefouriertransform, DFT_INVERSE|DFT_REAL_OUTPUT|DFT_ROWS);
	inversefouriertransform.convertTo(inversefouriertransform, CV_64F);
	
	return inversefouriertransform;
}

inline Mat smoothmovavg(Mat sm, int sn)
{
	// smooths each row of Mat m using 2n+1 point weighted moving average
	// x(p) = ( x(p-n) + x(p-n+1) + .. + 2*x(p) + x(p+1) + ... + x(p+n) ) / 2*(n+1)
	// The window size is truncated at the edges.
	
	// can see https://docs.opencv.org/2.4/doc/tutorials/core/how_to_scan_images/how_to_scan_images.html#howtoscanimagesopencv
	// for efficient ways 
	
	// accept only double type matrices
	// sm needs to be CV_64FC1
    CV_Assert(sm.depth() == CV_64F);
    
	Mat sresult;
	sm.copyTo(sresult);		// initializing size of result
	
	int smaxcols = sm.cols;
	int smaxrows = sm.rows;
	
	double ssum;
	int sindexi;
	double* srcptr;
	double* destptr;
	
	for(int si = 0; si < smaxrows; si++)
	{
		srcptr = sm.ptr<double>(si);
		destptr = sresult.ptr<double>(si);
		
		for(int sj = 0; sj < smaxcols; sj++)
		{
			ssum=0;
			
			for (int sk = -sn; sk < (sn+1); sk++)
			{
				// address as m.at<double>(y, x); ie (row,column)
				sindexi = sj + sk;
				if ( (sindexi > -1) && (sindexi < smaxcols) )	// truncate window 
					ssum = ssum + srcptr[sindexi];		//equivalent to ssum = ssum + sm.at<double>(si,sindexi);
				else
					ssum = ssum + srcptr[sj];				// when window is truncated,
															// weight of original point increases
				 
			}
			
			// we want to add m.at<double>(i,j) once again, since its weight is 2
			ssum = ssum + srcptr[sj];
			destptr[sj] = ssum / 2 / (sn+1);		//equivalent to sresult.at<double>(si,sj) = ssum / (2 * (sn+1) );
			
		}
			 
	}

	return sresult; 
	 
 

}


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


// the next function saves a Mat m as variablename f by dumping to outfile o, both windows and unix versions

#ifdef __unix__
inline void savematasdata(std::ofstream& o, char* f, Mat m)
{
	// saves a Mat m as variable named f in Matlab m file format
	o << f << "=";
	o << m;
	o << ";" << std::endl;
}

#else
inline void savematasdata(cv::FileStorage& o, char* f, Mat m)
{
	// saves Mat m by serializing to xml as variable named f
	o << f << m;
}
#endif

int main(int argc, char *argv[])
{
	int num = 0;
	qhyccd_handle *camhandle = NULL;
	int ret;
	char id[32];
	//char camtype[16];
	int found = 0;
	unsigned int w, h, bpp = 8, channels, cambitdepth = 16, numofframes = 100;
	unsigned int numofm1slices = 10, numofm2slices = 10, firstaccum, secondaccum;
	unsigned int offsetx = 0, offsety = 0;
	unsigned int indexi, manualindexi, averages = 1, opw, oph;
	uint  indextemp, indextempl;
	uint ascanat=20;
	uint averagestoggle = 1;


	int camtime = 1, camgain = 1, camspeed = 1, cambinx = 2, cambiny = 2, usbtraffic = 10;
	int camgamma = 1, binvalue = 1, normfactor = 1, normfactorforsave = 25;
	int numfftpoints = 1024;
	int numdisplaypoints = 512;
	bool saveframes = 0;
	bool manualaveraging = 0, saveinterferograms = 0;
	unsigned int manualaverages = 1;
	int movavgn = 0;

	bool doneflag = 0, skeypressed = 0, bkeypressed = 0, pkeypressed = 0;
	bool jthresholding = 0, jkeypressed = 0, ckeypressed = 0;
	Mat jmask, jmaskt;
	double lambdamin, lambdamax;
	lambdamin = 816e-9;
	lambdamax = 884e-9;
	int mediann = 5;
	uint increasefftpointsmultiplier = 1;
	double bscanthreshold = -30.0;
	bool rowwisenormalize = 0;
	bool donotnormalize = 1;

	w = 640;
	h = 480;

	int  fps, key, bscanat;
	int t_start, t_end;

	std::ifstream infile("BscanFFT.ini");
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
		infile >> camtime;
		infile >> tempstring;
		infile >> bpp;
		infile >> tempstring;
		infile >> w;
		infile >> tempstring;
		infile >> h;
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
		infile >> averages;
		infile >> tempstring;
		infile >> numfftpoints;
		infile >> tempstring;
		infile >> saveframes;
		infile >> tempstring;
		infile >> manualaveraging;
		infile >> tempstring;
		infile >> manualaverages;
		infile >> tempstring;
		infile >> saveinterferograms;
		infile >> tempstring;
		infile >> movavgn;
		infile >> tempstring;
		infile >> numdisplaypoints;
		infile >> tempstring;
		infile >> lambdaminstr;
		infile >> tempstring;
		infile >> lambdamaxstr;
		infile >> tempstring;
		infile >> mediann;
		infile >> tempstring;
		infile >> increasefftpointsmultiplier;
		infile >> tempstring;
		infile >> rowwisenormalize;
		infile >> tempstring;
		infile >> donotnormalize;
		infile.close();
		
		lambdamin = atof(lambdaminstr);
		lambdamax = atof(lambdamaxstr);
		averagestoggle = averages;
	}

	else std::cout << "Unable to open ini file, using defaults.";

	namedWindow("show", 0); // 0 = WINDOW_NORMAL
	moveWindow("show", 20, 0);

	namedWindow("Bscan", 0); // 0 = WINDOW_NORMAL
	moveWindow("Bscan", 800, 0);
	
	// debug
	/*
	char debugwinname[80];
	namedWindow("debug1", 0); // 0 = WINDOW_NORMAL
	moveWindow("debug1", 100, 600);
	namedWindow("debug2", 0); // 0 = WINDOW_NORMAL
	moveWindow("debug2", 200, 600);

	namedWindow("debug3", 0); // 0 = WINDOW_NORMAL
	moveWindow("debug3", 300, 600);

	namedWindow("debug4", 0); // 0 = WINDOW_NORMAL
	moveWindow("debug4", 400, 600);

	namedWindow("debug5", 0); // 0 = WINDOW_NORMAL
	moveWindow("debug5", 500, 600);

	namedWindow("debug6", 0); // 0 = WINDOW_NORMAL
	moveWindow("debug6", 600, 600);
	
	namedWindow("debug7", 0); // 0 = WINDOW_NORMAL
	moveWindow("debug7", 700, 600);

	namedWindow("debug8", 0); // 0 = WINDOW_NORMAL
	moveWindow("debug8", 800, 600);

	namedWindow("debug9", 0); // 0 = WINDOW_NORMAL
	moveWindow("debug9", 900, 600);

	namedWindow("debug0", 0); // 0 = WINDOW_NORMAL
	moveWindow("debug0", 0, 600);
	* */
	
	if (manualaveraging)
	{
		namedWindow("Bscanm", 0); // 0 = WINDOW_NORMAL
		moveWindow("Bscanm", 800, 400);
	}


	/////////////////////////////////////
	// init camera, variables, etc

	cambitdepth = bpp;
	opw = w / binvalue;
	oph = h / binvalue;
	float lambda0 = (lambdamin + lambdamax) / 2;
	float lambdabw = lambdamax - lambdamin;
	
	unsigned int vertposROI=10, widthROI=10;

	Mat data_y(oph, opw, CV_64F);		// the Mat constructor Mat(rows,columns,type)
	Mat data_ylin(oph, numfftpoints, CV_64F);
	Mat data_yb(oph, opw, CV_64F);
	Mat data_yp(oph, opw, CV_64F);
	Mat padded, paddedn;
	Mat barthannwin(1, opw, CV_64F);		// the Mat constructor Mat(rows,columns,type);
	Mat baccum, manualaccum;
	uint baccumcount, manualaccumcount;

	// initialize data_yb with zeros
	data_yb = Mat::zeros(Size(opw, oph), CV_64F);		//Size(cols,rows)		
	data_yp = Mat::zeros(Size(opw, oph), CV_64F);
	baccum = Mat::zeros(Size(opw, oph), CV_64F);
	baccumcount = 0;

	manualaccumcount = 0;
	
	Mat bscansave0[100];		// allocate buffer to save frames, max 100
	Mat bscansave1[100];		// one buffer is active while other is saved on skeypressed
	
	Mat jscansave;		// to save j frames

	Mat bscanmanualsave0[100];
	Mat bscanmanualsave1[100];

	Mat interferogramsave0[100];
	Mat interferogramsave1[100];
	Mat interferogrambsave0[100];
	Mat interferogrambsave1[100];
	
	bool zeroisactive = 1;

	int nr, nc;

	Mat m, opm, opmvector, bscan, bscanlog, bscandb, bscandisp, bscandispmanual, bscantemp, bscantemp2, bscantemp3, bscantransposed, chan[3];
	Mat tempmat;
	Mat bscandispj;
	Mat mraw;

	//Mat bscanl, bscantempl, bscantransposedl;
	Mat magI, cmagI, cmagImanual;
	//Mat magIl, cmagIl;
	double minbscan, maxbscan;
	//double minbscanl, maxbscanl;
	Scalar meanval;
	Mat lambdas, k, klinear;
	Mat diffk, slopes, fractionalk, nearestkindex;

	double kmin, kmax;
	double pi = 3.141592653589793;

	double minVal, maxVal, pixVal;
	Mat ascan;
	//minMaxLoc( m, &minVal, &maxVal, &minLoc, &maxLoc );

	double deltalambda = (lambdamax - lambdamin) / data_y.cols;
	
	 
	klinear = Mat::zeros(cv::Size(1, numfftpoints), CV_64F);
	fractionalk = Mat::zeros(cv::Size(1, numfftpoints), CV_64F);
	nearestkindex = Mat::zeros(cv::Size(1, numfftpoints), CV_32S);
		
	if (increasefftpointsmultiplier > 1)
	{
		lambdas = Mat::zeros(cv::Size(1, increasefftpointsmultiplier*data_y.cols), CV_64F);		//Size(cols,rows)
		diffk = Mat::zeros(cv::Size(1, increasefftpointsmultiplier*data_y.cols), CV_64F);
		slopes = Mat::zeros(cv::Size(data_y.rows, increasefftpointsmultiplier*data_y.cols), CV_64F); 
	}
	else
	{
		lambdas = Mat::zeros(cv::Size(1, data_y.cols), CV_64F);		//Size(cols,rows)
		diffk = Mat::zeros(cv::Size(1, data_y.cols), CV_64F);
		slopes = Mat::zeros(cv::Size(data_y.rows, data_y.cols), CV_64F);
		
	}

	resizeWindow("Bscan", oph, numdisplaypoints);		// (width,height)

	for (indextemp = 0; indextemp<(increasefftpointsmultiplier*data_y.cols); indextemp++)
	{
		// lambdas = linspace(830e-9, 870e-9 - deltalambda, data_y.cols)
		lambdas.at<double>(0, indextemp) = lambdamin + indextemp * deltalambda / increasefftpointsmultiplier;

	}
	k = 2 * pi / lambdas;
	kmin = 2 * pi / (lambdamax - deltalambda);
	kmax = 2 * pi / lambdamin;
	double deltak = (kmax - kmin) / numfftpoints;

	for (indextemp = 0; indextemp<(numfftpoints); indextemp++)
	{
		// klinear = linspace(kmin, kmax, numfftpoints)
		klinear.at<double>(0, indextemp) = kmin + (indextemp + 1)*deltak;
	}



	//for (indextemp=0; indextemp<(data_y.cols); indextemp++) 
	//{
	//printf("k=%f, klin=%f\n", k.at<double>(0,indextemp), klinear.at<double>(0,indextemp));
	//}


	for (indextemp = 1; indextemp<(increasefftpointsmultiplier*data_y.cols); indextemp++)
	{
		//find the diff of the non-linear ks
		// since this is a decreasing series, RHS is (i-1) - (i)
		diffk.at<double>(0, indextemp) = k.at<double>(0, indextemp - 1) - k.at<double>(0, indextemp);
		//printf("i=%d, diffk=%f \n", indextemp, diffk.at<double>(0,indextemp));
	}
	// and initializing the first point separately
	diffk.at<double>(0, 0) = diffk.at<double>(0, 1);

	for (int f = 0; f < numfftpoints; f++)
	{
		// find the index of the nearest k value, less than the linear k
		for (indextemp = 0; indextemp < increasefftpointsmultiplier*data_y.cols; indextemp++)
		{
			//printf("Before if k=%f,klin=%f \n",k.at<double>(0,indextemp),klinear.at<double>(0,f));
			if (k.at<double>(0, indextemp) < klinear.at<double>(0, f))
			{
				nearestkindex.at<int>(0, f) = indextemp;
				//printf("After if k=%f,klin=%f,nearestkindex=%d\n",k.at<double>(0,indextemp),klinear.at<double>(0,f),nearestkindex.at<int>(0,f));
				break;

			}	// end if


		}		//end indextemp loop

	}		// end f loop

	for (int f = 0; f < numfftpoints; f++)
	{
		// now find the fractional amount by which the linearized k value is greater than the next lowest k
		fractionalk.at<double>(0, f) = (klinear.at<double>(0, f) - k.at<double>(0, nearestkindex.at<int>(0, f))) / diffk.at<double>(0, nearestkindex.at<int>(0, f));
		//printf("f=%d, klinear=%f, diffk=%f, k=%f, nearesti=%d\n",f, klinear.at<double>(0,f), diffk.at<double>(0,nearestkindex.at<int>(0,f)), k.at<double>(0,nearestkindex.at<int>(0,f)),nearestkindex.at<int>(0,f) );
		//printf("f=%d, fractionalk=%f\n",f, fractionalk.at<double>(0,f));
	}



	timenow = localtime(&now);

	strftime(dirname, sizeof(dirname), "%Y-%m-%d_%H_%M_%S-", timenow);

	strcat(dirname, dirdescr);
#ifdef _WIN64
	CreateDirectoryA(dirname, NULL);
	cv::FileStorage outfile;
	sprintf(filename, "BscanFFT.xml");
	strcpy(pathname, dirname);
	strcat(pathname, "\\");
	strcat(pathname, filename);
	outfile.open(pathname, cv::FileStorage::WRITE);
#else
	mkdir(dirname, 0755);
#endif

#ifdef __unix__	
	sprintf(filename, "BscanFFT.m");
	strcpy(pathname, dirname);
	strcat(pathname, "/");
	strcat(pathname, filename);
	std::ofstream outfile(pathname);
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


		ret = SetQHYCCDResolution(camhandle, 0, 0, w, h); //handle, xpos,ypos,xwidth,ywidth
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

			m = Mat::zeros(cv::Size(w, h), CV_8U);
			mraw = Mat::zeros(cv::Size(w, h), CV_8U);
		}
		else // is 16 bit
		{
			m = Mat::zeros(cv::Size(w, h), CV_16U);
			mraw = Mat::zeros(cv::Size(w, h), CV_16U);
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
			printf("Exp time = %d \n", camtime);
		}
		else
		{
			printf("CONTROL_EXPOSURE fail\n");
			goto failure;
		}
		t_start = time(NULL);
		fps = 0;

		indexi = 0;
		manualindexi = 0;
		indextemp = 0;
		bscantransposed = Mat::zeros(Size(numdisplaypoints, oph), CV_64F);
		manualaccum = Mat::zeros(Size(oph, numdisplaypoints), CV_64F); // this is transposed version
																	   //bscantransposedl = Mat::zeros(Size(opw/2, oph), CV_64F);

		for (uint p = 0; p<(opw); p++)
		{
			// create modified Bartlett-Hann window
			// https://in.mathworks.com/help/signal/ref/barthannwin.html
			float nn = p;
			float NN = opw - 1;
			barthannwin.at<double>(0, p) = 0.62 - 0.48*std::abs(nn / NN - 0.5) + 0.38*std::cos(2 * pi*(nn / NN - 0.5));

		}

		while (1)		//camera frames acquisition loop
		{
			ret = GetQHYCCDLiveFrame(camhandle, &w, &h, &bpp, &channels, mraw.data);

			if (ret == QHYCCD_SUCCESS)
			{
				//median filter while the numbers are still int
				if (mediann>0)
					medianBlur(mraw, m, mediann);
				else
					mraw.copyTo(m);
				
				resize(m, opm, Size(), 1.0 / binvalue, 1.0 / binvalue, INTER_AREA);	// binning (averaging)
				imshow("show", opm);
				
				if (saveinterferograms)
						{
							// save mraw to active buffer
							// inactive buffer is saved to disk when skeypressed
							if (zeroisactive)
							{
								mraw.copyTo(interferogramsave0[indextemp]);
								opm.copyTo(interferogrambsave0[indextemp]);
								//printf("Saved to interferogramsave0[%d]\n",indextemp);
								//sprintf(debugwinname,"debug%d",baccumcount);
								//imshow(debugwinname,interferogramsave0[indextemp]);
								//waitKey(30);
							}
							else
							{
								mraw.copyTo(interferogramsave1[indextemp]);
								opm.copyTo(interferogrambsave1[indextemp]);
								//printf("Saved to interferogramsave1[%d]\n",indextemp);
								//sprintf(debugwinname,"debug%d",baccumcount);
								//imshow(debugwinname,interferogramsave1[indextemp]);
								//waitKey(30);
							}
							
						}
				
				
				opm.convertTo(data_y, CV_64F);	// initialize data_y
				
				// smoothing by weighted moving average
				if (movavgn > 0)
					data_y = smoothmovavg(data_y, movavgn);
				
				 
				
				//transpose(opm, data_y); 		// void transpose(InputArray src, OutputArray dst)
				// because we actually want the columns and not rows
				// using DFT_ROWS
				// But that has rolling shutter issues, so going back to rows

				if (bkeypressed == 1)

				{
					if (saveinterferograms)
						{
							// in this case, formerly active buffer is saved to disk when bkeypressed
							// since no further 
							// and all accumulation is done
							Mat activeMat, activeMatb, activeMat64;
							for (uint ii = 0; ii<averagestoggle; ii++)
							{
								if (zeroisactive)
								{
									activeMat  = interferogramsave1[ii];
									activeMatb = interferogrambsave1[ii];
								}
								else
								{
									activeMat  = interferogramsave0[ii];
									activeMatb = interferogrambsave0[ii];
								}
									
								sprintf(filename, "rawframeb%03d-%03d", indexi,ii);
								savematasimage(pathname, dirname, filename, activeMat);
								activeMatb.convertTo(activeMat64, CV_64F);
								accumulate(activeMat64,baccum);
							}
							baccum.copyTo(data_yb);		// saves the "background" or source spectrum
							if (rowwisenormalize)
								normalizerows(data_yb,data_yb,0.0001, 1);
							if (!donotnormalize)
								normalize(data_yb, data_yb, 0.0001, 1, NORM_MINMAX);
							else
								data_yb = data_yb / averagestoggle;
								
							bkeypressed = 0;
							
						}	
						
						else 
						{
							if (baccumcount < averagestoggle)
							{
								accumulate(data_y, baccum);
								// save the raw frame to buffer
								
								baccumcount++;
							}
							else
							{
								baccum.copyTo(data_yb);		// saves the "background" or source spectrum
								
								if (rowwisenormalize)
									normalizerows(data_yb, data_yb, 0.0001, 1);
								if (!donotnormalize)
									normalize(data_yb, data_yb, 0.0001, 1, NORM_MINMAX);
								else
									data_yb = data_yb / averagestoggle;
								bkeypressed = 0;
								baccumcount = 0;
								
							}
						} // end if not saveinterferograms
						
						if (manualaveraging)
								{
									averagestoggle = 1;
								}

				}

				if (pkeypressed == 1)

				{

					data_y.copyTo(data_yp);		// saves the pi shifted or J0 spectrum	
					if (saveinterferograms)
						{
							// only a single frame to be saved when pkeypressed
							Mat temp;
							data_y.convertTo(temp, CV_8UC1); 
							sprintf(filename, "rawframepbin%03d", indexi);
							savematasimage(pathname, dirname, filename, temp);
							sprintf(filename, "rawframep%03d", indexi);
							savematasimage(pathname, dirname, filename, mraw);
						}
					data_yp.convertTo(data_yp, CV_64F);
					if (rowwisenormalize)
						normalizerows(data_yp, data_yp, 0, 1);
					if (!donotnormalize)
						normalize(data_yp, data_yp, 0, 1, NORM_MINMAX);
					pkeypressed = 0;

				}
				fps++;
				t_end = time(NULL);
				if (t_end - t_start >= 5)
				{
					printf("fps = %d\n", fps / 5);
					opm.copyTo(opmvector);
					opmvector.reshape(0, 1);	//make it into a row array
					minMaxLoc(opmvector, &minVal, &maxVal);
					printf("Max intensity = %d\n", int(floor(maxVal)));
					printMinMaxAscan(bscandb, ascanat, numdisplaypoints);
					fps = 0;
					t_start = time(NULL);
				}

				////////////////////////////////////////////

				// apodize 
				// data_y = ( (data_y - data_yb) ./ data_yb ).*gausswin
				data_y.convertTo(data_y, CV_64F);
				if (rowwisenormalize)
					normalizerows(data_y, data_y, 0, 1);
				if (!donotnormalize)
					normalize(data_y, data_y, 0, 1, NORM_MINMAX);
				//data_yb.convertTo(data_yb, CV_64F);
				//
				data_y = (data_y - data_yp) / data_yb;


				for (int p = 0; p<(data_y.rows); p++)
				{
					//DC removal
					Scalar meanval = mean(data_y.row(p));
					data_y.row(p) = data_y.row(p) - meanval(0);		// Only the first value of the scalar is useful for us

					//windowing
					multiply(data_y.row(p), barthannwin, data_y.row(p));
				}
				
				//increasing number of points by zero padding
				if (increasefftpointsmultiplier > 1)
					data_y = zeropadrowwise(data_y, increasefftpointsmultiplier);


				// interpolate to linear k space
				for (int p = 0; p<(data_y.rows); p++)
				{
					for (int q = 1; q<(data_y.cols); q++)
					{
						//find the slope of the data_y at each of the non-linear ks
						slopes.at<double>(p, q) = data_y.at<double>(p, q) - data_y.at<double>(p, q - 1);
						// in the .at notation, it is <double>(y,x)
						//printf("slopes(%d,%d)=%f \n",p,q,slopes.at<double>(p,q));
					}
					// initialize the first slope separately
					slopes.at<double>(p, 0) = slopes.at<double>(p, 1);


					for (int q = 1; q<(data_ylin.cols - 1); q++)
					{
						//find the value of the data_ylin at each of the klinear points
						// data_ylin = data_y(nearestkindex) + fractionalk(nearestkindex)*slopes(nearestkindex)
						//std::cout << "q=" << q << " nearestk=" << nearestkindex.at<int>(0,q) << std::endl;
						data_ylin.at<double>(p, q) = data_y.at<double>(p, nearestkindex.at<int>(0, q))
							+ fractionalk.at<double>(nearestkindex.at<int>(0, q))
							* slopes.at<double>(p, nearestkindex.at<int>(0, q));
						//printf("data_ylin(%d,%d)=%f \n",p,q,data_ylin.at<double>(p,q));
					}
					//data_ylin.at<double>(p, 0) = 0;
					//data_ylin.at<double>(p, numfftpoints) = 0;

				}

				// InvFFT

				Mat planes[] = { Mat_<float>(data_ylin), Mat::zeros(data_ylin.size(), CV_32F) };
				Mat complexI;
				merge(planes, 2, complexI);         // Add to the expanded another plane with zeros

				dft(complexI, complexI, DFT_ROWS | DFT_INVERSE);            // this way the result may fit in the source matrix

																			// compute the magnitude and switch to logarithmic scale
																			// => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
				split(complexI, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
				magnitude(planes[0], planes[1], magI);


				if (indextemp < averagestoggle) 
				{
					bscantemp = magI.colRange(0, numdisplaypoints);
					bscantemp.convertTo(bscantemp, CV_64F);
					accumulate(bscantemp, bscantransposed);
					if (saveframes == 1)
					{
						// save the individual frames before averaging also
						if (zeroisactive)
						bscantemp.copyTo(bscansave0[indextemp]);
						else
						bscantemp.copyTo(bscansave1[indextemp]);
					}

					indextemp++;

				}
				
				if (indextemp >= averagestoggle) 
				{
					indextemp = 0;
					// we will also toggle the buffers, at the end of this 'else' code block
					
					transpose(bscantransposed, bscan);
					bscan = bscan / averagestoggle;
					bscan += Scalar::all(0.00001);   	// to prevent log of 0  
					// 20.0 * log(0.1) / 2.303 = -20 dB, which is sufficient 
					
					if (jthresholding)
					{
						// create the mask 
						Mat jthreshdiff = bscan - jscansave;
						Mat positivediff;
						jthreshdiff.copyTo(positivediff);		// just to initialize the Mat
						makeonlypositive(jthreshdiff, positivediff);
						positivediff.convertTo(positivediff, CV_8UC1, 1.0);
						threshold(positivediff, jmask, 5, 255, THRESH_BINARY);
						jmask.convertTo(jmask, CV_8UC1, 255.0);
						jmaskt = jmask.rowRange(0, numdisplaypoints);
					}

					
					log(bscan, bscanlog);					// switch to logarithmic scale
															//convert to dB = 20 log10(value), from the natural log above
					bscandb = 20.0 * bscanlog / 2.303;
					
					bscandb.row(4).copyTo(bscandb.row(1));	// masking out the DC in the display
                    bscandb.row(4).copyTo(bscandb.row(0));

					//bscandisp=bscandb.rowRange(0, numdisplaypoints);
					tempmat = bscandb.rowRange(0, numdisplaypoints);
					tempmat.copyTo(bscandisp);
					// apply bscanthresholding
					// MatExpr max(const Mat& a, double s)
					bscandisp = max(bscandisp, bscanthreshold);
					normalize(bscandisp, bscandisp, 0, 1, NORM_MINMAX);	// normalize the log plot for display
					bscandisp.convertTo(bscandisp, CV_8UC1, 255.0);
					if (jthresholding)
					{
						// bitwise AND the image with the mask
						// debug here
						bitwise_and(bscandisp, jmaskt, bscandispj);
						applyColorMap(bscandispj, cmagI, COLORMAP_JET);
					}
					else
					applyColorMap(bscandisp, cmagI, COLORMAP_JET);
					putText(cmagI,"^",Point(ascanat-10, numdisplaypoints), FONT_HERSHEY_COMPLEX, 1,Scalar(255,255,255),3,8);
					//putText(img,"Text",location, fontface, fonstscale,colorbgr,thickness,linetype, bool bottomLeftOrigin=false);
					
					imshow("Bscan", cmagI);
					printAvgROI(bscandb, ascanat, vertposROI, widthROI);
					
					if (jkeypressed == 1)
					{
						bscan.copyTo(jscansave);
						jthresholding = 1;			// setting the boolean flag
						jkeypressed = 0;
					}
					
					if (ckeypressed == 1)
					{
						// clear the thresholding boolean
						jthresholding = 0;
						ckeypressed = 0;
					}


					if (skeypressed == 1)

					{

						indexi++;
						sprintf(filename, "bscan%03d", indexi);
						savematasdata(outfile, filename, bscandb);
						savematasimage(pathname, dirname, filename, bscandisp);
						sprintf(filenamec, "bscanc%03d", indexi);
						savematasimage(pathname, dirname, filenamec, cmagI);
						
						if (jthresholding)
						{
							// save the respective j image also
							sprintf(filename, "jscan%03d", indexi);
							savematasdata(outfile, filename, jscansave);
							savematasimage(pathname, dirname, filename, jscansave);
							
						}
						if (saveinterferograms)
						{
							sprintf(filename, "linearized%03d", indexi);
							savematasdata(outfile, filename, data_ylin);
							normalize(data_ylin, bscantemp2, 0, 255, NORM_MINMAX);	// normalize the log plot for save
							bscantemp2.convertTo(bscantemp2, CV_8UC1, 1.0);		// imwrite needs 0-255 CV_8U
							savematasimage(pathname, dirname, filename, bscantemp2);
							// in this case, formerly active buffer is saved to disk when bkeypressed
							// since no further 
							// and all accumulation is done
							Mat activeMat;
							for (uint ii = 0; ii<averagestoggle; ii++)
							{
								if (zeroisactive)
								{
									activeMat  = interferogramsave1[ii];
								}
								else
								{
									activeMat  = interferogramsave0[ii];
								}
									
								sprintf(filename, "rawframe%03d-%03d", indexi,ii);
								savematasimage(pathname, dirname, filename, activeMat);
								
							}

						}

						if (saveframes == 1)
						{
							for (uint ii = 0; ii<averagestoggle; ii++)
							{
								// save the bscansave array after processing
								if (zeroisactive)
									transpose(bscansave1[ii], bscantemp2);	// don't touch the active buffer
								else
									transpose(bscansave0[ii], bscantemp2);
								bscantemp2 += Scalar::all(0.000001);   	// to prevent log of 0                 
								log(bscantemp2, bscantemp2);					// switch to logarithmic scale
																				//convert to dB = 20 log10(value), from the natural log above
								bscantemp2 = 20.0 * bscantemp2 / 2.303;
								normalize(bscantemp2, bscantemp2, 0, 1, NORM_MINMAX);	// normalize the log plot for save
								bscantemp2.convertTo(bscantemp2, CV_8UC1, 255.0);		// imwrite needs 0-255 CV_8U
								sprintf(filename, "bscan%03d-%03d", indexi, ii);
								savematasimage(pathname, dirname, filename, bscantemp2);
								
								if (saveinterferograms)
								{
									// inactive buffer is saved to disk when skeypressed
									// and all accumulation is done
									for (uint ii = 0; ii<averagestoggle; ii++)
									{
										sprintf(filename, "rawframe%03d-%03d", indexi,ii);
										if (zeroisactive)
											savematasimage(pathname, dirname, filename, interferogramsave1[ii]);
										else
											savematasimage(pathname, dirname, filename, interferogramsave0[ii]);
									}
									
								}	


							}
						}

						skeypressed = 0; // if bscanl is necessary, comment this line, do for bscanl also, then make it 0 

						if (manualaveraging)
						{
							if (manualaccumcount < manualaverages)
							{
								accumulate(bscan, manualaccum);

								if (saveframes == 1)
								{
									// save the individual frames before averaging also
									if (zeroisactive)
										bscan.copyTo(bscanmanualsave0[manualaccumcount]);
									else
										bscan.copyTo(bscanmanualsave1[manualaccumcount]);
								}

								manualaccumcount++;
							}
							else
							{
								manualaccumcount = 0;
								manualaccum = manualaccum / manualaverages;
								//printf("depth of manualaccum is %d \n", manualaccum.depth());
								log(manualaccum, manualaccum);					// switch to logarithmic scale
																				//convert to dB = 20 log10(value), from the natural log above
								bscandispmanual = 20.0 * manualaccum / 2.303;
								
								// apply bscanthresholding
								bscandispmanual = max(bscandispmanual, bscanthreshold);

								normalize(bscandispmanual, bscandispmanual, 0, 1, NORM_MINMAX);	// normalize the log plot for display
								bscandispmanual.convertTo(bscandispmanual, CV_8UC1, 255.0);
								applyColorMap(bscandispmanual, cmagImanual, COLORMAP_JET);
								
								imshow("Bscanm", cmagImanual);
								

								// and save - similar code as in skeypressed
								//////////////////////////////////////////
								manualindexi++;
								sprintf(filename, "bscanman%03d", manualindexi);
								sprintf(filenamec, "bscanmanc%03d", manualindexi);
								savematasdata(outfile, filename, manualaccum);
								savematasimage(pathname, dirname, filename, bscandispmanual);
								savematasimage(pathname, dirname, filenamec, cmagImanual);
								
								manualaccum = Mat::zeros(Size(oph, numdisplaypoints), CV_64F);


								if (saveframes == 1)
								{
									for (uint ii = 0; ii<manualaverages; ii++)
									{
										// save the bscanmanualsave array after processing
										//transpose(bscanmanualsave[ii], bscantemp2); - is already transposed
										if (zeroisactive)
											bscanmanualsave1[ii].copyTo(bscantemp3);		// save only the inactive buffer
										else
											bscanmanualsave0[ii].copyTo(bscantemp3);
											
										bscantemp3.convertTo(bscantemp3, CV_64F);
										bscantemp3 += Scalar::all(0.000001);   	// to prevent log of 0       
										          
										log(bscantemp3, bscantemp3);					// switch to logarithmic scale
																						//convert to dB = 20 log10(value), from the natural log above
										bscantemp3 = 20.0 * bscantemp3 / 2.303;
										normalize(bscantemp3, bscantemp3, 0, 1, NORM_MINMAX);	// normalize the log plot for save
										bscantemp3.convertTo(bscantemp3, CV_8UC1, 255.0);		// imwrite needs 0-255 CV_8U
										sprintf(filename, "bscanm%03d-%03d", manualindexi, ii);
										savematasimage(pathname, dirname, filename, bscantemp3);

									}



								} // end if saveframes

							}  //////////////end code to save manual////////////

						} // end if manual averaging


					} // end if skeypressed

					bscantransposed = Mat::zeros(Size(numdisplaypoints, oph), CV_64F);
					
					// toggle the buffers
					if (zeroisactive)
						zeroisactive=0;
					else
						zeroisactive=1;
						
				} // end else (if indextemp < averages)

				//////////////////////////////////////////////////////
				// a bscan without linearization, sanity check.
				//////////////////////////////////
				//nr = getOptimalDFTSize( data_y.rows );	//128 when taking transpose(opm, data_y);
				//nc = getOptimalDFTSize( data_y.cols );	//96
				////nc = nc * 4;		// 4x oversampling


				//copyMakeBorder(data_y, padded, 0, nr - data_y.rows, 0, nc - data_y.cols, BORDER_CONSTANT, Scalar::all(0));

				//Mat planesl[] = {Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F)};
				//Mat complexIl;
				//merge(planesl, 2, complexIl);         // Add to the expanded another plane with zeros

				//dft(complexIl, complexIl, DFT_ROWS|DFT_INVERSE);            // this way the result may fit in the source matrix

				//// compute the magnitude and switch to logarithmic scale
				//// => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
				//split(complexIl, planesl);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
				//magnitude(planesl[0], planesl[1], magIl);



				//if(indextempl < averages)
				//{
				//bscantempl = magIl.colRange(0,nc/2);
				//bscantempl.convertTo(bscantempl,CV_64F);
				//accumulate(bscantempl, bscantransposedl);
				//indextempl++;
				//}
				//else
				//{
				//indextempl = 0;
				//transpose(bscantransposedl, bscanl); 
				//// remove dc
				//bscanl.row(0).setTo(Scalar(0));

				//normalize(bscanl, bscanl, 0, 1, NORM_MINMAX);
				//bscanl += Scalar::all(1);                    // switch to logarithmic scale
				//log(bscanl, bscanl);
				//normalize(bscanl, bscanl, 0, 1, NORM_MINMAX);	// normalize the log plot for display

				//bscanl.convertTo(bscanl, CV_8UC1, 255.0);
				//applyColorMap(bscanl, cmagIl, COLORMAP_JET);

				//imshow( "Bscanl", cmagIl );

				//if (skeypressed==1)	

				//{

				////indexi++;
				//// this was already done in the earlier code
				//sprintf(filename, "bscanlam%03d.png",indexi);
				//sprintf(filenamec, "bscanlamc%03d.png",indexi);
				////normalize(bscan, bscan, 0, 255, NORM_MINMAX);

				//#ifdef __unix__
				//strcpy(pathname,dirname);
				//strcat(pathname,"/");
				//strcat(pathname,filename);
				//imwrite(pathname, bscanl);

				//strcpy(pathname,dirname);
				//strcat(pathname,"/");
				//strcat(pathname,filenamec);
				//imwrite(pathname, cmagIl);

				//sprintf(filename, "bscanlam%03d",indexi);
				//outfile<< filename << "=";
				//outfile<<bscanl;
				//outfile<<";"<<std::endl;

				//#else
				//imwrite(filename, bscanl);
				//imwrite(filenamec, cmagIl);
				//outfile << "bscanl" << bscanl;
				//#endif		 	
				//skeypressed=0; 	 

				//}


				//bscantransposedl = Mat::zeros(Size(opw/2, oph), CV_64F);
				//}




				////////////////////////////////////////////


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
						printf("Exp time = %d \n", camtime);
					}
					else
					{
						printf("CONTROL_EXPOSURE fail\n");
						goto failure;
					}
					break;

				case '-':
				case '_':

					camtime = camtime - 100;
					if (camtime < 0)
						camtime=0;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						printf("Exp time = %d \n", camtime);
					}
					else
					{
						printf("CONTROL_EXPOSURE fail\n");
						goto failure;
					}
					break;

				case 'U':

					camtime = camtime + 10000;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						printf("Exp time = %d \n", camtime);
					}
					else
					{
						printf("CONTROL_EXPOSURE fail\n");
						goto failure;
					}
					break;
				case 'D':

					camtime = camtime - 10000;
					if (camtime < 0)
						camtime=0;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						printf("Exp time = %d \n", camtime);
					}
					else
					{
						printf("CONTROL_EXPOSURE fail\n");
						goto failure;
					}
					break;
				case 'u':

					camtime = camtime + 1000;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						printf("Exp time = %d \n", camtime);
					}
					else
					{
						printf("CONTROL_EXPOSURE fail\n");
						goto failure;
					}
					break;
				case 'd':

					camtime = camtime - 1000;
					if (camtime < 0)
						camtime=0;
					ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
					if (ret == QHYCCD_SUCCESS)
					{
						printf("Exp time = %d \n", camtime);
					}
					else
					{
						printf("CONTROL_EXPOSURE fail\n");
						goto failure;
					}
					break;

				case 's':
				case 'S':

					skeypressed = 1;
					break;

				case 'b':
				case 'B':

					bkeypressed = 1;
					break;

				case 'p':
				case 'P':

					pkeypressed = 1;
					break;
					
				case 'j':
				case 'J':

					jkeypressed = 1;
					break;
					
				case 'c':
				case 'C':

					ckeypressed = 1;
					break;
					
				case ']':

					bscanthreshold += 1.0;
					printf("bscanthreshold = %f \n",bscanthreshold);
					break;
					
				case '[':

					bscanthreshold -= 1.0;
					printf("bscanthreshold = %f \n",bscanthreshold);
					break;
					
				case '(':
					if (ascanat > 10)
						ascanat -= 10;
					
					printf("ascanat = %d \n",ascanat);
					printMinMaxAscan(bscandb, ascanat, numdisplaypoints);
					break;
					
				case '9':
					if (ascanat > 0)
						ascanat -= 1;
					
					printf("ascanat = %d \n",ascanat);
					printMinMaxAscan(bscandb, ascanat, numdisplaypoints);
					break;
					
				case ')':
					if (ascanat < (oph-11))
						ascanat += 10;
					
					printf("ascanat = %d \n",ascanat);
					printMinMaxAscan(bscandb, ascanat, numdisplaypoints);
					break;
				case '0':
					if (ascanat < (oph-1))
						ascanat += 1;
					
					printf("ascanat = %d \n",ascanat);
					printMinMaxAscan(bscandb, ascanat, numdisplaypoints);
					break;
					
				case 'W':
					if ((ascanat + widthROI) < (oph-1))
						widthROI += 1;
					
					printf("ROI width = %d \n",widthROI);
					printAvgROI(bscandb, ascanat, vertposROI, widthROI);
					break;
					
				case 'w':
					if (widthROI > 2)
						widthROI -= 1;
					
					printf("ROI width = %d \n",widthROI);
					printAvgROI(bscandb, ascanat, vertposROI, widthROI);
					break;
					
				case 'h':
					if (vertposROI < (numdisplaypoints-1))
						vertposROI += 1;
					
					printf("ROI vertical position = %d \n",vertposROI);
					printAvgROI(bscandb, ascanat, vertposROI, widthROI);
					break;
					
				case 'H':
					if (vertposROI > 2)
						vertposROI -= 1;
					
					printf("ROI vertical position = %d \n",vertposROI);
					printAvgROI(bscandb, ascanat, vertposROI, widthROI);
					break;

				case 'a':
				case 'A':
					if (averagestoggle==1)
						averagestoggle = averages;
					else
						averagestoggle=1;
					printf("Now averaging %d bscans.\n",averagestoggle);	
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

	} // end of if found 

#ifdef __unix__
	outfile << "% Parameters were - camgain, camtime, bpp, w , h , camspeed, usbtraffic, binvalue, bscanthreshold" << std::endl;
	outfile << "% " << camgain;
	outfile << ", " << camtime;
	outfile << ", " << bpp;
	outfile << ", " << w;
	outfile << ", " << h;
	outfile << ", " << camspeed;
	outfile << ", " << usbtraffic;
	outfile << ", " << binvalue;
	outfile << ", " << int(bscanthreshold);





#else
	  //imwrite("bscan.png", normfactorforsave*bscan);


	outfile << "camgain" << camgain;
	outfile << "camtime" << camtime;
	outfile << "bscanthreshold" << int(threshold);


#endif






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

