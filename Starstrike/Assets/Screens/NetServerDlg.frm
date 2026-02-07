FORM


//  Project:   Starshatter 4.5
//  File:      NetAddrDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   rect:       (0,0,440,380),
   back_color: (0,0,0),
   fore_color: (255,255,255),
   font:       Limerick12,

   texture:    "Message.pcx",
   margins:    (50,40,48,40),

   layout: {
      x_mins:     (30, 150, 100, 100, 30),
      x_weights:  ( 0,   0,   1,   1,  0),

      y_mins:     (44,  30, 25, 25, 25, 25, 25, 25, 25, 25, 10, 35),
      y_weights:  ( 0,   1,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0)
   },

   defctrl: {
      base_color:    ( 92,  92,  92)
      back_color:    ( 41,  41,  41)
      fore_color:    (255, 255, 255)
      bevel_width:   0
      bevel_depth:   128
      border:        true
      border_color:  (192, 192, 192)
      align:         left
      font:          Verdana
      transparent:   true
      style:         0x02
   }

   ctrl: {
      id:            100
      type:          label
      text:          "Server Configuration"
      cells:         (1,1,3,1)
      font:          Limerick12
      align:         center
   }

   defctrl: {
      font:          Verdana
      transparent:   true
      style:         0
   },


   ctrl: {
      id:            110,
      type:          label,
      cells:         (1,2,1,1)
      text:          "Name: ",
   },

   ctrl: {
      id:            111,
      type:          label,
      cells:         (1,3,1,1)
      text:          "Type: ",
   },

   ctrl: {
      id:            112,
      type:          label,
      cells:         (1,4,1,1)
      text:          "Game Port: ",
   },

   ctrl: {
      id:            113,
      type:          label,
      cells:         (1,5,1,1)
      text:          "Admin Port: ",
   },

   ctrl: {
      id:            114,
      type:          label,
      cells:         (1,7,1,1)
      text:          "Game Password: ",
   },

   ctrl: {
      id:            115,
      type:          label,
      cells:         (1,8,1,1)
      text:          "Admin Name: ",
   },

   ctrl: {
      id:            116,
      type:          label,
      cells:         (1,9,1,1)
      text:          "Admin Password: ",
   },

   defctrl: {
      back_color:    (41, 41, 41),
      style:         0x02,
      scroll_bar:    0,

      active_color:  ( 92,  92,  92),
      back_color:    ( 41,  41,  41),
      base_color:    ( 92,  92,  92),
      border_color:  (192, 192, 192),

      border:        true
      simple:        true
      transparent:   false
      bevel_width:   3
      text_align:    left
      fixed_height:  18
   },

   ctrl: {
      id:            200,
      type:          edit,
      cells:         (2,2,2,1)
   },

   ctrl: {
      id:            201,
      type:          combo,
      cells:         (2,3,2,1)

      item:          "LAN",
      item:          "Private",
      item:          "Public",
   },

   ctrl: {
      id:            202,
      type:          edit,
      cells:         (2,4,2,1)
   },

   ctrl: {
      id:            203,
      type:          edit,
      cells:         (2,5,2,1)
   },

   ctrl: {
      id:            204,
      type:          edit,
      cells:         (2,7,2,1)
   },

   ctrl: {
      id:            205,
      type:          edit,
      cells:         (2,8,2,1)
   },

   ctrl: {
      id:            206,
      type:          edit,
      cells:         (2,9,2,1)
   },


   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (10,0,0,0)
      fixed_height:     19
   }

   ctrl: {
      id:            1,
      type:          button,
      text:          "Accept"
      cells:         (2,11,1,1)
   }

   ctrl: {
      id:            2,
      type:          button,
      text:          "Cancel"
      cells:         (3,11,1,1),
   }
}
