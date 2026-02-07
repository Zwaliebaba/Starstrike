FORM


//  Project:   Starshatter 4.5
//  File:      CmpCompleteDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2005. All Rights Reserved.


form: {
   back_color: (  0,   0,   0)
   fore_color: (255, 255, 255)
   font:       Limerick12,

   layout: {
      x_mins:     (0)
      x_weights:  (1)

      y_mins:     (0, 0, 0)
      y_weights:  (1, 6, 2)
   }

   // background images:

   defctrl: {
      fore_color:       (0,0,0)
      cell_insets:      (0,0,0,0)
   }

   ctrl: {
      id:            300
      type:          background
      texture:       LoadDlg1
      cells:         (0,0,1,1)
      margins:       (248,2,2,32)
      hide_partial:  false
   }

   ctrl: {
      id:            100
      type:          image
      cells:         (0,1,1,1)
      hide_partial:  false
   }

   ctrl: {
      id:            400
      type:          background
      texture:       LoadDlg2
      cells:         (0,2,1,1)
      margins:       (2,248,48,2)
      hide_partial:  false

      layout: {
         x_mins:     (20, 100, 100, 20)
         x_weights:  ( 0,   1,   0,  0)

         y_mins:     (20, 20, 30)
         y_weights:  ( 1,  0,  0)
      }
   }

   // close button:

   ctrl: {
      id:               1
      pid:              400
      type:             button
      text:             Close

      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      fixed_height:     19

      cells:            (2,1,1,1)
   }
}
