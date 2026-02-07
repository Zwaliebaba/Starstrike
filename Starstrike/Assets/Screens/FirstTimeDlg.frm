FORM


//  Project:   Starshatter 4.5
//  File:      FirstTimeDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   rect:       (0,0,440,430)
   back_color: (0,0,0)
   fore_color: (255,255,255)
   font:       Limerick12

   texture:    "Message.pcx"
   margins:    (50,40,48,40)

   layout: {
      x_mins:     (20, 90, 100, 100, 20)
      x_weights:  ( 0,  0,   1,   1,  0)

      y_mins:     (44, 25,  5, 45, 45, 45, 45, 45, 45, 35)
      y_weights:  ( 0,  0,  0,  0,  1,  0,  1,  0,  1,  0)
   }

   defctrl: {
      base_color:   ( 92,  92,  92),
      back_color:   ( 21,  21,  21),
      fore_color:   (255, 255, 255),
      font:         Verdana,
      align:        left,
      transparent:  true,
      bevel_width:  0,
      bevel_depth:  128,
      border:       true,
      border_color: (192, 192, 192),
      style:        0,
   },

   ctrl: {
      id:            110
      type:          label
      cells:         (1,1,3,1)
      font:          Limerick18
      text:          "NEW PLAYER"
      align:         center
   },

   ctrl: {
      id:            100
      type:          label
      cells:         (1,3,3,1)
      text:          "Create a new player account. Enter your name in the box provided. "
                     "The user name may be a nickname, callsign, or last name.",
   },

   ctrl: {
      id:            101,
      type:          label,
      cells:         (1,4,1,1)
      text:          "Player Name:",
   },

   ctrl: {
      id:            102,
      type:          label,
      cells:         (1,5,3,1)
      text:          "Select your preferred style of play. Arcade mode is similar to games "
                     "such as Wing Commander or FreeSpace. Standard mode is more like "
                     "Babylon 5, Independence War, or Falcon 4.0",
   },

   ctrl: {
      id:            103
      type:          label
      cells:         (1,6,1,1)
      text:          "Play Style:"
   },

   ctrl: {
      id:            104
      type:          label
      cells:         (1,7,3,1)
      align:         left
      transparent:   true
      text:          "The following option allows you to skip the training campaign, "
                     "'Operation Live Fire'.  If this is your first time playing Starshatter "
                     "select 'Cadet (First timer)'",
   },

   ctrl: {
      id:            105
      type:          label
      cells:         (1,8,1,1)
      text:          "Experience:",
   },

   defctrl: {
      style:         0x02
      scroll_bar:    0

      active_color:  ( 92,  92,  92)
      back_color:    ( 41,  41,  41)
      base_color:    ( 92,  92,  92)
      border_color:  (192, 192, 192)

      border:        true
      simple:        true
      bevel_width:   3
      text_align:    left
      transparent:   false

      fixed_height:  18
   }

   ctrl: {
      id:            200
      type:          edit
      cells:         (2,4,2,1)
      single_line:   true
   }

   ctrl: {
      id:            201
      type:          combo
      cells:         (2,6,2,1)

      item:          "Arcade Style"
      item:          "Standard Model"
   }

   ctrl: {
      id:            202
      type:          combo
      cells:         (2,8,2,1)

      item:          "Cadet (First timer)"
      item:          "Admiral (Experienced)"
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
      id:               1
      type:             button
      cells:            (3,9,1,1)
      text:             Accept
   }
}

