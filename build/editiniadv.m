% Hari Nandakumar
% 18 June 2019
 
% UI for editing ini file BscanFFT.ini

## 
## modified from demo_uicontrol,
# 20.03.2017 Andreas Weber <andy@josoansi.de> Demo which has the aim to show all available GUI elements.
## Useful since Octave 4.0

close all
clear 

graphics_toolkit qt

global inifilename = 'testfile.ini';

fid = fopen (inifilename);
global inifilerows = textscan(fid,'%s');
fclose(fid);

%fid = fopen (inifilename,'w');

% each line of ini file is one row in inifilerows

%printf('This is the 3rd row - %s',inifilerows{1}{3});
% fprintf(myfile,"%s\n",inifilerows{1}{3})
% fprintf(myfile,"%s\n",inifilerows{1}{4}) - even the numbers are written as strings
%for indexi=1:52
%fprintf(fid,"%s\n",inifilerows{1}{indexi})
%end
%fprintf(fid,"\n");
%fclose(fid);

function savetofile(obj, eventdata)
  global inifilename; % Declare global
  global inifilerows;
  
  fid = fopen (inifilename,'w');
      for indexi=1:52
          fprintf(fid,"%s\n",inifilerows{1}{indexi})
      end
      fprintf(fid,"\n");
      fclose(fid);
endfunction

function update_ini (obj, init = false)
  global inifilerows;
  ## gcbo holds the handle of the control
  h = guidata (obj);
  replot = false;
  recalc = false;
  switch (gcbo)
    case {h.adv_pushbutton}
      % call up the simplified edit
      editini
      
    case {h.lamin_edit}
      inifilerows{1}{42} = get (gcbo, "string");
      
    case {h.lamax_edit}
      inifilerows{1}{44} = get (gcbo, "string");
      
    case {h.nfft_edit}
      inifilerows{1}{28} = get (gcbo, "string");
      
    case {h.navg_edit}
      inifilerows{1}{26} = get (gcbo, "string");
      
    case {h.mavg_edit}
      inifilerows{1}{34} = get (gcbo, "string");
      
    case {h.numdisp_edit}
      inifilerows{1}{40} = get (gcbo, "string");
      
    case {h.bin_popup}
      inifilerows{1}{22} = get (h.bin_popup, "string"){get (h.bin_popup, "value")};
    
    case {h.msav_popup}
      inifilerows{1}{32} = get (h.msav_popup, "string"){get (h.msav_popup, "value")};
    
    case {h.camspeed_popup}
      inifilerows{1}{14} = get (h.camspeed_popup, "string"){get (h.camspeed_popup, "value")};
    
    case {h.usbtr_popup}
      inifilerows{1}{20} = get (h.usbtr_popup, "string"){get (h.usbtr_popup, "value")};
    
    case {h.savfr_popup}
      inifilerows{1}{30} = get (h.savfr_popup, "string"){get (h.savfr_popup, "value")};
    
    case {h.savint_popup}
      inifilerows{1}{36} = get (h.savint_popup, "string"){get (h.savint_popup, "value")};
    
    case {h.rown_popup}
      inifilerows{1}{50} = get (h.rown_popup, "string"){get (h.rown_popup, "value")};
    
    case {h.dontn_popup}
      inifilerows{1}{52} = get (h.dontn_popup, "string"){get (h.dontn_popup, "value")};
    
    case {h.movav_popup}
      inifilerows{1}{38} = get (h.movav_popup, "string"){get (h.movav_popup, "value")};
    
    case {h.median_popup}
      inifilerows{1}{46} = get (h.median_popup, "string"){get (h.median_popup, "value")};
    
    case {h.gain_slider}
      newval = get (h.gain_slider, "value");                         %get value from the slider
      newval = round(newval);                         %round off this value
      set(h.gain_slider, 'Value', newval);
      inifilerows{1}{4} = num2str(get (h.gain_slider, "value"));
      set(h.gain_edit, 'String', num2str(get (h.gain_slider, "value")));
      
    case {h.gain_edit}
      inifilerows{1}{4} = get (h.gain_edit, "string");
      if str2num(inifilerows{1}{4}) > 100
        inifilerows{1}{4} = '100';
        set(h.gain_edit, 'String', inifilerows{1}{4});
      end
      if str2num(inifilerows{1}{4}) < 1
        inifilerows{1}{4} = '1';
        set(h.gain_edit, 'String', inifilerows{1}{4});
      end  
      set(h.gain_slider, 'Value', str2num(inifilerows{1}{4}));
      
     
      
  endswitch


  
  
endfunction


