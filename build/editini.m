% Hari Nandakumar
% 16 June 2019

% UI for editing ini file BscanFFT.ini

## 
## modified from demo_uicontrol,
# 20.03.2017 Andreas Weber <andy@josoansi.de> Demo which has the aim to show all available GUI elements.
## Useful since Octave 4.0

close all
clear h

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
      % call up the advanced edit
      editiniadv
      
    case {h.desc_edit}
      inifilerows{1}{24} = get (gcbo, "string");
      
    case {h.bpp_popup}
      inifilerows{1}{8} = get (h.bpp_popup, "string"){get (h.bpp_popup, "value")};
    
    case {h.width_slider}
      newval = get (h.width_slider, "value");                         %get value from the slider
      newval = 4.*round(newval./4);                         %round off this value
      set(h.width_slider, 'Value', newval);
      inifilerows{1}{10} = num2str(get (h.width_slider, "value"));
      set(h.width_edit, 'String', num2str(get (h.width_slider, "value")));
      
    case {h.width_edit}
      inifilerows{1}{10} = get (h.width_edit, "string");
      if str2num(inifilerows{1}{10}) > 1280
        inifilerows{1}{10} = '1280';
        set(h.width_edit, 'String', inifilerows{1}{10});
      end
      if str2num(inifilerows{1}{10}) < 4
        inifilerows{1}{10} = '4';
        set(h.width_edit, 'String', inifilerows{1}{10});
      end  
      set(h.width_slider, 'Value', str2num(inifilerows{1}{10}));
      
     case {h.height_slider}
      newval = get (h.height_slider, "value");                         %get value from the slider
      newval = 4.*round(newval./4);                         %round off this value
      set(h.height_slider, 'Value', newval);
      inifilerows{1}{12} = num2str(get (h.height_slider, "value"));
      set(h.height_edit, 'String', inifilerows{1}{12});
    
      
     case {h.height_edit}
      inifilerows{1}{12} = get (h.height_edit, "string");
      if str2num(inifilerows{1}{12}) > 960
        inifilerows{1}{12} = '960';
        set(h.height_edit, 'String', inifilerows{1}{12});
      end
      if str2num(inifilerows{1}{12}) < 4
        inifilerows{1}{12} = '4';
        set(h.height_edit, 'String', inifilerows{1}{12});
      end  
      set(h.height_slider, 'Value', str2num(inifilerows{1}{12}));
      
    case {h.exp_slider}
      newval = get (h.exp_slider, "value");                         %get value from the slider
      newval = 100.*round(newval./100);                         %round off this value
      set(h.exp_slider, 'Value', newval);
      inifilerows{1}{6} = num2str(get (h.exp_slider, "value"));
      set(h.exp_edit, 'String', num2str(get (h.exp_slider, "value")));
      
    case {h.exp_edit}
      inifilerows{1}{6} = get (h.exp_edit, "string");
      if str2num(inifilerows{1}{10}) > 1000000
        inifilerows{1}{6} = '1000000';
        set(h.exp_edit, 'String', inifilerows{1}{6});
      end
      if str2num(inifilerows{1}{6}) < 100
        inifilerows{1}{6} = '100';
        set(h.exp_edit, 'String', inifilerows{1}{6});
      end  
      set(h.exp_slider, 'Value', str2num(inifilerows{1}{6}));
    
      
  endswitch


  
  
endfunction


## description
h.desc_label = uicontrol ("style", "text",
                                "units", "normalized",
                                "string", "Description:",
                                "horizontalalignment", "left",
                                "position", [0.05 0.90 0.35 0.08]);

h.desc_edit = uicontrol ("style", "edit",
                               "units", "normalized",
                               "string", inifilerows{1}{24},
                               "callback", @update_ini,
                               "position", [0.06 0.85 0.35 0.06]);
                               
## bpp
h.bpp_label = uicontrol ("style", "text",
                               "units", "normalized",
                               "string", "Bits per pixel:",
                               "horizontalalignment", "left",
                               "position", [0.65 0.90 0.35 0.08]);


