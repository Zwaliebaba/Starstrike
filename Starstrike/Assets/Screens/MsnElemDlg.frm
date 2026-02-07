FORM


//  Project:   Starshatter 4.5
//  File:      MsnElemDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   rect:       (0,0,600,450)
   back_color: (0,0,0)
   fore_color: (255,255,255)
   font:       Limerick12

   texture:    "Message.pcx"
   margins:    (50,40,48,40)

   layout: {
      x_mins:     (20, 100, 100, 100, 20)
      x_weights:  ( 0,   1,   1,   1,  0)

      y_mins:     (44,  30, 100, 10, 35)
      y_weights:  ( 0,   0,   1,  0,  0)
   }

   defctrl: {
      base_color:    ( 92,  92,  92)
      back_color:    ( 41,  41,  41)
      fore_color:    (255, 255, 255)
      bevel_width:   0
      border:        true
      border_color:  (192, 192, 192)
      align:         left
      font:          Verdana
      transparent:   true
      style:         0x02
   }

   ctrl: {
      id:            10
      type:          label
      text:          "Mission Element"
      cells:         (1,1,3,1)
      font:          Limerick12
      align:         center
   }

   ctrl: {
      id:            100
      type:          panel
      cells:         (1,2,3,1)
      transparent:   true

      layout: {
         x_mins:     (80, 100, 20, 90, 100)
         x_weights:  ( 0,   1,  0,  0,   1)

         y_mins:     (25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25)
         y_weights:  ( 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1)
      }
   }

   defctrl: {
      font:          Verdana
      transparent:   true
      pid:           100
   }

   ctrl: {
      id:            101,
      type:          label,
      cells:         (0,0,1,1)
      text:          "Name:",
   },

   ctrl: {
      id:            102
      type:          label
      cells:         (0,1,1,1)
      text:          "Class:",
   },

   ctrl: {
      id:            103
      type:          label
      cells:         (0,2,1,1)
      text:          "Design:"
   },

   ctrl: {
      id:            113
      type:          label
      cells:         (0,3,1,1)
      text:          "Skin:"
   },

   ctrl: {
      id:            104
      type:          label
      cells:         (0,4,1,1)
      text:          "Size:"
   },

   ctrl: {
      id:            105,
      type:          label,
      cells:         (0,5,1,1)
      text:          "IFF:",
   },

   ctrl: {
      id:            107,
      type:          label,
      cells:         (0,6,1,1)
      text:          "Role:",
   },

   ctrl: {
      id:            108,
      type:          label,
      cells:         (0,7,1,1)
      text:          "Sector:",
   },

   ctrl: {
      id:            109,
      type:          label,
      cells:         (0,8,1,1)
      text:          "Loc: (km)",
   },

   ctrl: {
      id:            110,
      type:          label,
      cells:         (0,9,1,1)
      text:          "Heading:",
   },

   ctrl: {
      id:            111,
      type:          label,
      cells:         (0,10,1,1)
      text:          "Hold Time:",
   },

   ctrl: {
      id:            112,
      type:          label,
      cells:         (0,11,1,1)
      text:          "Intel:",
   },


   // right column:
   ctrl: {
      id:            121,
      type:          label,
      cells:         (3,0,1,1)
      text:          "Player:",
   },

   ctrl: {
      id:            121,
      type:          label,
      cells:         (3,1,1,1)
      text:          "Alert:",
   },

   ctrl: {
      id:            122,
      type:          label,
      cells:         (3,2,1,1)
      text:          "Playable:",
   },

   ctrl: {
      id:            123,
      type:          label,
      cells:         (3,3,1,1)
      text:          "Command AI:",
   },

   ctrl: {
      id:            124,
      type:          label,
      cells:         (3,4,1,1)
      text:          "Respawns:",
   },

   ctrl: {
      id:            125,
      type:          label,
      cells:         (3,5,1,1)
      text:          "Cmdr:",
   },

   ctrl: {
      id:            126,
      type:          label,
      cells:         (3,6,1,1)
      text:          "Carrier:",
   },

   ctrl: {
      id:            127,
      type:          label,
      cells:         (3,7,1,1)
      text:          "Squadron:",
   },

   ctrl: {
      id:            129,
      type:          label,
      cells:         (3,8,1,1)
      text:          "Loadout:",
   },

   ctrl: {
      id:            130,
      type:          label,
      cells:         (3,9,1,1)
      text:          "Objective:",
   },

   ctrl: {
      id:            131,
      type:          label,
      cells:         (3,10,1,1)
      text:          "Target:",
   },

   defctrl: {
      font:          Verdana,
      style:         0x02,
      scroll_bar:    0,
      active_color:  ( 62, 106, 151)
      back_color:    ( 41,  41,  41),
      border_color:  (192, 192, 192),
      transparent:   false
      simple:        true
      border:        true
      single_line:   true
      fixed_height:  18
      bevel_width:   3
   },

   ctrl: {
      id:            201,
      type:          edit,
      cells:         (1,0,1,1)
      text:          "",
   },

   ctrl: {
      id:            202,
      type:          combo,
      cells:         (1,1,1,1)
   },

   ctrl: {
      id:            203,
      type:          combo,
      cells:         (1,2,1,1)
   },

   ctrl: {
      id:            213,
      type:          combo,
      cells:         (1,3,1,1)
   },

   ctrl: {
      id:            204,
      type:          edit,
      cells:         (1,4,1,1)
      text:          "1",
   },

   ctrl: {
      id:            205,
      type:          edit,
      cells:         (1,5,1,1)
      text:          "",
   },

   ctrl: {
      id:            206,
      type:          combo,
      cells:         (1,6,1,1)
   },

   ctrl: {
      id:            207,
      type:          combo,
      cells:         (1,7,1,1)
   },

   ctrl: {
      id:            28
      type:          panel
      transparent:   true
      cells:         (1,8,1,1)
      layout:  {
         x_mins:     (20, 5, 20, 5, 20)
         x_weights:  ( 1, 0,  1, 0,  1)

         y_mins:     (25)
         y_weights:  ( 1)
      }
   }

   ctrl: {
      id:            208
      pid:           28
      type:          edit,
      cells:         (0,0,1,1)
      text:          "0", // X
   },

   ctrl: {
      id:            209
      pid:           28
      type:          edit,
      cells:         (2,0,1,1)
      text:          "0", // Y
   },

   ctrl: {
      id:            210
      pid:           28
      type:          edit,
      cells:         (4,0,1,1)
      text:          "0", // Z
   },

   ctrl: {
      id:            211,
      type:          combo,
      cells:         (1,9,1,1)

      item:          "North",
      item:          "East",
      item:          "South",
      item:          "West",
   },

   ctrl: {
      id:            212,
      type:          edit,
      cells:         (1,10,1,1)
      text:          "",
   },

   ctrl: {
      id:            229,
      type:          combo,
      cells:         (1,11,1,1)
   },

   // right column:
   defctrl: {
      active_color:  (250, 250, 100),
      back_color:    ( 92,  92,  92),
      active:        true,
      border:        false,

      fixed_width:      16
      fixed_height:     16

      standard_image:   Checkbox_0
      activated_image:  Checkbox_2
      transition_image: Checkbox_1
      sticky:           true
   }

   ctrl: {
      id:            221,
      type:          button,
      bevel_style:   checkbox,
      cells:         (4,0,1,1)
   },

   ctrl: {
      id:            222,
      type:          button,
      bevel_style:   checkbox,
      cells:         (4,1,1,1)
   },

   ctrl: {
      id:            223,
      type:          button,
      bevel_style:   checkbox,
      cells:         (4,2,1,1)
   },

   ctrl: {
      id:            224,
      type:          button,
      bevel_style:   checkbox,
      cells:         (4,3,1,1)
   },

   defctrl: {
      active_color:  ( 62, 106, 151)
      back_color:    ( 41,  41,  41),
      border_color:  (192, 192, 192),
      border:        true,
      fixed_width:      0
      fixed_height:     18

      standard_image:   ""
      activated_image:  ""
      transition_image: ""
      sticky:           false
   },

   ctrl: {
      id:            225,
      type:          edit,
      cells:         (4,4,1,1)
      text:          "0",
   },

   ctrl: {
      id:            226,
      type:          combo,
      cells:         (4,5,1,1)
   },

   ctrl: {
      id:            227,
      type:          combo,
      cells:         (4,6,1,1)
   },

   ctrl: {
      id:            228,
      type:          combo,
      cells:         (4,7,1,1)
   },

   ctrl: {
      id:            230,
      type:          combo,
      cells:         (4,8,1,1)
   },

   ctrl: {
      id:            231,
      type:          combo,
      cells:         (4,9,1,1)
   },

   ctrl: {
      id:            232,
      type:          combo,
      cells:         (4,10,1,1)
   },




   defctrl: {
      align:            left,
      font:             Limerick12,
      fore_color:       (0,0,0),
      standard_image:   Button17_0,
      activated_image:  Button17_1,
      transition_image: Button17_2,
      bevel_width:      6,
      margins:          (3,18,0,0)
      cell_insets:      (10,0,0,0)
      pid:              0
      fixed_height:     19
      fixed_width:      0
   }

   ctrl: {
      id:            1,
      type:          button,
      text:          "Apply",
      cells:         (2,4,1,1)
   }

   ctrl: {
      id:            2,
      type:          button,
      text:          "Cancel",
      cells:         (3,4,1,1),
   }
}