## lam
h.lam_label = uicontrol ("style", "text",
                                "units", "normalized",
                                "string", "Lambda min & max:",
                                "horizontalalignment", "left",
                                "position", [0.05 0.90 0.35 0.08]);

h.lamin_edit = uicontrol ("style", "edit",
                               "units", "normalized",
                               "string", inifilerows{1}{42},
                               "callback", @update_ini,
                               "position", [0.05 0.85 0.15 0.06]);
                               
h.lamax_edit = uicontrol ("style", "edit",
                               "units", "normalized",
                               "string", inifilerows{1}{44},
                               "callback", @update_ini,
                               "position", [0.22 0.85 0.15 0.06]);
                               
## bin
h.bin_label = uicontrol ("style", "text",
                               "units", "normalized",
                               "string", "Bin:",
                               "horizontalalignment", "left",
                               "position", [0.40 0.90 0.35 0.08]);


if (inifilerows{1}{22}=='8')                             
h.bin_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"8",
                                          "1" ,
                                          "2",
                                          "4",
                                          "10"
                                          },
                               "callback", @update_ini,
                               "position", [0.40 0.85 0.09 0.06]);
endif
%else % change the order of the popup
if (inifilerows{1}{22}=='1')
h.bin_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"1",
                                          "2" ,
                                          "4",
                                          "8",
                                          "10"
                                          },
                               "callback", @update_ini,
                               "position", [0.40 0.85 0.09 0.06]);
endif

if (inifilerows{1}{22}=='2')
h.bin_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"2",
                                          "1" ,
                                          "4",
                                          "8",
                                          "10"
                                          },
                               "callback", @update_ini,
                               "position", [0.40 0.85 0.09 0.06]);
endif

if (inifilerows{1}{22}=='4')
h.bin_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"4",
                                          "1",
                                          "2" ,
                                          "8",
                                          "10"
                                          },
                               "callback", @update_ini,
                               "position", [0.40 0.85 0.09 0.06]);
endif

if (inifilerows{1}{22}=='10')
h.bin_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"10",
                                          "1";
                                          "2" ,
                                          "4",
                                          "8"
                                          },
                               "callback", @update_ini,
                               "position", [0.40 0.85 0.09 0.06]);
endif

## manual averaging
h.msav_label = uicontrol ("style", "text",
                               "units", "normalized",
                               "string", "Manual averaging:",
                               "horizontalalignment", "left",
                               "position", [0.60 0.75 0.25 0.08]);

h.mavg_edit = uicontrol ("style", "edit",
                               "units", "normalized",
                               "string", inifilerows{1}{34},
                               "callback", @update_ini,
                               "position", [0.65 0.70 0.13 0.06]);

                               
if (inifilerows{1}{32}=='0')                             
h.msav_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"0",
                                          "1" 
                                          },
                               "callback", @update_ini,
                               "position", [0.55 0.70 0.08 0.06]);
%endif
else % change the order of the popup
 
h.msav_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"1",
                                          "0" 
                                          },
                               "callback", @update_ini,
                               "position", [0.55 0.70 0.08 0.06]);
endif



## savint
h.savint_label = uicontrol ("style", "text",
                               "units", "normalized",
                               "string", "Save ifgrams:  Rowwise norm:   Don't norm:",
                               "horizontalalignment", "left",
                               "position", [0.4 0.60 0.55 0.08]);


if (inifilerows{1}{36}=='0')                             
h.savint_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"0",
                                          "1" 
                                          },
                               "callback", @update_ini,
                               "position", [0.40 0.55 0.08 0.06]);
%endif
else % change the order of the popup
 
h.savint_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"1",
                                          "0" 
                                          },
                               "callback", @update_ini,
                               "position", [0.40 0.55 0.08 0.06]);
endif


## nfft
h.nfft_label = uicontrol ("style", "text",
                                "units", "normalized",
                                "string", "FFT points:",
                                "horizontalalignment", "left",
                                "position", [0.5 0.90 0.35 0.08]);

h.nfft_edit = uicontrol ("style", "edit",
                               "units", "normalized",
                               "string", inifilerows{1}{28},
                               "callback", @update_ini,
                               "position", [0.5 0.85 0.13 0.06]);
                               
## navg
h.navg_label = uicontrol ("style", "text",
                                "units", "normalized",
                                "string", "Averages:",
                                "horizontalalignment", "left",
                                "position", [0.65 0.90 0.13 0.08]);

h.navg_edit = uicontrol ("style", "edit",
                               "units", "normalized",
                               "string", inifilerows{1}{26},
                               "callback", @update_ini,
                               "position", [0.65 0.85 0.13 0.06]);

