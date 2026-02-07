FORM


//  Project:   Starshatter 4.5
//  File:      DebriefDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 100,  20, 100, 100, 10),
      x_weights:  ( 0, 0.2, 0.4, 0.2, 0.2,  0),

      y_mins:     (28, 30,  10,  90, 24, 60, 45),
      y_weights:  ( 0,  0,   0,   0,  0,  1,  0)
   },

   // background images:

   ctrl: {
      id:            9990
      type:          background
      texture:       Frame4a
      cells:         (1,3,4,1),
      cell_insets:   (0,0,0,0),
      margins:       (2,2,16,16)
      hide_partial:  false
   }

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame2a,
      cells:         (1,4,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (2,32,40,32)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame2b,
      cells:         (3,4,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (0,40,40,32)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Mission Debriefing",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,3,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   },


   // info panel:

   ctrl: {
      id:               700
      type:             panel
      transparent:      false

      texture:          Panel
      margins:          (12,12,12,0),

      cells:            (1,3,4,1),
      cell_insets:      (10,10,12,10)

      layout: {
         x_mins:     ( 20, 60, 100, 60, 100, 20)
         x_weights:  (  0,  0,   1,  0,   1,  0)

         y_mins:     (  0,  20,  15,  15,  0)
         y_weights:  (  1,   0,   0,   0,  1)
      }
   }

   defctrl: {
      align:            left
      bevel_width:      0
      font:             Verdana
      fore_color:       (255, 255, 255)
      standard_image:   ""
      activated_image:  ""
      transparent:      true
      margins:          (0,0,0,0)
      cell_insets:      (0,0,0,0)
      text_insets:      (1,1,1,1)
   },

   ctrl: {
      id:            200
      pid:           700
      type:          label
      cells:         (1,1,2,1)
      text:          "title goes here",
      fore_color:    (255, 255, 128)
      font:          Limerick12
   },

   ctrl: {
      id:            201
      pid:           700
      type:          label
      cells:         (1,2,1,1)
      text:          "System:"
   },

   ctrl: {
      id:            202
      pid:           700
      type:          label
      cells:         (2,2,1,1)
      text:          "alpha"
   },

   ctrl: {
      id:            203
      pid:           700
      type:          label
      cells:         (1,3,1,1)
      text:          "Sector:"
   },

   ctrl: {
      id:            204
      pid:           700
      type:          label
      cells:         (2,3,1,1)
      text:          "bravo"
   },


   ctrl: {
      id:            206
      pid:           700
      type:          label
      cells:         (4,1,1,1)
      text:          "Day 7 11:32:04"
      align:         right
   },


   // main panel:

   ctrl: {
      id:               800
      type:             panel
      transparent:      false

      texture:          Panel
      margins:          (12,12,12,0),

      cells:            (1,5,4,2)
      cell_insets:      (10,10,12,54)

      layout: {
         x_mins:     ( 20, 100,  10, 100,  20)
         x_weights:  (  0,   4,   0,   3,   0)

         y_mins:     ( 10,  20,  40,  20,  60,  20,  60,  20)
         y_weights:  (  0,   0,   0,   0,   1,   0,   1,   0)
      }
   }

   defctrl: {
      pid:           800
   }

   ctrl: {
      id:            110,
      type:          label
      cells:         (1,1,3,1)
      text:          Objectives
      font:          Limerick12
   },

   ctrl: {
      id:            210
      type:          label
      cells:         (1,2,3,1)
      text:          " "
      font:          Verdana
   },

   ctrl: {
      id:            111
      type:          label
      cells:         (3,1,1,1)
      text:          "Mission Score"
      font:          Limerick12
   }

   ctrl: {
      id:            211
      type:          label
      cells:         (3,1,1,1)
      align:         right
      fore_color:    (255, 255, 128)
      font:          Limerick12
   }


   ctrl: {
      id:            120
      type:          label
      cells:         (1,3,1,1)
      text:          "Active Units"
      font:          Limerick12
   },

   ctrl: {
      id:            320
      type:          list
      cells:         (1,4,1,1)
      cell_insets:   (0,0,0,10)
      font:          "Verdana"
      back_color:    (41,41,41)
      transparent:   false,

      style:            0x02,
      item_style:       0x00,
      selected_style:   0x02,
      scroll_bar:       2,
      leading:          2,
      show_headings:    true,

      column:     {
         title:   PKG,
         width:   50,
         align:   center,
         sort:    3 },

      column:     {
         title:   CALLSIGN,
         width:   90,
         align:   left,
         sort:    3 },

      column:     {
         title:   ROLE,
         width:   82,
         align:   left,
         sort:    3 },

      column:     {
         title:   TYPE,
         width:   60,
         align:   left,
         sort:    3 },
   },

   ctrl: {
      id:            130
      type:          label
      cells:         (1,5,1,1)
      text:          "Mission Summary"
      font:          Limerick12
   },

   ctrl: {
      id:            330
      type:          list
      cells:         (1,6,1,1)
      font:          "Verdana"
      back_color:    (41,41,41)
      transparent:   false

      style:            0x02,
      item_style:       0x00,
      selected_style:   0x00,
      scroll_bar:       2,
      leading:          2,
      show_headings:    true,

      column:     {
         title:   STAT,
         width:   141,
         align:   right,
         sort:    3 },

      column:     {
         title:   VALUE,
         width:   141,
         align:   left,
         sort:    3 },

   },


   ctrl: {
      id:            130
      type:          label
      cells:         (3,3,1,1)
      text:          "Mission Log"
      font:          Limerick12
   },

   ctrl: {
      id:            340
      type:          list
      cells:         (3,4,1,3)
      font:          "Verdana"
      back_color:    (41,41,41)
      transparent:   false

      style:            0x02,
      item_style:       0x00,
      selected_style:   0x00,
      scroll_bar:       2,
      leading:          2,
      show_headings:    true,

      column:     {
         title:   TIME,
         width:   50,
         align:   center,
         sort:    3 },

      column:     {
         title:   EVENT,
         width:   132,
         align:   left,
         sort:    3 },

      column:     {
         title:   TARGET,
         width:   80,
         align:   left,
         sort:    3 },
   }


   // ok and cancel buttons:

   defctrl: {
      pid:              0
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,10,0,26)
   },

   ctrl: {
      id:            1
      type:          button
      text:          Close
      cells:         (4,6,1,1),
   },

}
