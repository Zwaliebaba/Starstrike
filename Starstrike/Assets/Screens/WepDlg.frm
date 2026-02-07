FORM


//  Project:   STARS
//  File:      WepWin.frm
//
//  John DiCamillo Software Consulting
//  Copyright © 1998. All Rights Reserved.


form: {
   rect:       (  0,   0, 640, 480),
   screen_width:  640,
   screen_height: 480,

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "WepWin_640.pcx",

   defctrl: {
      base_color: ( 92,  92,  92),
      back_color: ( 92,  92,  92),
      fore_color: (255, 255, 255),
      font:       GUI,
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
   },

   ctrl: {
      id:         100,
      type:       camera,
      rect:       (12, 42, 618, 286),
      back_color: ( 0,  0,  0),
      font:       HUDbold,
      text:       "System"
   },

   // weapon list:

   defctrl: {
      back_color:  (62,106,151),
      font:        GUI,
      sticky:      false,
      transparent: false
   },


   ctrl: {
      id:         501,
      type:       button,
      rect:       (10, 345, 93, 25),
      text:       "Wep 1"
   },

   ctrl: {
      id:         511,
      type:       slider,
      active_color: (53,159,67),
      rect:       (103, 345, 16, 25),
      text:       "",
      style:      0x21,
      orientation: 1,
      back_color: (92, 92, 92),
      fore_color: (53,159,67)
   },

   ctrl: {
      id:         521,
      type:       button,
      rect:       (121, 345,  25, 25),
      sticky:     true,
      picture:    "Man.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         531,
      type:       button,
      rect:       (150, 345,  25, 25),
      sticky:     true,
      picture:    "Auto.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         541,
      type:       button,
      rect:       (179, 345,  25, 25),
      sticky:     true,
      picture:    "Def.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         551,
      type:       label,
      rect:       (208, 348,  40, 25),
      text:       "",
      transparent: true
   },



   ctrl: {
      id:         502,
      type:       button,
      rect:       (10, 377, 93, 25),
      text:       "Wep 1"
   },

   ctrl: {
      id:         512,
      type:       slider,
      active_color: (53,159,67),
      rect:       (103, 377, 16, 25),
      text:       "",
      style:      0x21,
      orientation: 1,
      back_color: (92, 92, 92),
      fore_color: (53,159,67)
   },

   ctrl: {
      id:         522,
      type:       button,
      rect:       (121, 377,  25, 25),
      sticky:     true,
      picture:    "Man.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         532,
      type:       button,
      rect:       (150, 377,  25, 25),
      sticky:     true,
      picture:    "Auto.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         542,
      type:       button,
      rect:       (179, 377,  25, 25),
      sticky:     true,
      picture:    "Def.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         552,
      type:       label,
      rect:       (208, 380,  40, 25),
      text:       "",
      transparent: true
   },



   ctrl: {
      id:         503,
      type:       button,
      rect:       (10, 409, 93, 25),
      text:       "Wep 1"
   },

   ctrl: {
      id:         513,
      type:       slider,
      active_color: (53,159,67),
      rect:       (103, 409, 16, 25),
      text:       "",
      style:      0x21,
      orientation: 1,
      back_color: (92, 92, 92),
      fore_color: (53,159,67)
   },

   ctrl: {
      id:         523,
      type:       button,
      rect:       (121, 409,  25, 25),
      sticky:     true,
      picture:    "Man.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         533,
      type:       button,
      rect:       (150, 409,  25, 25),
      sticky:     true,
      picture:    "Auto.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         543,
      type:       button,
      rect:       (179, 409,  25, 25),
      sticky:     true,
      picture:    "Def.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         553,
      type:       label,
      rect:       (208, 412,  40, 25),
      text:       "",
      transparent: true
   },



   ctrl: {
      id:         504,
      type:       button,
      rect:       (10, 441, 93, 25),
      text:       "Wep 1"
   },

   ctrl: {
      id:         514,
      type:       slider,
      active_color: (53,159,67),
      rect:       (103, 441, 16, 25),
      text:       "",
      style:      0x21,
      orientation: 1,
      back_color: (92, 92, 92),
      fore_color: (53,159,67)
   },

   ctrl: {
      id:         524,
      type:       button,
      rect:       (121, 441,  25, 25),
      sticky:     true,
      picture:    "Man.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         534,
      type:       button,
      rect:       (150, 441,  25, 25),
      sticky:     true,
      picture:    "Auto.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         544,
      type:       button,
      rect:       (179, 441,  25, 25),
      sticky:     true,
      picture:    "Def.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         554,
      type:       label,
      rect:       (208, 444,  40, 25),
      text:       "",
      transparent: true
   },



   // WEP DETAIL:

   defctrl: {
      base_color: ( 92,  92,  92),
      back_color: ( 92,  92,  92),
      fore_color: (255, 255, 255),
      font:       GUI,
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
   },

   defctrl: {
      back_color:  (62,106,151),
      font:        GUI,
      sticky:      false,
      transparent: false
   },


   ctrl: {
      id:         1300,
      type:       label,
      rect:       (260, 345, 180, 20),
      text:       "Contact List",
      transparent: true
   },

   ctrl: {
      id:         1301,
      type:       list,
      rect:       (260, 365, 180, 100),
      text:       "contact list",
      back_color: (21,21,21),
      font:       GUIsmall,

      style:            0x02,
      item_style:       0x02,
      selected_style:   0x02,
      scroll_bar:       2,
      leading:          2,
      show_headings:    true,
   },

   ctrl: {
      id:         1302,
      type:       list,
      rect:       (445, 365, 185, 100),
      text:       "system list",
      back_color: (21,21,21),
      font:       GUIsmall,

      style:            0x02,
      item_style:       0x02,
      selected_style:   0x02,
      scroll_bar:       2,
      leading:          2,
      show_headings:    true,
   }
}