## savfr
h.savfr_label = uicontrol ("style", "text",
                               "units", "normalized",
                               "string", "Save frames:",
                               "horizontalalignment", "left",
                               "position", [0.80 0.9 0.15 0.08]);


if (inifilerows{1}{30}=='0')                             
h.savfr_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"0",
                                          "1" 
                                          },
                               "callback", @update_ini,
                               "position", [0.85 0.85 0.08 0.06]);
%endif
else % change the order of the popup
 
h.savfr_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"1",
                                          "0" 
                                          },
                               "callback", @update_ini,
                               "position", [0.85 0.85 0.08 0.06]);
endif

## row-wise normalize, don't normalize

if (inifilerows{1}{50}=='0')                             
h.rown_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"0",
                                          "1" 
                                          },
                               "callback", @update_ini,
                               "position", [0.60 0.55 0.08 0.06]);
%endif
else % change the order of the popup
 
h.rown_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"1",
                                          "0" 
                                          },
                               "callback", @update_ini,
                               "position", [0.60 0.55 0.08 0.06]);
endif

if (inifilerows{1}{52}=='0')                             
h.dontn_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"0",
                                          "1" 
                                          },
                               "callback", @update_ini,
                               "position", [0.80 0.55 0.08 0.06]);
%endif
else % change the order of the popup
 
h.dontn_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"1",
                                          "0" 
                                          },
                               "callback", @update_ini,
                               "position", [0.80 0.55 0.08 0.06]);
endif

# save

h.save_pushbutton = uicontrol ("style", "pushbutton",
                                 "units", "normalized",
                                 "string", "Save ini file",
                                 "callback", @savetofile,
                                 "position", [0.77 0.05 0.2 0.1]);

 
h.adv_pushbutton = uicontrol ("style", "pushbutton",
                                "units", "normalized",
                                "string", "Basic",
                                "callback", @update_ini,
                                "position", [0.10 0.05 0.2 0.1]);
                                
h.gain_label = uicontrol ("style", "text",
                                "units", "normalized",
                                "string", "Gain:",
                                "horizontalalignment", "left",
                                "position", [0.05 0.75 0.15 0.08]);
                                
## gain
h.gain_edit = uicontrol ("style", "edit",
                           "units", "normalized",
                           "string", inifilerows{1}{4},
                           "callback", @update_ini,
                           "horizontalalignment", "left",
                           "position", [0.25 0.70 0.07 0.06]);

val_min = 1;
val_max = 100;
stepSz = [1,10]; % <- [minorStep,majorStep]
                           
h.gain_slider = uicontrol ("style", "slider",
                            "units", "normalized",
                            "string", "width",
                            "callback", @update_ini,
                            'Min',val_min, 'Max',val_max',
                    'SliderStep',stepSz/(val_max-val_min), 
                            "value", str2num(inifilerows{1}{4}),
                          %"value", 1280,
                            "position", [0.05 0.70 0.20 0.06]);

## camspeed, usbtraffic

h.camspeed_label = uicontrol ("style", "text",
                                "units", "normalized",
                                "string", "Camspeed:",
                                "horizontalalignment", "left",
                                "position", [0.05 0.60 0.35 0.08]);
                                
if (inifilerows{1}{14}=='0')                             
h.camspeed_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"0",
                                          "1" ,
                                          "2"
                                          },
                               "callback", @update_ini,
                               "position", [0.05 0.55 0.10 0.06]);
endif
%else % change the order of the popup
if (inifilerows{1}{14}=='1')
h.camspeed_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"1",
                                          "0" ,
                                          "2"
                                          },
                               "callback", @update_ini,
                               "position", [0.05 0.55 0.10 0.06]);
endif
if (inifilerows{1}{14}=='2')
h.camspeed_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"2",
                                          "0" ,
                                          "1"
                                          },
                               "callback", @update_ini,
                               "position", [0.05 0.55 0.10 0.06]);
endif


h.usbtr_label = uicontrol ("style", "text",
                                "units", "normalized",
                                "string", "USB traffic:",
                                "horizontalalignment", "left",
                                "position", [0.20 0.60 0.13 0.08]);
                                
if (inifilerows{1}{20}=='0')                             
h.usbtr_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"0",
                                          "1" ,
                                          "2"
                                          },
                               "callback", @update_ini,
                               "position", [0.20 0.55 0.10 0.06]);
endif
%else % change the order of the popup
if (inifilerows{1}{20}=='1')
h.usbtr_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"1",
                                          "0" ,
                                          "2"
                                          },
                               "callback", @update_ini,
                               "position", [0.20 0.55 0.10 0.06]);
