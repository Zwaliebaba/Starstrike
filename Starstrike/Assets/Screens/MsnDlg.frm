FORM


//  Project:   STARS
//  File:      MsnDlg.frm
//
//  John DiCamillo Software Consulting
//  Copyright © 2001. All Rights Reserved.


form: {
   rect:       (0, 0, 640, 480),
   screen_width:  640,
   screen_height: 480,

   base_color: ( 72,  82,  92),
   back_color: ( 72,  82,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "msn_brief_640.pcx",

   defctrl: {
      base_color: (160, 160, 150),
      back_color: ( 62, 106, 151),
      fore_color: (255, 255, 255),
      font:       GUI,
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0),
      transparent:  true,
      align:        left
   },

   ctrl: {
      id:            100,
      type:          label,
      rect:          (7, 6, 600, 20),
      text:          "Mission Briefing",
      transparent:   true,
      align:         left,
      font:          GUI,
      fore_color:    (255,255,255),
   },

   ctrl: {
      id:            200,
      type:          label,
      rect:          (20, 55, 300, 20),
      text:          "title goes here",
      fore_color:    (255, 255, 128)
   },

   ctrl: {
      id:            201,
      type:          label,
      rect:          (20, 87,  65, 50),
      text:          "System:",
      font:          "Verdana"
   },

   ctrl: {
      id:            202,
      type:          label,
      rect:          (80, 87, 125, 50),
      text:          "alpha",
      font:          "Verdana"
   },

   ctrl: {
      id:            203,
      type:          label,
      rect:          (20, 77,  65, 50),
      text:          "Sector:",
      font:          "Verdana"
   },

   ctrl: {
      id:            204,
      type:          label,
      rect:          (80, 77, 125, 50),
      text:          "bravo",
      font:          "Verdana"
   },


   ctrl: {
      id:            206,
      type:          label,
      rect:          (420, 55, 200, 20),
      text:          "Day 7 11:32:04",
      align:         right
   },


   defctrl: {
      transparent:  false,
      align:        center,
      font:         Terminal,
      fore_color:   (4,4,4),
      bevel_width:  0,
      align:        left,
      sticky:       true,
      standard_image:   tab_70x17_0,
      activated_image:  tab_70x17_1,
      transition_image: tab_70x17_2,
   },

   ctrl: {
      id:            900,
      type:          button,
      rect:          (20, 450, 70, 17),
      sticky:        true,
      text:          "SIT"
   },

   ctrl: {
      id:            901,
      type:          button,
      rect:          (95, 450, 70, 17),
      sticky:        true,
      text:          "PKG"
   },

   ctrl: {
      id:            902,
      type:          button,
      rect:          (170, 450, 70, 17),
      sticky:        true,
      text:          "NAV"
   },

   ctrl: {
      id:            903,
      type:          button,
      rect:          (245, 450, 70, 17),
      sticky:        true,
      text:          "WEP",
   },




   ctrl: {
      id:            1,
      type:          button,
      rect:          (350, 450, 130, 17),
      text:          Accept,
      sticky:           false,
      standard_image:   grn_130x17_0,
      activated_image:  grn_130x17_1,
      transition_image: grn_130x17_2,
   },

   ctrl: {
      id:            2,
      type:          button,
      rect:          (490, 450, 130, 17),
      text:          Cancel,
      sticky:           false,
      standard_image:   red_130x17_0,
      activated_image:  red_130x17_1,
      transition_image: red_130x17_2,
   }
}





