FORM


//  Project:   STARS
//  File:      PlanDlg.frm
//
//  John DiCamillo Software Consulting
//  Copyright © 1999. All Rights Reserved.


form: {
   rect:       (438, 212, 202, 268),
   screen_width:  640,
   screen_height: 480,

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "NavDetail_640.pcx",

   defctrl: {
      base_color: ( 92,  92,  92),
      back_color: ( 92,  92,  92),
      fore_color: (255, 255, 255),
      font:       GUIsmall,
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
   },

   ctrl: {
      id:         1,
      type:       label,
      rect:       (446, 220, 180, 18),
      text:       "Flight Plan",
      font:       GUI,
      transparent: true
   },


   ctrl: {
      id:         101,
      type:       label,
      rect:       (446, 239, 80, 18),
      text:       "Element:",
      transparent: true
   },

   ctrl: {
      id:         201,
      type:       combo,
      rect:       (530, 240, 100, 16),

      back_color:   (  0,   0,   0),
      border_color: ( 92,  92,  92),
      active_color: ( 64,  64,  64),

      font:         GUIsmall,
      simple:       true,
      bevel_width:  3,
      text_align:   left,
   },



   ctrl: {
      id:         102,
      type:       label,
      rect:       (446, 257, 80, 18),
      text:       "Nav Pt:",
      transparent: true
   },

   ctrl: {
      id:         202,
      type:       combo,
      rect:       (530, 258, 100, 16),

      back_color:   (  0,   0,   0),
      border_color: ( 92,  92,  92),
      active_color: ( 64,  64,  64),

      font:         GUIsmall,
      simple:       true,
      bevel_width:  3,
      text_align:   left,
   },


   ctrl: {
      id:         103,
      type:       label,
      rect:       (446, 275, 80, 18),
      text:       "Action:",
      transparent: true
   },

   ctrl: {
      id:         203,
      type:       combo,
      rect:       (530, 276, 100, 16),

      back_color:   (  0,   0,   0),
      border_color: ( 92,  92,  92),
      active_color: ( 64,  64,  64),

      font:         GUIsmall,
      simple:       true,
      bevel_width:  3,
      text_align:   left,
   },


   ctrl: {
      id:         104,
      type:       label,
      rect:       (446, 293, 80, 18),
      text:       "Formation:",
      transparent: true
   },

   ctrl: {
      id:         204,
      type:       combo,
      rect:       (530, 294, 100, 16),

      back_color:   (  0,   0,   0),
      border_color: ( 92,  92,  92),
      active_color: ( 64,  64,  64),

      font:         GUIsmall,
      simple:       true,
      bevel_width:  3,
      text_align:   left,

      item:         Diamond,
      item:         Spread,
      item:         Box,
      item:         Trail
   },



   ctrl: {
      id:         105,
      type:       label,
      rect:       (446, 311, 80, 18),
      text:       "Speed:",
      transparent: true
   },

   ctrl: {
      id:         205,
      type:       slider,
      rect:       (530, 312, 60, 16),

      active_color: ( 53, 159,  67),
      back_color:   (  0,   0,   0),
      border_color: ( 92,  92,  92),
      active:       true,

   },

   ctrl: {
      id:         305,
      type:       label,
      rect:       (590, 311, 40, 18),

      text:       "350",
      align:      right,
      transparent: true,
   },


   defctrl: {
      back_color: (130,105,80),
      font:       GUI,
   },

   ctrl: {
      id:         501,
      type:       button,
      rect:       (443, 400, 93, 25),
      text:       "Add Nav",
      sticky:     true
   },

   ctrl: {
      id:         502,
      type:       button,
      rect:       (540, 400, 93, 25),
      text:       "Del Nav",
      sticky:     true
   },

   ctrl: {
      id:         503,
      type:       button,
      rect:       (443, 427, 93, 25),
      text:       "Edit Nav",
      sticky:     true
   },

   ctrl: {
      id:         504,
      type:       button,
      rect:       (540, 427, 93, 25),
      text:       "clear",
      sticky:     false
   },
}



