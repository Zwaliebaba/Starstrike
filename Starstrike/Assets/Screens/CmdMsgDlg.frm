FORM


//  Project:   Starshatter 4.5
//  File:      CmdMsgDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   rect:       (0,0,400,240)
   back_color: (0,0,0)
   fore_color: (255,255,255)
   font:       Limerick12

   texture:    "Message.pcx"
   margins:    (50,40,48,40)

   layout: {
      x_mins:     (20, 40, 100, 100, 20),
      x_weights:  ( 0,  0,   1,   1,  0),

      y_mins:     (44,  30, 10, 80, 35),
      y_weights:  ( 0,   0,  0,  1,  0)
   }

   defctrl: {
      fore_color:    (255, 255, 255)
      font:          Limerick12
      transparent:   true
   }

   ctrl: {
      id:            100,
      type:          label,
      text:          "Message Title",
      align:         center
      cells:         (1,1,3,1)
      font:          Limerick18
   },


   ctrl: {
      id:            101,
      type:          label,
      rect:          (180, 195, 280, 120),
      text:          "Message Text",
      align:         left
      font:          Verdana
      cells:         (1,3,3,1)
      cell_insets:   (0,0,0,20)
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
      cell_insets:      (10,0,0,16)
      transparent:      false
      fixed_height:     19
   }

   ctrl: {
      id:            1,
      type:          button,
      cells:         (3,4,1,1)
      text:          Close
   }
}

