FORM


//  Project:   Starshatter 4.5
//  File:      AwardDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10,  50, 512,  50, 10)
      x_weights:  ( 0,   1,   0,   1,  0)

      y_mins:     (28, 25, 20,  5, 30, 256, 10, 50, 45)
      y_weights:  ( 0,  0,  0,  1,  0,   0,  0,  2,  0)
   },

   // background images:

/***
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
***/

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Congratulations",
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
   },

   // award name:

   ctrl: {
      id:            203,
      type:          label,
      cells:         (2,4,1,1)
      align:         center
      transparent:   true
      border:        false
      font:          Limerick18
      fore_color:    (255,255,255)
      back_color:    ( 10, 10, 10)
      style:         0x0040
   },

   // award insignia:

   ctrl: {
      id:            202,
      type:          image,
      cells:         (2,5,1,1)
      align:         center
      transparent:   true
      border:        false
   },

   // award description or info:

   ctrl: {
      id:            201,
      type:          label,
      cells:         (2,7,1,1)
      align:         left
      transparent:   true
      border:        false
      font:          Verdana
      fore_color:    (0,0,0)
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
      id:            1
      pid:           0
      type:          button
      text:          Close
      cells:         (3,8,1,1),
   },

}