if (inifilerows{1}{8}=='8')                             
h.bpp_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"8",
                                          "16" 
                                          },
                               "callback", @update_ini,
                               "position", [0.65 0.85 0.10 0.06]);
else % reverse the order of the popup
h.bpp_popup = uicontrol ("style", "popupmenu",
                               "units", "normalized",
                               "string", {"16",
                                          "8" 
                                          },
                               "callback", @update_ini,
                               "position", [0.65 0.85 0.10 0.06]);
endif

 

h.save_pushbutton = uicontrol ("style", "pushbutton",
                                 "units", "normalized",
                                 "string", "Save ini file",
                                 "callback", @savetofile,
                                 "position", [0.77 0.05 0.2 0.1]);

 
h.adv_pushbutton = uicontrol ("style", "pushbutton",
                                "units", "normalized",
                                "string", "Advanced",
                                "callback", @update_ini,
                                "position", [0.10 0.05 0.2 0.1]);
                                
h.res_label = uicontrol ("style", "text",
                                "units", "normalized",
                                "string", "Capture Resolution - w & h:",
                                "horizontalalignment", "left",
                                "position", [0.05 0.65 0.35 0.08]);
                                
## width
h.width_edit = uicontrol ("style", "edit",
                           "units", "normalized",
                           "string", inifilerows{1}{10},
                           "callback", @update_ini,
                           "horizontalalignment", "left",
                           "position", [0.50 0.60 0.10 0.06]);

val_min = 4;
val_max = 1280;
stepSz = [4,40]; % <- [minorStep,majorStep]
                           
h.width_slider = uicontrol ("style", "slider",
                            "units", "normalized",
                            "string", "width",
                            "callback", @update_ini,
                            'Min',val_min, 'Max',val_max',
                    'SliderStep',stepSz/(val_max-val_min), 
                            "value", str2num(inifilerows{1}{10}),
                          %"value", 1280,
                            "position", [0.05 0.60 0.35 0.06]);

                            
## height
h.height_edit = uicontrol ("style", "edit",
                           "units", "normalized",
                           "string", inifilerows{1}{12},
                           "callback", @update_ini,
                           "horizontalalignment", "left",
                           "position", [0.50 0.50 0.10 0.06]);

val_min = 4;
val_max = 960;
stepSz = [4,40]; % <- [minorStep,majorStep]
                           
h.height_slider = uicontrol ("style", "slider",
                            "units", "normalized",
                            "string", "width",
                            "callback", @update_ini,
                            'Min',val_min, 'Max',val_max',
                    'SliderStep',stepSz/(val_max-val_min), 
                            "value", str2num(inifilerows{1}{12}),
                            "position", [0.05 0.50 0.35 0.06]);
# exp time                           
h.exp_label = uicontrol ("style", "text",
                                "units", "normalized",
                                "string", "Exposure time (microsec):",
                                "horizontalalignment", "left",
                                "position", [0.05 0.40 0.35 0.08]);
                                

h.exp_edit = uicontrol ("style", "edit",
                           "units", "normalized",
                           "string", inifilerows{1}{6},
                           "callback", @update_ini,
                           "horizontalalignment", "left",
                           "position", [0.50 0.35 0.15 0.06]);

val_min = 100;
val_max = 1000000;
stepSz = [100,1000]; % <- [minorStep,majorStep]
                           
h.exp_slider = uicontrol ("style", "slider",
                            "units", "normalized",
                            "string", "width",
                            "callback", @update_ini,
                            'Min',val_min, 'Max',val_max',
                    'SliderStep',stepSz/(val_max-val_min), 
                            "value", str2num(inifilerows{1}{10}),
                          "position", [0.05 0.35 0.35 0.06]);


set (gcf, "color", get(0, "defaultuicontrolbackgroundcolor"))
guidata (gcf, h)
update_ini (gcf, true);
set(gcf, 'MenuBar', 'none');
set(gcf, 'ToolBar', 'none');
set(gcf,'name','Settings','numbertitle','off')