form: {
   rect:       (  0,   0, 800, 600),
   screen_width:  800,
   screen_height: 600,

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "WepWin_800.pcx",

   defctrl: {
      base_color: ( 92,  92,  92),
      back_color: ( 92,  92,  92),
      fore_color: (255, 255, 255),
      font:       "GUI",
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
   },

   ctrl: {
      id:         100,
      type:       camera,
      rect:       (10, 42, 778, 404),
      back_color: ( 0,  0,  0),
      font:       HUDbold,
      text:       "CAMERA GOES HERE"
   },

   // weapon list:

   defctrl: {
      back_color:  (62,106,151),
      font:        GUI,
      sticky:      false,
      transparent: false
   },


   ctrl: {
      id:         501,
      type:       button,
      rect:       (10, 466, 93, 25),
      text:       "Wep 1"
   },

   ctrl: {
      id:         511,
      type:       slider,
      active_color: (53,159,67),
      rect:       (103, 466, 16, 25),
      text:       "",
      style:      0x21,
      orientation: 1,
      back_color: (92, 92, 92),
      fore_color: (53,159,67)
   },

   ctrl: {
      id:         521,
      type:       button,
      rect:       (121, 466,  25, 25),
      sticky:     true,
      picture:    "Man.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         531,
      type:       button,
      rect:       (150, 466,  25, 25),
      sticky:     true,
      picture:    "Auto.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         541,
      type:       button,
      rect:       (179, 466,  25, 25),
      sticky:     true,
      picture:    "Def.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         551,
      type:       label,
      rect:       (208, 469,  40, 25),
      text:       "",
      transparent: true
   },



   ctrl: {
      id:         502,
      type:       button,
      rect:       (10, 498, 93, 25),
      text:       "Wep 1"
   },

   ctrl: {
      id:         512,
      type:       slider,
      active_color: (53,159,67),
      rect:       (103, 498, 16, 25),
      text:       "",
      style:      0x21,
      orientation: 1,
      back_color: (92, 92, 92),
      fore_color: (53,159,67)
   },

   ctrl: {
      id:         522,
      type:       button,
      rect:       (121, 498,  25, 25),
      sticky:     true,
      picture:    "Man.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         532,
      type:       button,
      rect:       (150, 498,  25, 25),
      sticky:     true,
      picture:    "Auto.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         542,
      type:       button,
      rect:       (179, 498,  25, 25),
      sticky:     true,
      picture:    "Def.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         552,
      type:       label,
      rect:       (208, 501,  40, 25),
      text:       "",
      transparent: true
   },



   ctrl: {
      id:         503,
      type:       button,
      rect:       (10, 530, 93, 25),
      text:       "Wep 1"
   },

   ctrl: {
      id:         513,
      type:       slider,
      active_color: (53,159,67),
      rect:       (103, 530, 16, 25),
      text:       "",
      style:      0x21,
      orientation: 1,
      back_color: (92, 92, 92),
      fore_color: (53,159,67)
   },

   ctrl: {
      id:         523,
      type:       button,
      rect:       (121, 530,  25, 25),
      sticky:     true,
      picture:    "Man.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         533,
      type:       button,
      rect:       (150, 530,  25, 25),
      sticky:     true,
      picture:    "Auto.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         543,
      type:       button,
      rect:       (179, 530,  25, 25),
      sticky:     true,
      picture:    "Def.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         553,
      type:       label,
      rect:       (208, 533,  40, 25),
      text:       "",
      transparent: true
   },



   ctrl: {
      id:         504,
      type:       button,
      rect:       (10, 562, 93, 25),
      text:       "Wep 1"
   },

   ctrl: {
      id:         514,
      type:       slider,
      active_color: (53,159,67),
      rect:       (103, 562, 16, 25),
      text:       "",
      style:      0x21,
      orientation: 1,
      back_color: (92, 92, 92),
      fore_color: (53,159,67)
   },

   ctrl: {
      id:         524,
      type:       button,
      rect:       (121, 562,  25, 25),
      sticky:     true,
      picture:    "Man.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         534,
      type:       button,
      rect:       (150, 562,  25, 25),
      sticky:     true,
      picture:    "Auto.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         544,
      type:       button,
      rect:       (179, 562,  25, 25),
      sticky:     true,
      picture:    "Def.pcx",
      picture_type: 1,
      picture_loc: 4
   },

   ctrl: {
      id:         554,
      type:       label,
      rect:       (208, 565,  40, 25),
      text:       "",
      transparent: true
   },



   // WEP DETAIL:

   defctrl: {
      base_color: ( 92,  92,  92),
      back_color: ( 92,  92,  92),
      fore_color: (255, 255, 255),
      font:       GUI,
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
   },

   defctrl: {
      back_color:  (62,106,151),
      font:        GUI,
      sticky:      false,
      transparent: false
   },


   ctrl: {
      id:         1300,
      type:       label,
      rect:       (260, 465, 180, 20),
      text:       "Contact List",
      transparent: true
   },

   ctrl: {
      id:         1301,
      type:       list,
      rect:       (260, 485, 180, 100),
      text:       "contact list",
      back_color: (21,21,21),
      font:       GUIsmall,

      style:            0x02,
      item_style:       0x02,
      selected_style:   0x02,
      scroll_bar:       2,
      leading:          2,
      show_headings:    true,
   },

   ctrl: {
      id:         1302,
      type:       list,
      rect:       (445, 485, 185, 100),
      text:       "system list",
      back_color: (21,21,21),
      font:       GUIsmall,

      style:            0x02,
      item_style:       0x02,
      selected_style:   0x02,
      scroll_bar:       2,
      leading:          2,
      show_headings:    true,
   },

   ctrl: {
      id:         2000,
      type:       label,
      rect:       (664, 462, 128, 128),
      transparent: true
   }
}



