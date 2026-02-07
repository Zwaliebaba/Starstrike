FORM


//  Project:   Starshatter 4.5
//  File:      ExitDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   rect:       (0,0,440,320)
   back_color: (0,0,0)
   fore_color: (255,255,255)
   font:       Limerick12

   texture:    "Message.pcx"
   margins:    (50,40,48,40)

   layout: {
      x_mins:     (40, 40, 100, 100, 40)
      x_weights:  ( 0,  0,   1,   1,  0)

      y_mins:     (44, 30, 60, 35, 25)
      y_weights:  ( 0,  0,  0,  0,  0)
   }

   ctrl: {
      id:            100
      type:          label
      cells:         (1,1,3,1)
      text:          "Loading Mission"

      align:         center
      font:          Limerick18
      fore_color:    (255,255,255)
      transparent:   true
   },

   ctrl: {
      id:            101
      type:          label
      cells:         (1,3,3,1)
      text:          ""

      align:         center
      font:          Verdana
      fore_color:    (255,255,255)
      transparent:   true
   },

   ctrl: {
      id:            102,
      type:          slider,
      cells:         (1,4,3,1)
      fixed_height:  8

      active_color:  (250, 250, 100),
      back_color:    ( 21,  21,  21),
      border:        false,
      transparent:   false,
   },
}