endif
if (inifilerows{1}{20}=='2')
h.usbtr_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"2",
                                          "0" ,
                                          "1"
                                          },
                               "callback", @update_ini,
                               "position", [0.20 0.55 0.10 0.06]);
endif

## moving average
h.movav_label = uicontrol ("style", "text",
                               "units", "normalized",
                               "string", "Moving average:  2D Median filt:   Num disp points:   Incr fft points:",
                               "horizontalalignment", "left",
                               "position", [0.05 0.45 0.90 0.08]);


if (inifilerows{1}{38}=='0')                             
h.movav_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"0",
                                          "3" ,
                                          "5",
                                          "7",
                                          "9"
                                          },
                               "callback", @update_ini,
                               "position", [0.1 0.40 0.08 0.06]);
endif
%else % change the order of the popup
if (inifilerows{1}{38}=='3') 
h.movav_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"3",
                                          "0" ,
                                          "5",
                                          "7",
                                          "9"
                                          },
                               "callback", @update_ini,
                               "position", [0.10 0.40 0.08 0.06]);
endif

if (inifilerows{1}{38}=='5') 
h.movav_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"5",
                                          "0" ,
                                          "3",
                                          "7",
                                          "9"
                                          },
                               "callback", @update_ini,
                               "position", [0.10 0.40 0.08 0.06]);
endif

if (inifilerows{1}{38}=='7') 
h.movav_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"7",
                                          "0" ,
                                          "3",
                                          "5",
                                          "9"
                                          },
                               "callback", @update_ini,
                               "position", [0.10 0.40 0.08 0.06]);
endif

if (inifilerows{1}{38}=='9') 
h.movav_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"9",
                                          "0" ,
                                          "3",
                                          "5",
                                          "7"
                                          },
                               "callback", @update_ini,
                               "position", [0.10 0.40 0.08 0.06]);
endif                           

## 2D median filter

if (inifilerows{1}{46}=='0')                             
h.median_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"0",
                                          "3" ,
                                          "5",
                                          "7",
                                          "9"
                                          },
                               "callback", @update_ini,
                               "position", [0.3 0.40 0.08 0.06]);
endif
%else % change the order of the popup
if (inifilerows{1}{46}=='3') 
h.median_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"3",
                                          "0" ,
                                          "5",
                                          "7",
                                          "9"
                                          },
                               "callback", @update_ini,
                               "position", [0.30 0.40 0.08 0.06]);
endif

if (inifilerows{1}{46}=='5') 
h.median_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"5",
                                          "0" ,
                                          "3",
                                          "7",
                                          "9"
                                          },
                               "callback", @update_ini,
                               "position", [0.30 0.40 0.08 0.06]);
endif

if (inifilerows{1}{46}=='7') 
h.median_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"7",
                                          "0" ,
                                          "3",
                                          "5",
                                          "9"
                                          },
                               "callback", @update_ini,
                               "position", [0.30 0.40 0.08 0.06]);
endif

if (inifilerows{1}{46}=='9') 
h.median_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"9",
                                          "0" ,
                                          "3",
                                          "5",
                                          "7"
                                          },
                               "callback", @update_ini,
                               "position", [0.30 0.40 0.08 0.06]);
endif 

# num disp points
# calculated from bin value and resolution
% = width / bin / 2
width =  str2num(inifilerows{1}{10});
bin =   str2num(inifilerows{1}{22});
inifilerows{1}{40} = num2str(width/bin/2);
                     
h.numdisp_edit = uicontrol ("style", "edit",
                               "units", "normalized",
                               "string", inifilerows{1}{40},
                               "callback", @update_ini,
                               "position", [0.45 0.4 0.15 0.06]);
                               

# increase fft points
# calculated from fft points and resolution
% = fftpoints/(width/bin)
%width =  str2num(inifilerows{1}{10});
%bin =   str2num(inifilerows{1}{22});
nfft =   str2num(inifilerows{1}{28});
inifilerows{1}{48} = num2str(nfft/(width/bin));
                     
h.incrfft_edit = uicontrol ("style", "edit",
                               "units", "normalized",
                               "string", inifilerows{1}{48},
                               "callback", @update_ini,
                               "position", [0.7 0.4 0.10 0.06]);
                               

                               
set (gcf, "color", get(0, "defaultuicontrolbackgroundcolor"))
guidata (gcf, h)
update_ini (gcf, true);
set(gcf, 'MenuBar', 'none');
set(gcf, 'ToolBar', 'none');
set(gcf,'name','Advanced Settings','numbertitle','off');
 

