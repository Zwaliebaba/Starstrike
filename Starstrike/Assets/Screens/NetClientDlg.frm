FORM


//  Project:   Starshatter 4.5
//  File:      NetClientDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10,  50, 100, 100, 100, 100, 100,  50, 10)
      x_weights:  ( 0,   1,   1,   1,   1,   1,   1,   1,  0)

      y_mins:     (28, 25, 20, 42,  5, 30, 60, 50, 45)
      y_weights:  ( 0,  0,  0,  0,  0,  0,  1,  0,  0)
   },

   // background images:

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame3a
      cells:         (1,3,4,6),
      cell_insets:   (0,0,0,10),
      margins:       (48,80,48,48)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame3b
      cells:         (5,3,3,6),
      cell_insets:   (0,0,0,10),
      margins:       (80,48,48,48)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Multiplayer Client",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,3,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   },


   // main panel:

   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,10,0,0)
      fixed_height:     19
   },

   ctrl: {
      id:            101
      type:          button
      cells:         (2,5,1,1)
      text:          "Add"
   },

   ctrl: {
      id:            102,
      type:          button,
      cells:         (3,5,1,1)
      text:          "Del",
   },

   ctrl: {
      id:            210,
      type:          label,
      cells:         (4,5,3,1)
      transparent:   true,
      font:          Verdana
   },

   ctrl: {
      id:            301,
      type:          button,
      cells:         (2,7,2,1)
      text:          "Local Server"
   },

   ctrl: {
      id:            302,
      type:          button,
      cells:         (5,7,1,1)
      cell_insets:   (10,0,0,0)
      text:          "Host"
   },

   ctrl: {
      id:            303,
      type:          button,
      cells:         (6,7,1,1)
      cell_insets:   (10,0,0,0)
      text:          "Join"
   },

   ctrl: {
      id:            200,
      type:          list,
      cells:         (2,6,5,1)
      cell_insets:   (0,0,0,10)

      fore_color:    (255,255,255)
      back_color:    ( 61, 61, 61)
      bevel_width:   0
      fixed_height:  0
      texture:       Panel
      margins:       (12,12,12,0)

      font:          Verdana,
      style:         0x02,
      scroll_bar:    2,
      show_headings: true,

      column:     {
         title:   "SERVER NAME",
         width:   177,
         align:   left,
         sort:    0 },

      column:     {
         title:   TYPE,
         width:   85,
         align:   center,
         sort:    0 },

      column:     {
         title:   PASSWORD,
         width:   85,
         align:   center,
         sort:    0 },

      column:     {
         title:   STATUS,
         width:   65,
         align:   center,
         sort:    0 },

      column:     {
         title:   PLAYERS,
         width:   65,
         align:   center,
         sort:    0 },

      column:     {
         title:   PING,
         width:   65,
         align:   center,
         sort:    0 },
   },


   // ok and cancel buttons:

   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (50,5,0,0)
      fixed_height:     19
   },

   ctrl: {
      id:            2
      pid:           0
      type:          button
      text:          Close
      cells:         (6,8,2,1),
   },

}