form: {
   rect:       (598, 264, 202, 336),
   screen_width:  800,
   screen_height: 600,

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "NavDetail_800.pcx",

   defctrl: {
      base_color: ( 92,  92,  92),
      back_color: ( 92,  92,  92),
      fore_color: (255, 255, 255),
      font:       GUIsmall,
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
   },

   ctrl: {
      id:         1,
      type:       label,
      rect:       (606, 272, 180, 18),
      text:       "Flight Plan",
      font:       GUI,
      transparent: true
   },


   ctrl: {
      id:         101,
      type:       label,
      rect:       (606, 291, 80, 18),
      text:       "Element:",
      transparent: true
   },

   ctrl: {
      id:         201,
      type:       combo,
      rect:       (690, 292, 100, 16),

      back_color:   (  0,   0,   0),
      border_color: ( 92,  92,  92),
      active_color: ( 64,  64,  64),

      font:         GUIsmall,
      simple:       true,
      bevel_width:  3,
      text_align:   left,
   },



   ctrl: {
      id:         102,
      type:       label,
      rect:       (606, 309, 80, 18),
      text:       "Nav Pt:",
      transparent: true
   },

   ctrl: {
      id:         202,
      type:       combo,
      rect:       (690, 310, 100, 16),

      back_color:   (  0,   0,   0),
      border_color: ( 92,  92,  92),
      active_color: ( 64,  64,  64),

      font:         GUIsmall,
      simple:       true,
      bevel_width:  3,
      text_align:   left,
   },


   ctrl: {
      id:         103,
      type:       label,
      rect:       (606, 327, 80, 18),
      text:       "Action:",
      transparent: true
   },

   ctrl: {
      id:         203,
      type:       combo,
      rect:       (690, 328, 100, 16),

      back_color:   (  0,   0,   0),
      border_color: ( 92,  92,  92),
      active_color: ( 64,  64,  64),

      font:         GUIsmall,
      simple:       true,
      bevel_width:  3,
      text_align:   left,
   },


   ctrl: {
      id:         104,
      type:       label,
      rect:       (606, 345, 80, 18),
      text:       "Formation:",
      transparent: true
   },

   ctrl: {
      id:         204,
      type:       combo,
      rect:       (690, 346, 100, 16),

      back_color:   (  0,   0,   0),
      border_color: ( 92,  92,  92),
      active_color: ( 64,  64,  64),

      font:         GUIsmall,
      simple:       true,
      bevel_width:  3,
      text_align:   left,

      item:         Diamond,
      item:         Spread,
      item:         Box,
      item:         Trail
   },



   ctrl: {
      id:         105,
      type:       label,
      rect:       (606, 363, 80, 18),
      text:       "Speed:",
      transparent: true
   },

   ctrl: {
      id:         205,
      type:       slider,
      rect:       (690, 364, 60, 16),

      active_color: ( 53, 159,  67),
      back_color:   (  0,   0,   0),
      border_color: ( 92,  92,  92),
      active:       true,

   },

   ctrl: {
      id:         305,
      type:       label,
      rect:       (750, 363, 40, 18),

      text:       "350",
      align:      right,
      transparent: true,
   },


   defctrl: {
      back_color: (130,105,80),
      font:       GUI,
   },

   ctrl: {
      id:         501,
      type:       button,
      rect:       (603, 452, 93, 25),
      text:       "Add Nav",
      sticky:     true
   },

   ctrl: {
      id:         502,
      type:       button,
      rect:       (700, 452, 93, 25),
      text:       "Del Nav",
      sticky:     true
   },

   ctrl: {
      id:         503,
      type:       button,
      rect:       (603, 479, 93, 25),
      text:       "Edit Nav",
      sticky:     true
   },

   ctrl: {
      id:         504,
      type:       button,
      rect:       (700, 479, 93, 25),
      text:       "clear",
      sticky:     false
   },
}



