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
      x_mins:     (20, 40, 100, 100, 20)
      x_weights:  ( 0,  0,   1,   1,  0)

      y_mins:     (44,  30, 30, 80, 35)
      y_weights:  ( 0,   0,  0,  1,  0)
   }

   ctrl: {
      id:               100,
      type:             label,
      text:             "Exit Starshatter?",
      transparent:      true,
      align:            center,
      font:             Limerick18,
      fore_color:       (255,255,255),
      cells:            (1,1,3,1)
   }

   ctrl: {
      id:               101,
      type:             label,
      text:             "Are you sure you want to exit Starshatter and return to Windows?",
      transparent:      true,
      align:            left,
      font:             Verdana,
      fore_color:       (255,255,255),
      cells:            (1,2,3,1)
   }

   ctrl: {
      id:               201,
      type:             text,
      text:             "",
      scroll_bar:       0,
      smooth_scroll:    true,
      transparent:      true,
      align:            left,
      font:             Verdana,
      fore_color:       (255,255,255),
      cells:            (1,3,3,1)
   }

   defctrl: {
      align:            left,
      font:             Limerick12,
      fore_color:       (0,0,0),
      standard_image:   Button17_0,
      activated_image:  Button17_1,
      transition_image: Button17_2,
      bevel_width:      6,
      margins:          (3,18,0,0),
      cell_insets:      (10,0,0,16)
   }

   ctrl: {
      id:            1,
      type:          button,
      text:          "Exit",
      cells:         (2,4,1,1)
   }

   ctrl: {
      id:            2,
      type:          button,
      text:          "Cancel",
      cells:         (3,4,1,1),
   }
}
