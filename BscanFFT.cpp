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
 * 15 Sep 2018
 *  
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
 



// Interpolation using 

using namespace cv;

int main(int argc,char *argv[])
{
    int num = 0;
    qhyccd_handle *camhandle=NULL;
    int ret;
    char id[32];
    //char camtype[16];
    int found = 0;
    unsigned int w,h,bpp=8,channels, cambitdepth=16, numofframes=100; 
    unsigned int numofm1slices=10, numofm2slices=10, firstaccum, secondaccum;
    unsigned int offsetx=0, offsety=0;
    unsigned int indexi, averages=1, indextemp, opw, oph;

     
    int camtime = 1,camgain = 1,camspeed = 1,cambinx = 2,cambiny = 2,usbtraffic = 10;
    int camgamma = 1, binvalue=1, normfactor=1, normfactorforsave=25;
    
     
    bool doneflag=0, skeypressed=0, bkeypressed=0;
    
    w=640;
    h=480;
    
    int  fps, key, bscanat;
    int t_start,t_end;
    
    std::ifstream infile("BscanFFT.ini");
    std::string tempstring;
    char dirdescr[60];
    sprintf(dirdescr, "_");
     
	namedWindow("show",0); // 0 = WINDOW_NORMAL
	moveWindow("show", 20, 0);
	 
	namedWindow("Bscan",0); // 0 = WINDOW_NORMAL
	moveWindow("Bscan", 800, 0);
	
	
	char dirname[80];
	char filename[20];
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
			infile.close();
		  }

		  else std::cout << "Unable to open ini file, using defaults.";
		  
			
	   
	   
    ///////////////////////
    // init camera etc
    
    cambitdepth = bpp;
    opw = w/binvalue;
	oph = h/binvalue;
	
	Mat ROI;
	Mat plot_result;
	Mat plot_result2;
	 
	Mat data_y( opw, oph, CV_64F );
	Mat data_yb( opw, oph, CV_64F );
	Mat padded;
	 
	int nr, nc;
	
	Mat m, opm, opmvector, bscan, bscantemp, bscantransposed;
	Scalar meanval;
	Mat lambdas, k, klinear;
	double lambdamin, lambdamax, kmin, kmax;
	double pi = 3.141592653589793;
	
	double minVal, maxVal, pixVal;
	//minMaxLoc( m, &minVal, &maxVal, &minLoc, &maxLoc );
	
	// assuming current data_y's each row goes from
	// lambda_min to lambda_max
	// 830 nm to 870 nm,
	lambdamin = 830e-9;
	lambdamax = 870e-9;
	kmin = 2*pi/lambdamax;
	kmax = 2*pi/lambdamin;
	double deltalambda = (lambdamax - lambdamin ) / data_y.cols;
	double deltak = (kmax - kmin) / data_y.cols;
	
	// lambdas = linspace(830e-9, 870e-9, data_y.cols)
	// k = 2*pi/lambdas
	// klinear = linspace(kmin, kmax, data_y.cols)
	lambdas = Mat::zeros(cv::Size(1, data_y.cols), CV_64F);
	klinear = Mat::zeros(cv::Size(1, data_y.cols), CV_64F);
	
	for (indextemp=0; indextemp<(data_y.cols); indextemp++) 
	{
		lambdas.at<double>(0,indextemp) = lambdamin + indextemp*deltalambda;
		k = 2*pi / lambdas;
		klinear.at<double>(0,indextemp) = kmin + indextemp*deltak;
		
	}
	
	
	timenow = localtime(&now);
	
	strftime(dirname, sizeof(dirname), "%Y-%m-%d_%H_%M_%S-", timenow);

	strcat(dirname, dirdescr);
#ifdef _WIN64
	CreateDirectoryA(dirname, NULL);
	cv::FileStorage outfile;
	outfile.open("BscanFFT.xml", cv::FileStorage::WRITE);
#else
	mkdir(dirname, 0755);
#endif

#ifdef __unix__	
	sprintf(filename, "BscanFFT.m");
	strcpy(pathname,dirname);
	strcat(pathname,"/");
	strcat(pathname,filename);
	std::ofstream outfile(pathname);
