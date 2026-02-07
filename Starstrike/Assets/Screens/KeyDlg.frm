FORM


//  Project:   Starshatter 4.5
//  File:      KeyDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   rect:       (0,0,440,380),
   back_color: (0,0,0),
   fore_color: (255,255,255),
   font:       Limerick12,

   texture:    "Message.pcx",
   margins:    (50,40,48,40),

   layout: {
      x_mins:     (30, 100, 100, 100, 30),
      x_weights:  ( 0,   0,   1,   1,  0),

      y_mins:     (44,  30, 25, 25, 25, 25, 25, 40, 55, 10, 35),
      y_weights:  ( 0, 0.5,  0,  0,  0,  0,  0,  0,  0,  1,  0)
   },

   defctrl: {
      base_color:    ( 92,  92,  92)
      back_color:    ( 41,  41,  41)
      fore_color:    (255, 255, 255)
      bevel_width:   0
      bevel_depth:   128
      border:        true
      border_color:  (192, 192, 192)
      align:         left
      font:          Verdana
      transparent:   true
      style:         0x02
   }

   ctrl: {
      id:            100
      type:          label
      text:          "Key Binding"
      cells:         (1,1,3,1)
      font:          Limerick12
      align:         center
   }

   ctrl: {
      id:            101
      type:          label
      text:          "Command:"
      cells:         (1,2,1,1)
   }

   ctrl: {
      id:            102
      type:          label
      text:          "Current Key:"
      cells:         (1,3,1,1)
   }

   ctrl: {
      id:            103
      type:          label
      text:          "New Key:"
      cells:         (1,4,1,1)
   }

   ctrl: {
      id:            11
      type:          label
      cells:         (1,8,3,1)
      text:          "Press any key to select a new binding for this command.  "
                     "Then click Apply to save the new binding, or Cancel to "
                     "return to the original binding."
   }

   defctrl: { 
      cell_insets:   (0,0,0,5)
      transparent:   false
   }

   ctrl: {
      id:            201,
      type:          label,
      cells:         (2,2,2,1)
      text:          "none selected",
   },

   ctrl: {
      id:            202,
      type:          label,
      cells:         (2,3,2,1)
      text:          "none selected",
   },


   ctrl: {
      id:            203,
      type:          label,
      cells:         (2,4,2,1)
      text:          "",
   },


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
      id:            300,
      type:          button,
      cells:         (2,6,2,1)
      text:          "Clear Binding",
      cell_insets:   (0,0,0,6)
   },

   ctrl: {
      id:            1,
      type:          button,
      text:          "Apply",
      cells:         (2,10,1,1)
   }

   ctrl: {
      id:            2,
      type:          button,
      text:          "Cancel",
      cells:         (3,10,1,1),
   }
}
