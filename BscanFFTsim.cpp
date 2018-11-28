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
 * modified from BscanFFT.cpp
 * for testing with saved images.
 *  imgi.png and backg.png
 * 
 * Implementing line scan FFT
 * for SD OCT
 * with binning
 * and inputs from ini file.
 * 
 * Saves frames on receipt of s key 
 * 
 * 
 * + key increases exposure time by 0.1 ms
 * - key decreases exposure time by 0.1 ms
 * u key increases exposure time by 1 ms
 * d key decreases exposure time by 1 ms
 * U key increases exposure time by 10 ms
 * D key decreases exposure time by 10 ms
 * ESC key quits
 * 
 * 
 * 
 * Hari Nandakumar
 *   
 * 26 Sep 2018
 * 			
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
	// w=winpath, p=pathname, d=dirname, f=filename

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
    qhyccd_handle *camhandle=NULL;
    int ret;
    char id[32];
    //char camtype[16];
    int found = 0;
	unsigned int w, h, bpp = 8, channels, cambitdepth = 16, numofframes = 100;
	unsigned int numofm1slices = 10, numofm2slices = 10, firstaccum, secondaccum;
	unsigned int offsetx = 0, offsety = 0;
	unsigned int indexi, manualindexi, averages = 1, opw, oph;
	int  indextemp, indextempl;


	int camtime = 1, camgain = 1, camspeed = 1, cambinx = 2, cambiny = 2, usbtraffic = 10;
	int camgamma = 1, binvalue = 1, normfactor = 1, normfactorforsave = 25;
	int numfftpoints = 1024;
	int numdisplaypoints = 512;
	bool saveframes = 0;
	bool manualaveraging = 0, saveinterferograms = 0;
	unsigned int manualaverages = 1;
	int movavgn = 0;
     
	bool doneflag = 0, skeypressed = 0, bkeypressed = 0, pkeypressed = 0;
    
    w=640;
    h=480;
    
    int  fps, key, bscanat;
    int t_start,t_end;
    
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
	char pathname[40];
	struct tm *timenow;
	
	time_t now = time(NULL);
	
    // inputs from ini file
    if (infile.is_open())
		  {
			
			infile >> tempstring;
			infile >> tempstring;
			infile >> tempstring;
			// first three lines of ini file are comments
			infile >> camgain  ;
			infile >> tempstring;
			infile >> camtime  ;
			infile >> tempstring;
			infile >> bpp  ;
			infile >> tempstring;
			infile >> w  ;
			infile >> tempstring;
			infile >> h  ;
			infile >> tempstring;
			infile >> camspeed  ;
			infile >> tempstring;
			infile >> cambinx  ;
			infile >> tempstring;
			infile >> cambiny  ;
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
			infile.close();
		  }

		  else std::cout << "Unable to open ini file, using defaults.";
		  
	namedWindow("show", 0); // 0 = WINDOW_NORMAL
	moveWindow("show", 20, 0);

	namedWindow("Bscan", 0); // 0 = WINDOW_NORMAL
	moveWindow("Bscan", 800, 0);

	if (manualaveraging)
	{
		namedWindow("Bscanm", 0); // 0 = WINDOW_NORMAL
		moveWindow("Bscanm", 800, 400);
	}
	   
	   
    /////////////////////////////////////
    // init camera, variables, etc
    
    cambitdepth = bpp;
    opw = w/binvalue;
	oph = h/binvalue;
	
	Mat ROI;
	Mat plot_result;
	Mat plot_result2;
	 
	Mat data_y( oph, opw, CV_64F );		// the Mat constructor Mat(rows,columns,type)
	Mat data_ylin( oph, numfftpoints, CV_64F );
	Mat data_yb( oph, opw, CV_64F );
	Mat data_yp( oph, opw, CV_64F );
	Mat padded, paddedn;
	Mat barthannwin( 1, opw, CV_64F );		// the Mat constructor Mat(rows,columns,type);
	Mat baccum, manualaccum;
	int baccumcount, manualaccumcount;
	
	// initialize data_yb with zeros
	data_yb = Mat::zeros(Size(opw, oph), CV_64F);		//Size(cols,rows)	
	data_yp = Mat::zeros(Size(opw, oph), CV_64F);
	baccum = Mat::zeros(Size(opw, oph), CV_64F);
	baccumcount = 0;

	manualaccumcount = 0;

	Mat bscansave[100];		// allocate buffer to save frames, max 100
	Mat bscanmanualsave[100];
	Mat interferogramsave[100];
	 
	int nr, nc;
	
	Mat m, opm, opmvector, bscan, bscanlog, bscandb, bscandisp, bscandispmanual, bscantemp, bscantemp2, bscantemp3, bscantransposed, chan[3];
	//Mat bscanl, bscantempl, bscantransposedl;
	Mat magI, cmagI, cmagImanual;
	//Mat magIl, cmagIl;
	double minbscan, maxbscan;
	//double minbscanl, maxbscanl;
	Scalar meanval;
	Mat lambdas, k, klinear;
	Mat diffk, slopes, fractionalk, nearestkindex;
	
	double lambdamin, lambdamax, kmin, kmax;
	double pi = 3.141592653589793;
	
	double minVal, maxVal, pixVal;
	//minMaxLoc( m, &minVal, &maxVal, &minLoc, &maxLoc );
	
	// assuming current data_y's each row goes from
	// lambda_min to lambda_max
	// 830 nm to 870 nm,
	lambdamin = 816e-9;
	lambdamax = 884e-9;
	
	double deltalambda = (lambdamax - lambdamin ) / data_y.cols;
	
	
	lambdas = Mat::zeros(cv::Size(1, data_y.cols), CV_64F);		//Size(cols,rows)
	klinear = Mat::zeros(cv::Size(1, numfftpoints), CV_64F);
	diffk = Mat::zeros(cv::Size(1, data_y.cols), CV_64F);
	fractionalk = Mat::zeros(cv::Size(1, numfftpoints), CV_64F);
	slopes = Mat::zeros(cv::Size(data_y.rows, data_y.cols), CV_64F);
	nearestkindex = Mat::zeros(cv::Size( 1, numfftpoints ), CV_32S);
	
	resizeWindow("Bscan", oph, numdisplaypoints);		// (width,height)
	
	for (indextemp=0; indextemp<(data_y.cols); indextemp++) 
	{
		// lambdas = linspace(830e-9, 870e-9, data_y.cols)
		lambdas.at<double>(0,indextemp) = lambdamin + indextemp*deltalambda;
	
	}
	k = 2*pi / lambdas;
	kmin = 2*pi/(lambdamax-deltalambda);
	kmax = 2*pi/lambdamin;
	double deltak = (kmax - kmin) / numfftpoints;
	
	for (indextemp=0; indextemp<(numfftpoints); indextemp++) 
	{
		// klinear = linspace(kmin, kmax, numfftpoints)
		klinear.at<double>(0,indextemp) = kmin + (indextemp+1)*deltak;
	}
	
	
	
	//for (indextemp=0; indextemp<(data_y.cols); indextemp++) 
	//{
		//printf("k=%f, klin=%f\n", k.at<double>(0,indextemp), klinear.at<double>(0,indextemp));
	//}
	
	
	for (indextemp=1; indextemp<(data_y.cols); indextemp++) 
	{
		//find the diff of the non-linear ks
		// since this is a decreasing series, RHS is (i-1) - (i)
		diffk.at<double>(0,indextemp) = k.at<double>(0,indextemp-1) - k.at<double>(0,indextemp);
		//printf("i=%d, diffk=%f \n", indextemp, diffk.at<double>(0,indextemp));
	}
	// and initializing the first point separately
	diffk.at<double>(0,0) = diffk.at<double>(0,1);
	
	for (int f=0; f < numfftpoints; f++)
	{
		// find the index of the nearest k value, less than the linear k
		for (indextemp = 0; indextemp < data_y.cols; indextemp++)
		{
			//printf("Before if k=%f,klin=%f \n",k.at<double>(0,indextemp),klinear.at<double>(0,f));
			if (k.at<double>(0,indextemp) < klinear.at<double>(0,f) ) 
			{
				nearestkindex.at<int>(0,f) = indextemp;
				//printf("After if k=%f,klin=%f,nearestkindex=%d\n",k.at<double>(0,indextemp),klinear.at<double>(0,f),nearestkindex.at<int>(0,f));
				break;
				
			}	// end if
			
			
		}		//end indextemp loop
		
	}		// end f loop
	
	for (int f=0; f < numfftpoints; f++)
	{
				// now find the fractional amount by which the linearized k value is greater than the next lowest k
		fractionalk.at<double>(0,f) = (klinear.at<double>(0,f) - k.at<double>(0,nearestkindex.at<int>(0,f)) ) / diffk.at<double>(0,nearestkindex.at<int>(0,f));
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
    
    
/*
    ret = InitQHYCCDResource();
    if(ret != QHYCCD_SUCCESS)
    {
        printf("Init SDK not successful!\n");
    }
    
    num = ScanQHYCCD();
    if(num > 0)
    {
        printf("Found QHYCCD,the num is %d \n",num);
    }
    else
    {
        printf("QHYCCD camera not found, please check the usb cable.\n");
        goto failure;
    }

    for(int i = 0;i < num;i++)
    {
        ret = GetQHYCCDId(i,id);
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("connected to the first camera from the list,id is %s\n",id);
            found = 1;
            break;
        }
    }
    
    if(found != 1)
    {
        printf("The camera is not QHYCCD or other error \n");
        goto failure;
    }

    if(found == 1)
    {
        camhandle = OpenQHYCCD(id);
        if(camhandle != NULL)
        {
            //printf("Open QHYCCD success!\n");
        }
        else
        {
            printf("Open QHYCCD failed \n");
            goto failure;
        }
        ret = SetQHYCCDStreamMode(camhandle,1);
    

        ret = InitQHYCCD(camhandle);
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("Init QHYCCD success!\n");
        }
        else
        {
            printf("Init QHYCCD fail code:%d\n",ret);
            goto failure;
        }
        
        
        
       ret = IsQHYCCDControlAvailable(camhandle,CONTROL_TRANSFERBIT);
        if(ret == QHYCCD_SUCCESS)
        {
            ret = SetQHYCCDBitsMode(camhandle,cambitdepth);
            if(ret != QHYCCD_SUCCESS)
            {
                printf("SetQHYCCDBitsMode failed\n");
                
                getchar();
                return 1;
            }

           
                     
        }  
              

        ret = SetQHYCCDResolution(camhandle,0,0, w, h); //handle, xpos,ypos,xwidth,ywidth
        if(ret == QHYCCD_SUCCESS)
        {
            printf("Resolution set - width = %d height = %d\n", w,h); 
        }
        else
        {
            printf("SetQHYCCDResolution fail\n");
            goto failure;
        }
        
        
        
        
        ret = SetQHYCCDParam(camhandle, CONTROL_USBTRAFFIC, usbtraffic); //handle, parameter name, usbtraffic (which can be 0..100 perhaps)
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("CONTROL_USBTRAFFIC success!\n");
        }
        else
        {
            printf("CONTROL_USBTRAFFIC fail\n");
            goto failure;
        }
        
        ret = SetQHYCCDParam(camhandle, CONTROL_SPEED, camspeed); //handle, parameter name, speed (which can be 0,1,2)
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("CONTROL_CONTROL_SPEED success!\n");
        }
        else
        {
            printf("CONTROL_CONTROL_SPEED fail\n");
            goto failure;
        }
        
        ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("CONTROL_EXPOSURE success!\n");
        }
        else
        {
            printf("CONTROL_EXPOSURE fail\n");
            goto failure;
        }
        
        ret = SetQHYCCDParam(camhandle, CONTROL_GAIN, camgain); //handle, parameter name, gain (which can be 0..99)
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("CONTROL_GAIN success!\n");
        }
        else
        {
            printf("CONTROL_GAIN fail\n");
            goto failure;
        }
        
        ret = SetQHYCCDParam(camhandle, CONTROL_GAMMA, camgamma); //handle, parameter name, gamma (which can be 0..2 perhaps)
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("CONTROL_GAMMA success!\n");
        }
        else
        {
            printf("CONTROL_GAMMA fail\n");
            goto failure;
        }
        
   */     
        if (cambitdepth==8)
        {
			
			m  = Mat::zeros(cv::Size(w, h), CV_8U);    
		}
        else // is 16 bit
        {
			m  = Mat::zeros(cv::Size(w, h), CV_16U);
		}
		 
	/*	
        ret = BeginQHYCCDLive(camhandle);
        if(ret == QHYCCD_SUCCESS)
        {
            printf("BeginQHYCCDLive success!\n");
            key=waitKey(300);
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
        if(ret == QHYCCD_SUCCESS)
        {
            printf("Exp time = %d \n", camtime);
        }
        else
        {
            printf("CONTROL_EXPOSURE fail\n");
            goto failure;
        }
        */
        t_start = time(NULL);
        fps = 0;
        
        indexi = 0;
		manualindexi = 0;
        indextemp = 0;
        bscantransposed = Mat::zeros(Size(numfftpoints/2, oph), CV_64F);
		manualaccum = Mat::zeros(Size(oph, numdisplaypoints), CV_64F); // this is transposed version
	    //bscantransposedl = Mat::zeros(Size(opw/2, oph), CV_64F);
	    
	    for (int p=0; p<(opw); p++)
		{
			// create modified Bartlett-Hann window
			// https://in.mathworks.com/help/signal/ref/barthannwin.html
			float nn = p;
			float NN = opw-1;
			barthannwin.at<double>(0,p) = 0.62 - 0.48*std::abs(nn/NN - 0.5) + 0.38*std::cos(2*pi*(nn/NN - 0.5));
			
		}
		
        while(1)		//camera frames acquisition loop
        { 
            ret = QHYCCD_SUCCESS;//GetQHYCCDLiveFrame(camhandle,&w,&h,&bpp,&channels,m.data);
            m = imread("imgi.png");
            split(m,chan);	// saved image is 3 channel, we want only one channel
             
            if (ret == QHYCCD_SUCCESS)  
            {
            resize(chan[0], opm, Size(), 1.0/binvalue, 1.0/binvalue, INTER_AREA);	// binning (averaging)
             //take only one channel
            imshow("show",opm);
            opm.copyTo(data_y);
				opm.convertTo(data_y, CV_64F);	// initialize data_y
				
				// smoothing by weighted moving average
				data_y = smoothmovavg(data_y, movavgn);
            //transpose(opm, data_y); 		// void transpose(InputArray src, OutputArray dst)
											// because we actually want the columns and not rows
											// using DFT_ROWS
											// But that has rolling shutter issues, so going back to rows
					
			if (bkeypressed==1)	
                 
					{
					m = imread("backg.png");
					split(m,chan);
					resize(chan[0], opm, Size(), 1.0/binvalue, 1.0/binvalue, INTER_AREA);	// binning (averaging)
					opm.copyTo(data_yb);	
					 //data_y.copyTo(data_yb);		// saves the "background" or source spectrum	
					 bkeypressed=0; 
						
					}	
					
			if (pkeypressed==1)	
                 
					{
					m = imread("piimgi.png");
					split(m,chan);
					resize(chan[0], opm, Size(), 1.0/binvalue, 1.0/binvalue, INTER_AREA);	// binning (averaging)
					opm.copyTo(data_yp);	
					 //data_y.copyTo(data_yb);		// saves the "background" or source spectrum	
					 pkeypressed=0; 
						
					}
			
			fps++;
            t_end = time(NULL);
                if(t_end - t_start >= 5)
                {
                    printf("fps = %d\n",fps / 5); 
                    opm.copyTo(opmvector);
                    opmvector.reshape(0,1);	//make it into a row array
                    minMaxLoc(opmvector, &minVal, &maxVal);
                    printf("Max intensity = %d\n", int(floor(maxVal)));
                    fps = 0;
                    t_start = time(NULL);
                }
            
            ////////////////////////////////////////////
             
                // apodize 
                // data_y = ( (data_y - data_yp) ./ data_yb ).*window
                data_y.convertTo(data_y, CV_64F);
				normalize(data_y, data_y, 0, 1, NORM_MINMAX);
				//data_yb.convertTo(data_yb, CV_64F);
                data_y =  (data_y - data_yp) / data_yb  ;
                
                for (int p=0; p<(data_y.rows); p++)
                {
					//DC removal
					Scalar meanval = mean(data_y.row(p));
					data_y.row(p) = data_y.row(p) - meanval(0);		// Only the first value of the scalar is useful for us
					
					//windowing
					multiply(data_y.row(p), barthannwin, data_y.row(p)); 
				}			
				
                
                // interpolate to linear k space
                for (int p=0; p<(data_y.rows); p++)
                {
					for (int q=1; q<(data_y.cols); q++)
					{
						//find the slope of the data_y at each of the non-linear ks
						slopes.at<double>(p,q) = data_y.at<double>(p,q) - data_y.at<double>(p,q-1);	
						// in the .at notation, it is <double>(y,x)
						//printf("slopes(%d,%d)=%f \n",p,q,slopes.at<double>(p,q));
					}
					// initialize the first slope separately
					slopes.at<double>(p,0) = slopes.at<double>(p,1);
					
					
					for (int q=1; q<(data_ylin.cols - 1); q++)
					{
						//find the value of the data_ylin at each of the klinear points
						// data_ylin = data_y(nearestkindex) + fractionalk(nearestkindex)*slopes(nearestkindex)
						//std::cout << "q=" << q << " nearestk=" << nearestkindex.at<int>(0,q) << std::endl;
						data_ylin.at<double>(p,q) = data_y.at<double>(p, nearestkindex.at<int>(0,q) ) 
							 + fractionalk.at<double>(nearestkindex.at<int>(0,q)) 
								* slopes.at<double>(p,nearestkindex.at<int>(0,q) );	
						//printf("data_ylin(%d,%d)=%f \n",p,q,data_ylin.at<double>(p,q));
					}
					//data_ylin.at<double>(p, 0) = 0;
					//data_ylin.at<double>(p, numfftpoints) = 0;
					
				}

                // InvFFT
                
                nr = getOptimalDFTSize( data_ylin.rows );	//128 when taking transpose(opm, data_y);
                nc = getOptimalDFTSize( data_ylin.cols );	//96
                //nc = nc * 4;		// 4x oversampling
               
                 
                //copyMakeBorder(data_ylin, padded, 0, nr - data_ylin.rows, 0, nc - data_ylin.cols, BORDER_CONSTANT, Scalar::all(0));
                //normalize(data_ylin, paddedn, 0, 1, NORM_MINMAX);
                //imshow("linearized", paddedn);
               
				// smoothing by weighted moving average
				//data_ylin = smoothmovavg(data_ylin, 5);
				Mat planes[] = {Mat_<float>(data_ylin), Mat::zeros(data_ylin.size(), CV_32F)};
				Mat complexI;
				merge(planes, 2, complexI);         // Add to the expanded another plane with zeros

				dft(complexI, complexI, DFT_ROWS|DFT_INVERSE);            // this way the result may fit in the source matrix

				// compute the magnitude and switch to logarithmic scale
				// => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
				split(complexI, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
				magnitude(planes[0], planes[1], magI); 
				

				if(indextemp < averages)
				{
					bscantemp = magI.colRange(0, numdisplaypoints);
					bscantemp.convertTo(bscantemp,CV_64F);
					accumulate(bscantemp, bscantransposed);
					indextemp++;
				}
				else
				{
					indextemp = 0;
					transpose(bscantransposed, bscan); 
					
					bscan += Scalar::all(0.000001);   	// to prevent log of 0                 
					log(bscan, bscanlog);					// switch to logarithmic scale
															//convert to dB = 20 log10(value), from the natural log above
					bscandb = 20.0 * bscanlog / 2.303;

					normalize(bscandb, bscandisp, 0, 1, NORM_MINMAX);	// normalize the log plot for display
					bscandisp.convertTo(bscandisp, CV_8UC1, 255.0);
					applyColorMap(bscandisp, cmagI, COLORMAP_JET);
					
					imshow( "Bscan", cmagI );
					//imshow( "Bscan", bscan );
					
					if (skeypressed==1)	
                 
					{
						
					indexi++;
					sprintf(filename, "bscan%03d.png",indexi);
					sprintf(filenamec, "bscanc%03d.png",indexi);
					//normalize(bscan, bscan, 0, 255, NORM_MINMAX);
					
#ifdef __unix__
					strcpy(pathname,dirname);
					strcat(pathname,"/");
					strcat(pathname,filename);
					imwrite(pathname, bscandisp);
					
					strcpy(pathname,dirname);
					strcat(pathname,"/");
					strcat(pathname,filenamec);
					imwrite(pathname, cmagI);
					
					sprintf(filename, "bscan%03d",indexi);
					outfile<< filename << "=";
					outfile<<bscan;
					outfile<<";"<<std::endl;
					
#else
					imwrite(filename, bscandisp);
					imwrite(filenamec, cmagI);
					sprintf(filename, "bscan%03d",indexi);
					outfile << filename << bscan;
#endif		 	
					skeypressed=0; // if necessary, comment, do for bscanl also, then make it 0 	 
						
					}
				
					bscantransposed = Mat::zeros(Size(numfftpoints/2, oph), CV_64F);
				}
				 
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
                
                
					 
                key=waitKey(30); // wait 30 milliseconds for keypress
                // max frame rate at 1280x960 is 30 fps => 33 milliseconds
                
                switch (key) 
                {
                
                case 27: //ESC key
					doneflag=1;
					break;
					
					/*
					case '+':
				 
						camtime = camtime + 100;
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
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
				 
						camtime = camtime - 100;
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
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
						if(ret == QHYCCD_SUCCESS)
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
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
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
						if(ret == QHYCCD_SUCCESS)
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
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
						{
							printf("Exp time = %d \n", camtime);
						}
						else
						{
							printf("CONTROL_EXPOSURE fail\n");
							goto failure;
						}
						break; */
						
                case 's':  
					 
							skeypressed=1;
					break;
                
                case 'b':  
					 
							bkeypressed=1;
					break; 
					
				case 'p':  
					 
							pkeypressed=1;
					break; 
					
				 
				default:
					break;
                
				} 
            
				if(doneflag==1)
				{
					break;
				}   

			 }  // if ret success end
        } // inner while loop end
        
	//} // end of if found 
        
#ifdef __unix__
	outfile << "% Parameters were - camgain, camtime, bpp, w , h , camspeed, usbtraffic, binvalue" << std::endl;
	outfile << "% " << camgain;
	outfile << ", " << camtime;
	outfile << ", " << bpp;
	outfile << ", " << w;
	outfile << ", " << h;
	outfile << ", " << camspeed;
	outfile << ", " << usbtraffic;
	outfile << ", " << binvalue;
				


				
		 
#else
		//imwrite("bscan.png", normfactorforsave*bscan);
		 
		
		outfile << "camgain" << camgain;
		outfile << "camtime" << camtime;
		 
		
#endif

         
		

 
    /*
    if(camhandle)
    {
        StopQHYCCDLive(camhandle);

        ret = CloseQHYCCD(camhandle);
        if(ret == QHYCCD_SUCCESS)
        {
            printf("Closed QHYCCD.\n");
        }
        else
        {
            goto failure;
        }
    }
    

	
    ret = ReleaseQHYCCDResource();
    if(ret == QHYCCD_SUCCESS)
    {
        printf("SDK Resource released successfully.\n");
    }
    else
    {
        goto failure;
    }
	*/
 
    return 0; 

failure:
    printf("Fatal error !! \n");
    return 1;
}

