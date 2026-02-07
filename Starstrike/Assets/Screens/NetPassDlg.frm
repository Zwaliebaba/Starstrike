FORM


//  Project:   Starshatter 4.5
//  File:      NetPassDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   rect:       (0,0,440,280),
   back_color: (0,0,0),
   fore_color: (255,255,255),
   font:       Limerick12,

   texture:    "Message.pcx",
   margins:    (50,40,48,40),

   layout: {
      x_mins:     (30, 100, 100, 100, 30),
      x_weights:  ( 0,   0,   1,   1,  0),

      y_mins:     (44,  30, 25, 25, 25, 25, 10, 35),
      y_weights:  ( 0,   1,  0,  0,  0,  0,  2,  0)
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
      text:          "Enter Server Password"
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
      id:            101,
      type:          label,
      cells:         (1,3,1,1)
      text:          "Server:",
   },

   ctrl: {
      id:            102,
      type:          label,
      cells:         (1,4,1,1)
      text:          "Password:",
   },

   ctrl: {
      id:            110,
      type:          label,
      cells:         (2,3,2,1)
      text:          ""
   },

   defctrl: {
      back_color:    (41, 41, 41)
      style:         0x02
      scroll_bar:    0
      transparent:   false
   },

   ctrl: {
      id:            200,
      type:          edit,
      cells:         (2,4,2,1)
      fixed_height:  18
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
      cells:         (2,7,1,1)
   }

   ctrl: {
      id:            2,
      type:          button,
      text:          "Cancel"
      cells:         (3,7,1,1),
   }
}
