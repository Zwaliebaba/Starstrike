FORM


//  Project:   STARSHATTER
//  File:      CmdDlg.frm
//
//  John DiCamillo
//  Copyright © 1997-2001. All Rights Reserved.


form: {
   rect:       (0, 0, 640, 480),
   screen_width:  640,
   screen_height: 480,

   base_color: (160, 160, 150),
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "std_B_640.pcx",

   defctrl: {
      base_color: (160, 160, 150),
      back_color: ( 41,  41,  41),
      fore_color: (255, 255, 255),
      bevel_width:   0,
      bevel_depth:   128,
      border:        false,
      transparent:   true,
      align:         left,
   },

   ctrl: {
      id:            1000,
      type:          label,
      rect:          (7, 6, 600, 20),
      text:          "Operational Command",
   },

   ctrl: {
      id:            1001,
      type:          label,
      rect:          (20, 50, 600, 50),
      transparent:   false,
      back_color:    (69, 69, 67),
      border_color:  ( 4,  4,  4),
      text:          " ",
   },

   ctrl: {
      id:            300,
      type:          label,
      rect:          (25, 55, 300, 20),
      text:          "Operation Title Goes Here",
      fore_color:    (255, 255, 128)
   },

   ctrl: {
      id:            301,
      type:          label,
      rect:          (420, 55, 200, 20),
      text:          "Day 7 11:32:04",
      align:         right
   },

   ctrl: {
      id:            200,
      type:          label,
      rect:          (25,  75, 400,  20),
      text:          "PLAYER GROUP",
   },

   ctrl: {
      id:            201,
      type:          label,
      rect:          (440,  75, 180,  20),
      align:         right,
   },

   defctrl: {
      transparent:      false,
      align:            left,
      sticky:           true,
      font:             Terminal,
      fore_color:       (4, 4, 4),
      border:           false,
      simple:           false,

      standard_image:   btn_130x17_0,
      activated_image:  btn_130x17_1,
      transition_image: btn_130x17_2,
   },

   ctrl: {
      id:            100,
      type:          button,
      rect:          (20, 120, 130, 17),
      text:          "Orders"
   },

   ctrl: {
      id:            101,
      type:          button,
      rect:          (20, 145, 130, 17),
      text:          "Theater"
   },

   ctrl: {
      id:            102,
      type:          button,
      rect:          (20, 170, 130, 17),
      text:          "Forces"
   },

   ctrl: {
      id:            103,
      type:          button,
      rect:          (20, 195, 130, 17),
      text:          "Intel"
   },

   ctrl: {
      id:            104,
      type:          button,
      rect:          (20, 220, 130, 17),
      text:          "Missions"
   },

   defctrl: { sticky: false },

   ctrl: {
      id:            1,
      type:          button,
      rect:          (350, 445, 130, 17),
      text:          "Save",
      standard_image:   grn_130x17_0,
      activated_image:  grn_130x17_1,
      transition_image: grn_130x17_2,
   },

   ctrl: {
      id:            2,
      type:          button,
      rect:          (490, 445, 130, 17),
      text:          "Cancel",
      standard_image:   red_130x17_0,
      activated_image:  red_130x17_1,
      transition_image: red_130x17_2,
   }
}





