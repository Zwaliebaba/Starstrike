FORM


//  Project:   Starshatter 4.5
//  File:      MsnSelectDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 80, 80, 10, 100, 100, 100, 10),
      x_weights:  ( 0,  1,  1,  0,   3,   3,   3,  0),

      y_mins:     (28, 25, 20, 52, 60, 45),
      y_weights:  ( 0,  0,  0,  0,  1,  0)
   },

   // background images:

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame3a
      cells:         (1,3,4,3),
      cell_insets:   (0,0,0,10),
      margins:       (48,80,48,48)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame3b
      cells:         (5,3,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (80,48,48,48)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Single Missions",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,4,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   },


   // main panel:

   ctrl: {
      id:            300
      type:          panel
      transparent:   true

      cells:         (1,3,6,3)
      cell_insets:   (10,10,14,54)
      hide_partial:  false

      layout: {
         x_mins:     (50, 50, 50, 20, 50, 50, 50)
         x_weights:  ( 1,  1,  1,  0,  1,  1,  1)

         y_mins:     (25, 100,  5, 25, 100, 10, 25)
         y_weights:  ( 0,   1,  0,  0,   2,  0,  0)
      }
   }


   defctrl: {
      fore_color:       (255,255,255)
      back_color:       (0,0,0)
      font:             Limerick12
      bevel_width:      0

      align:            left
      transparent:      true
   },

   ctrl: {
      id:            101
      pid:           300
      type:          label
      cells:         (0,0,3,1)
      text:          "Campaign"
      style:         0x0040,
   },

   ctrl: {
      id:            102,
      pid:           300
      type:          label
      cells:         (0,3,3,1)
      text:          "Mission",
      style:         0x0040,
   },

   ctrl: {
      id:            102
      pid:           300
      type:          label
      cells:         (4,3,3,1)
      text:          "Description"
      style:         0x0040
   },

   defctrl: {
      active_color:     (192, 192, 192)
      fore_color:       (255, 255, 255)
      back_color:       ( 61,  61,  61)
      bevel_width:      0
      texture:          Panel
      margins:          (12,12,12,0)

      align:            left

      font:             Verdana
      simple:           true
      text_align:       left
      transparent:      false
      style:            0x02
      scroll_bar:       2
   },

   ctrl: {
      id:            203
      pid:           300
      type:          list,
      cells:         (0,1,3,1)

      column:     {
         title:   Campaigns,
         width:   202,
         align:   left,
         sort:    0 }
   },

   ctrl: {
      id:            202
      pid:           300
      type:          list
      cells:         (0,4,3,1)

      column:     {
         title:   Missions,
         width:   202,
         align:   left,
         sort:    0 }
   },

   ctrl: {
      id:            200
      pid:           300
      type:          text
      cells:         (4,4,3,1)
   },


   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      fixed_height:     19
      cell_insets:      (5,5,0,0)
      margins:          (3,18,0,0)
   },

   ctrl: {
      id:            301
      pid:           300
      type:          button
      cells:         (0,6,1,1)
      text:          "New"
   },

   ctrl: {
      id:            302
      pid:           300
      type:          button
      cells:         (1,6,1,1)
      text:          "Edit"
   },

   ctrl: {
      id:            303
      pid:           300
      type:          button
      cells:         (2,6,1,1)
      text:          "Delete"
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
      cell_insets:      (0,10,0,0)
      fixed_height:     19
   },

   ctrl: {
      id:            1
      type:          button
      text:          Accept
      cells:         (5,5,1,1)
   },

   ctrl: {
      id:            2
      type:          button
      text:          Cancel
      cells:         (6,5,1,1),
   },
}