#endif
    
    

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
            printf("connected to the first camera from the list,id is %s\n",id);
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
            printf("SetQHYCCDResolution success - width = %d !\n", w); 
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
        
        
        if (cambitdepth==8)
        {
			
			m  = Mat::zeros(cv::Size(w, h), CV_8U);    
		}
        else // is 16 bit
        {
			m  = Mat::zeros(cv::Size(w, h), CV_16U);
		}
		 
		
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
            printf("CONTROL_EXPOSURE =%d success!\n", camtime);
        }
        else
        {
            printf("CONTROL_EXPOSURE fail\n");
            goto failure;
        }
        t_start = time(NULL);
        fps = 0;
        
        indexi = 0;
        indextemp = 0;
        bscantransposed = Mat::zeros(Size(opw/2, oph), CV_64F);
	    
	    
        while(1)
        { 
            ret = GetQHYCCDLiveFrame(camhandle,&w,&h,&bpp,&channels,m.data);
            
            if (ret == QHYCCD_SUCCESS)  
            {
            resize(m, opm, Size(), 1.0/binvalue, 1.0/binvalue, INTER_AREA);	// binning (averaging)
            imshow("show",opm);
            opm.copyTo(data_y);
            //transpose(opm, data_y); 		// void transpose(InputArray src, OutputArray dst)
											// because we actually want the columns and not rows
											// using DFT_ROWS
											// But that has rolling shutter issues, so going back to rows
											
			if ( indexi == 0)
			{
				// if it is the first frame acquired, initialize data_yb
				data_y.copyTo(data_yb);
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
                // data_y = ( (data_y - data_yb) ./ data_yb ).*gausswin
                data_y.convertTo(data_y, CV_64F);
                data_yb.convertTo(data_yb, CV_64F);
                data_y =  (data_y - data_yb) / data_yb  ;
                
                // approximate DC removal
                meanval = mean(data_y);		// only the first value of this scalar is nonzero for us, meanval(0)
                data_y = data_y - meanval(0);
                
                // interpolate to linear k space
	
                // InvFFT
                
                nr = getOptimalDFTSize( data_y.rows );	//128 when taking transpose(opm, data_y);
                nc = getOptimalDFTSize( data_y.cols );	//96
                //nc = nc * 4;		// 4x oversampling
                 
                copyMakeBorder(data_y, padded, 0, nr - data_y.rows, 0, nc - data_y.cols, BORDER_CONSTANT, Scalar::all(0));

				Mat planes[] = {Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F)};
				Mat complexI;
				merge(planes, 2, complexI);         // Add to the expanded another plane with zeros

				dft(complexI, complexI, DFT_ROWS|DFT_INVERSE);            // this way the result may fit in the source matrix

				// compute the magnitude and switch to logarithmic scale
				// => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
				split(complexI, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
				magnitude(planes[0], planes[1], planes[0]);// planes[0] = magnitude
				Mat magI = planes[0];
				Mat cmagI;

				magI += Scalar::all(1);                    // switch to logarithmic scale
				log(magI, magI);
				if(indextemp < averages)
				{
					bscantemp = magI.colRange(0,nc/2);
					bscantemp.convertTo(bscantemp,CV_64F);
					accumulate(bscantemp, bscantransposed);
					indextemp++;
				}
				else
				{
					indextemp = 0;
					transpose(bscantransposed, bscan); 
					// remove dc
					bscan.row(0).setTo(Scalar(0));
				
					normalize(bscan, bscan, 0, 1, NORM_MINMAX);
					bscan.convertTo(bscan, CV_8UC1, 255.0);
					applyColorMap(bscan, cmagI, COLORMAP_JET);
					
					imshow( "Bscan", cmagI );
					
					bscantransposed = Mat::zeros(Size(opw/2, oph), CV_64F);
				}
				 
				 
				
				
				
            ////////////////////////////////////////////
                
                if (skeypressed==1)	
                 
					{
						
					indexi++;
					sprintf(filename, "bscan%03d.png",indexi);
					
#ifdef __unix__
					strcpy(pathname,dirname);
					strcat(pathname,"/");
					strcat(pathname,filename);
					imwrite(pathname, normfactorforsave*bscan);
					
#else
					imwrite(filename, normfactorforsave*bscan);
#endif		 	
					skeypressed=0; 	 
						
					}
				 
									
				if (bkeypressed==1)	
                 
					{
						
					 data_y.copyTo(data_yb);		// saves the "background" or source spectrum	
					 bkeypressed=0; 
						
					}	
					 
			

					 
					 
                key=waitKey(30); // wait 30 milliseconds for keypress
                // max frame rate at 1280x960 is 30 fps => 33 milliseconds
                
                switch (key) 
                {
                
                case 27: //ESC key
					doneflag=1;
					break;
					
					case '+':
				 
						camtime = camtime + 100;
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
						{
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
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
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
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
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
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
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
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
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
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
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
						}
						else
						{
							printf("CONTROL_EXPOSURE fail\n");
							goto failure;
						}
						break;
						
                case 's':  
					 
							skeypressed=1;
					break;
                
                case 'b':  
					 
							bkeypressed=1;
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
        
	} // end of if found 
        
#ifdef __unix__
        outfile<<"bscan=";
		outfile<<bscan;
		outfile<<";"<<std::endl;
		 
		outfile<<"% Parameters were - camgain, camtime, bpp, w , h , camspeed, usbtraffic, binvalue"<<std::endl;
				outfile<<"% "<<camgain; 
				outfile<<", "<<camtime;  
				outfile<<", "<<bpp; 
				outfile<<", "<<w ; 
				outfile<<", "<<h ; 
				outfile<<", "<<camspeed ;
				outfile<<", "<<usbtraffic ;
				outfile<<", "<<binvalue ;
				

		 
		strcpy(pathname,dirname);
		strcat(pathname,"/");
		strcat(pathname,"bscan.png");
		imwrite(pathname, normfactorforsave*bscan);
				
		 
#else
		//imwrite("bscan.png", normfactorforsave*bscan);
		 
		outfile << "bscan" << bscan;
		outfile << "camgain" << camgain;
		outfile << "camtime" << camtime;
		 
		
#endif

         
		

 
    
    if(camhandle)
    {
        StopQHYCCDLive(camhandle);

        ret = CloseQHYCCD(camhandle);
        if(ret == QHYCCD_SUCCESS)
        {
            printf("Close QHYCCD success!\n");
        }
        else
        {
            goto failure;
        }
    }
    

	
    ret = ReleaseQHYCCDResource();
    if(ret == QHYCCD_SUCCESS)
    {
        printf("Release SDK Resource  success!\n");
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