form: {
   rect:       (0, 0, 800, 600),
   screen_width:  800,
   screen_height: 600,

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "msn_brief_800.pcx",

   defctrl: {
      base_color:    ( 92,  92,  92),
      back_color:    ( 92,  92,  92),
      fore_color:    (255, 255, 255),
      font:          Verdana,
      bevel_width:   0,
      bevel_depth:   128,
      border:        true,
      border_color:  (  0,   0,   0),
      transparent:   true,
      align:         left,
      active:        true,
   },

   ctrl: {
      id:            200,
      type:          label,
      rect:          (25, 55, 400, 20),
      text:          "title goes here",
      fore_color:    (255, 255, 128),
      font:          GUI,
   },

   ctrl: {
      id:            201,
      type:          label,
      rect:          (25, 70,  65, 50),
      text:          "System:",
   },

   ctrl: {
      id:            202,
      type:          label,
      rect:          (100, 70, 225, 50),
      text:          "alpha",
   },

   ctrl: {
      id:            203,
      type:          label,
      rect:          (25, 82,  65, 50),
      text:          "Sector:",
   },

   ctrl: {
      id:            204,
      type:          label,
      rect:          (100, 82, 225, 50),
      text:          "bravo",
   },


   ctrl: {
      id:            206,
      type:          label,
      rect:          (575, 55, 200, 20),
      text:          " ",
      align:         right
   },

   defctrl: {
      font:             Terminal,
      simple:           false,
      border:           false,
      base_color:       (160, 160, 150),
      back_color:       ( 92,  92,  92),
      fore_color:       (  4,   4,   4),
      bevel_width:      0,
      transparent:      false,
      sticky:           true,

      standard_image:   tab_90x17_0,
      activated_image:  tab_90x17_1,
      transition_image: tab_90x17_2,
   },

   ctrl: {
      id:            900,
      type:          button,
      rect:          (20, 550, 90, 17),
      text:          " Sit",
   },

   ctrl: {
      id:            901,
      type:          button,
      rect:          (115, 550, 90, 17),
      text:          " Pkg",
   },

   ctrl: {
      id:            902,
      type:          button,
      rect:          (210, 550, 90, 17),
      text:          " Map",
   },

   ctrl: {
      id:            903,
      type:          button,
      rect:          (305, 550, 90, 17),
      text:          " Wep",
   },

   ctrl: {
      id:               1,
      type:             button,
      rect:             (500, 558, 130, 17),
      text:             Accept,
      sticky:           false,
      standard_image:   grn_130x17_0,
      activated_image:  grn_130x17_1,
      transition_image: grn_130x17_2,
   },

   ctrl: {
      id:               2,
      type:             button,
      rect:             (640, 558, 130, 17),
      text:             Cancel,
      sticky:           false,
      standard_image:   red_130x17_0,
      activated_image:  red_130x17_1,
      transition_image: red_130x17_2,
   }
}






form: {
   rect:       (0, 0, 1024, 768),
   screen_width:  1024,
   screen_height:  768,

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "msn_brief_1024.pcx",

   defctrl: {
      base_color:    (180, 180, 172),
      back_color:    ( 92,  92,  92),
      fore_color:    (255, 255, 255),
      font:          Verdana,
      bevel_width:   0,
      bevel_depth:   128,
      border:        true,
      border_color:  (  0,   0,   0),
      transparent:   true,
      align:         left,
      active:        true,
   },

   ctrl: {
      id:            200,
      type:          label,
      rect:          (25, 55, 500, 20),
      text:          "title goes here",
      fore_color:    (255, 255, 128),
      font:          GUI,
   },

   ctrl: {
      id:            201,
      type:          label,
      rect:          (25, 70,  65, 50),
      text:          "System:",
   },

   ctrl: {
      id:            202,
      type:          label,
      rect:          (100, 70, 325, 50),
      text:          "alpha",
   },

   ctrl: {
      id:            203,
      type:          label,
      rect:          (25, 82,  65, 50),
      text:          "Sector:",
   },

   ctrl: {
      id:            204,
      type:          label,
      rect:          (100, 82, 325, 50),
      text:          "bravo",
   },


   ctrl: {
      id:            206,
      type:          label,
      rect:          (575, 55, 200, 20),
      text:          " ",
      align:         right
   },

   defctrl: {
      font:             Terminal,
      simple:           false,
      border:           false,
      base_color:       (160, 160, 150),
      back_color:       ( 92,  92,  92),
      fore_color:       (  4,   4,   4),
      bevel_width:      0,
      transparent:      false,
      sticky:           true,

      standard_image:   tab_90x17_0,
      activated_image:  tab_90x17_1,
      transition_image: tab_90x17_2,
   },

   ctrl: {
      id:            900,
      type:          button,
      rect:          (20, 730, 90, 17),
      text:          " SIT",
   },

   ctrl: {
      id:            901,
      type:          button,
      rect:          (115, 730, 90, 17),
      text:          " PKG",
   },

   ctrl: {
      id:            902,
      type:          button,
      rect:          (210, 730, 90, 17),
      text:          " MAP",
   },

   ctrl: {
      id:            903,
      type:          button,
      rect:          (305, 730, 90, 17),
      text:          " WEP",
   },

   ctrl: {
      id:         1,
      type:       button,
      rect:       (730, 730, 130, 17),
      text:       Accept,
      sticky:     false,
      standard_image:   grn_130x17_0,
      activated_image:  grn_130x17_1,
      transition_image: grn_130x17_2,
   },

   ctrl: {
      id:         2,
      type:       button,
      rect:       (870, 730, 130, 17),
      text:       Cancel,
      sticky:     false,
      standard_image:   red_130x17_0,
      activated_image:  red_130x17_1,
      transition_image: red_130x17_2,
   },
}

