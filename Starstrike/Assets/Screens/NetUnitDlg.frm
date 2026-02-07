FORM


//  Project:   Starshatter 4.5
//  File:      NetUnitDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 150,  60, 90, 100, 100, 10)
      x_weights:  ( 0,   1,   1,  1,   1,   1,  0)

      y_mins:     (28, 25, 20, 20, 20, 15, 25, 25, 20, 25, 50, 25, 45)
      y_weights:  ( 0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  2,  0,  0)
   },

   // background images:

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame3a
      cells:         (1,3,3,10),
      cell_insets:   (0,0,0,10),
      margins:       (48,80,48,48)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame3b
      cells:         (4,3,2,10),
      cell_insets:   (0,0,0,10),
      margins:       (80,48,48,48)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Multiplayer Unit Selection",
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
      font:          Limerick12
      fore_color:    (255,255,255)
      back_color:    ( 41, 41, 41)
      cell_insets:   (10,10,0,0)
      transparent:   true
      style:         0x40
   }

   ctrl: {
      id:            101
      type:          label
      cells:         (1,4,1,1)
      text:          "Players"
   }

   ctrl: {
      id:            102
      type:          label
      cells:         (3,4,3,1)
      text:          "Unit Mapping"
   }

   ctrl: {
      id:            103
      type:          label
      cells:         (1,9,3,1)
      text:          "Chat"
   }


   defctrl: {
      font:          Verdana
      transparent:   false
      style:         0x02
      cell_insets:   (10,10,0,5)
      texture:       Panel
      margins:       (12,12,12,0)

      fore_color:    (255,255,255),
      border_color:  (192, 192, 192)
      active_color:  ( 92,  92,  92)
      show_headings: true
      simple:        true
      text_align:    left
      fixed_height:  0
      scroll_bar:    2
   }

   ctrl: {
      id:            201,
      type:          list,
      cells:         (1,5,1,4)
      show_headings: true

      column:     {
         title:   PLAYER,
         width:   132,
         align:   left,
         sort:    3 },	// sort never
   },

   ctrl: {
      id:            202,
      type:          list,
      cells:         (3,5,3,4)
      show_headings: true,

      column:     {
         title:   "PLAYER",
         width:   120,
         align:   left,
         sort:    3 },	// sort never

      column:     {
         title:   "UNIT",
         width:   115,
         align:   left,
         sort:    3 },	// sort never

      column:     {
         title:   "SHIP TYPE",
         width:   115,
         align:   left,
         sort:    3 },	// sort never

      column:     {
         title:   "ROLE",
         width:   120,
         align:   left,
         sort:    3 },	// sort never

      column:     {
         title:   "TEAM",
         width:   51,
         align:   left,
         sort:    3 },	// sort never
   },

   ctrl: {
      id:            211,
      type:          list,
      cells:         (1,10,5,1)
      show_headings: true,

      column:     {
         title:   NAME,
         width:   130,
         align:   left,
         sort:    0 },

      column:     {
         title:   "CHAT MESSAGE",
         width:   629,
         align:   left,
         sort:    0 },
   },

   ctrl: {
      id:            212,
      type:          edit,
      cells:         (1,11,5,1)
      scroll_bar:    0,
      transparent:   false,
   },

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
      cell_insets:      (5,5,0,0)
      fixed_height:     19
      fixed_width:      0
      cell_insets:      (5,5,0,0)
   },

   ctrl: {
      id:            206,
      type:          button,
      cells:         (2,5,1,1)
      text:          "SELECT UNIT"
      cell_insets:   (5,5,20,0)
   },

   ctrl: {
      id:            203,
      type:          button,
      cells:         (2,6,1,1)
      text:          "MAP >>",
   },

   ctrl: {
      id:            204,
      type:          button,
      cells:         (2,7,1,1)
      text:          "MAP <<",
   },

   ctrl: {
      id:            205,
      type:          button,
      cells:         (2,8,1,1)
      cell_insets:   (5,5,20,0)
      text:          "*  BAN  *",
   },

   defctrl: {
      font:          Verdana,
      fore_color:    (255, 255, 255),
      back_color:    ( 61,  61,  59),
      transparent:   true,
      style:         0x02,
      scroll_bar:    2,
      standard_image:   "",
      activated_image:  "",
      transition_image: "",
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
      cell_insets:      (5,5,0,0)
      fixed_height:     19
      fixed_width:      0
      pid:              0
   },

   ctrl: {
      id:            1
      type:          button
      text:          Accept
      cells:         (4,12,1,1)
   },

   ctrl: {
      id:            2
      type:          button
      text:          Cancel
      cells:         (5,12,1,1)
   },

}