form: {
   rect:       (0, 0, 800, 600),
   screen_width:  800,
   screen_height: 600,

   base_color: (160, 160, 150),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "cmd_800.pcx",

   defctrl: {
      base_color:    (160, 160, 150),
      back_color:    ( 92,  92,  92),
      fore_color:    (255, 255, 255),
      font:          GUI,
      bevel_width:   0,
      bevel_depth:   128,
      border:        true,
      border_color:  (  0,   0,   0),
      transparent:   true,
      align:         left,
      active:        true,
   },

   ctrl: {
      id:            300,
      type:          label,
      rect:          (25, 50, 300, 20),
      text:          "Operation Title Goes Here",
      fore_color:    (255, 255, 128)
   },

   ctrl: {
      id:            301,
      type:          label,
      rect:          (575, 50, 200, 20),
      text:          "Day 7 11:32:04",
      align:         right
   },

   ctrl: {
      id:            200,
      type:          label,
      rect:          (25,  75, 450,  25),
      text:          "PLAYER GROUP",
   },

   ctrl: {
      id:            201,
      type:          label,
      rect:          (575,  75, 200,  25),
      align:         right,
   },


   defctrl: {
      transparent:      false,
      align:            left,
      sticky:           true,
      font:             Terminal,
      fore_color:       (4, 4, 4),
      border:           false,
      simple:           false,

      standard_image:   btn_130x17_0,
      activated_image:  btn_130x17_1,
      transition_image: btn_130x17_2,
   },

   ctrl: {
      id:            100,
      type:          button,
      rect:          (25, 130, 130, 17),
      text:          "Orders"
   },

   ctrl: {
      id:            101,
      type:          button,
      rect:          (25, 155, 130, 17),
      text:          "Theater"
   },

   ctrl: {
      id:            102,
      type:          button,
      rect:          (25, 180, 130, 17),
      text:          "Forces"
   },

   ctrl: {
      id:            103,
      type:          button,
      rect:          (25, 205, 130, 17),
      text:          "Intel"
   },

   ctrl: {
      id:            104,
      type:          button,
      rect:          (25, 230, 130, 17),
      text:          "Missions"
   },


   defctrl: {
      back_color:       ( 92,  92,  92),
      fore_color:       (  4,   4,   4),
      bevel_width:      0,
      transparent:      false,
      sticky:           false,
   },

   ctrl: {
      id:               1,
      type:             button,
      rect:             (500, 558, 130, 17),
      text:             Save,
      standard_image:   grn_130x17_0,
      activated_image:  grn_130x17_1,
      transition_image: grn_130x17_2,
   },

   ctrl: {
      id:               2,
      type:             button,
      rect:             (640, 558, 130, 17),
      text:             Exit,
      standard_image:   red_130x17_0,
      activated_image:  red_130x17_1,
      transition_image: red_130x17_2,
   }
}






form: {
   rect:       (0, 0, 1024, 768),
   screen_width:  1024,
   screen_height:  768,

   base_color: (160, 160, 150),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "cmd_1024.pcx",

   defctrl: {
      base_color:    (160, 160, 150),
      back_color:    ( 92,  92,  92),
      fore_color:    (255, 255, 255),
      font:          GUI,
      bevel_width:   0,
      bevel_depth:   128,
      border:        true,
      border_color:  (  0,   0,   0),
      transparent:   true,
      align:         left,
      active:        true,
   },

   ctrl: {
      id:            300,
      type:          label,
      rect:          (25, 55, 300, 20),
      text:          "Operation Title Goes Here",
      fore_color:    (255, 255, 128)
   },

   ctrl: {
      id:            301,
      type:          label,
      rect:          (800, 55, 200, 20),
      text:          "Day 7 11:32:04",
      align:         right
   },

   ctrl: {
      id:            200,
      type:          label,
      rect:          (25,  80, 450,  25),
      text:          "PLAYER GROUP",
   },

   ctrl: {
      id:            201,
      type:          label,
      rect:          (800,  80, 200,  25),
      align:         right,
   },


   defctrl: {
      transparent:      false,
      align:            left,
      sticky:           true,
      font:             Terminal,
      fore_color:       (4, 4, 4),
      border:           false,
      simple:           false,

      standard_image:   btn_130x17_0,
      activated_image:  btn_130x17_1,
      transition_image: btn_130x17_2,
   },

   ctrl: {
      id:            100,
      type:          button,
      rect:          (25, 155, 130, 17),
      text:          "Orders"
   },

   ctrl: {
      id:            101,
      type:          button,
      rect:          (25, 180, 130, 17),
      text:          "Theater"
   },

   ctrl: {
      id:            102,
      type:          button,
      rect:          (25, 205, 130, 17),
      text:          "Forces"
   },

   ctrl: {
      id:            103,
      type:          button,
      rect:          (25, 230, 130, 17),
      text:          "Intel"
   },

   ctrl: {
      id:            104,
      type:          button,
      rect:          (25, 255, 130, 17),
      text:          "Missions"
   },


   defctrl: {
      back_color:       ( 92,  92,  92),
      fore_color:       (  4,   4,   4),
      bevel_width:      0,
      transparent:      false,
      sticky:           false,
   },

   ctrl: {
      id:         1,
      type:       button,
      rect:       (730, 730, 130, 17),
      text:       Save,
      sticky:     false,
      standard_image:   grn_130x17_0,
      activated_image:  grn_130x17_1,
      transition_image: grn_130x17_2,
   },

   ctrl: {
      id:         2,
      type:       button,
      rect:       (870, 730, 130, 17),
      text:       Exit,
      sticky:     false,
      standard_image:   red_130x17_0,
      activated_image:  red_130x17_1,
      transition_image: red_130x17_2,
   },
}

