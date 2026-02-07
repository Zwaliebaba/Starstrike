FORM


//  Project:   Starshatter 4.5
//  File:      CmpLoadDlg.frm
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
      y_weights:  (2, 1, 3)
   },

   // background images:

   defctrl: {
      fore_color:       (0,0,0)
      cell_insets:      (0,0,0,0)
   },

   ctrl: {
      id:            300
      type:          background
      texture:       LoadDlg1
      cells:         (0,0,1,1)
      margins:       (248,2,2,32)
      hide_partial:  false
   },

   ctrl: {
      id:            100
      type:          image
      cells:         (0,1,1,1)
      hide_partial:  false
   },

   ctrl: {
      id:            400
      type:          background
      texture:       LoadDlg2
      cells:         (0,2,1,1)
      margins:       (2,248,48,2)
      hide_partial:  false

      layout: {
         x_mins:     (30, 150, 30)
         x_weights:  ( 1,   1,  1)

         y_mins:     (20, 20, 20, 20)
         y_weights:  ( 1,  1,  1,  3)
      }
   },


   ctrl: {
      id:            101
      pid:           400
      type:          label,
      cells:         (1,1,1,1)
      text:          "",
      font:          Verdana
      align:         center
      transparent:   true
   },

   ctrl: {
      id:            102
      pid:           400
      type:          slider
      cells:         (1,2,1,1)

      active_color:  (255, 255, 160)
      back_color:    ( 21,  21,  21)
      border:        true
      transparent:   false
      fixed_height:  10
   },
}

