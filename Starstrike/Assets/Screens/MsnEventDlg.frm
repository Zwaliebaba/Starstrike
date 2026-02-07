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
      active_color:  ( 62, 106, 151)
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
      text:          "Mission Event"
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
         x_mins:     (80, 100, 30, 90, 100)
         x_weights:  ( 0,   1,  0,  0,   1)

         y_mins:     (25, 25, 25, 25, 25, 25, 25, 25, 25, 25)
         y_weights:  ( 0,  0,  0,  0,  0,  0,  0,  0,  0,  1)
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
      text:          "ID:",
   },

   ctrl: {
      id:            102,
      type:          label,
      cells:         (0,1,1,1)
      text:          "Time:",
   },

   ctrl: {
      id:            103,
      type:          label,
      cells:         (0,2,1,1)
      text:          "Delay:",
   },

   ctrl: {
      id:            104,
      type:          label,
      cells:         (0,3,1,1)
      text:          "Action:",
   },

   ctrl: {
      id:            105,
      type:          label,
      cells:         (0,4,1,1)
      text:          "Ship:",
   },

   ctrl: {
      id:            106,
      type:          label,
      cells:         (0,5,1,1)
      text:          "Source:",
   },

   ctrl: {
      id:            107,
      type:          label,
      cells:         (0,6,1,1)
      text:          "Target:",
   },

   ctrl: {
      id:            108,
      type:          label,
      cells:         (0,7,1,1)
      text:          "Param",
   },

   ctrl: {
      id:            109,
      type:          label,
      cells:         (0,8,1,1)
      text:          "Sound:",
   },

   ctrl: {
      id:            110,
      type:          label,
      cells:         (0,9,1,1)
      text:          "Message:",
   },


   // right column:
   ctrl: {
      id:            121,
      type:          label,
      cells:         (3,1,1,1)
      text:          "Chance:",
   },

   ctrl: {
      id:            121,
      type:          label,
      cells:         (3,3,1,1)
      text:          "Trigger:",
   },

   ctrl: {
      id:            122,
      type:          label,
      cells:         (3,4,1,1)
      text:          "Ship:",
   },

   ctrl: {
      id:            123,
      type:          label,
      cells:         (3,5,1,1)
      text:          "Target:",
   },

   ctrl: {
      id:            124,
      type:          label,
      cells:         (3,6,1,1)
      text:          "Param:",
   },

   ctrl: {
      id:            201,
      type:          label,
      cells:         (1,0,1,1)
      text:          "",
   },

   defctrl: {
      font:          Verdana,
      style:         0x02,
      scroll_bar:    0,
      active_color:  ( 62, 106, 151)
      back_color:    ( 41,  41,  41)
      border_color:  (192, 192, 192)
      transparent:   false
      simple:        true
      border:        true
      single_line:   true
      fixed_height:  18
      bevel_width:   3
   },

   ctrl: {
      id:            202,
      type:          edit,
      cells:         (1,1,1,1)
   },

   ctrl: {
      id:            203,
      type:          edit,
      cells:         (1,2,1,1)
   },

   ctrl: {
      id:            204,
      type:          combo,
      cells:         (1,3,1,1)
   },

   ctrl: {
      id:            205,
      type:          combo,
      cells:         (1,4,1,1)
   },

   ctrl: {
      id:            206,
      type:          combo,
      cells:         (1,5,1,1)
   },

   ctrl: {
      id:            207,
      type:          combo,
      cells:         (1,6,1,1)
   },

   ctrl: {
      id:            208,
      type:          edit,
      cells:         (1,7,1,1)
   },

   ctrl: {
      id:            209,
      type:          edit,
      cells:         (1,8,1,1)
   },

   ctrl: {
      id:            210,
      type:          edit,
      cells:         (1,9,4,1)
      cell_insets:   (0,0,0,15)
      fixed_height:  0
   },

   defctrl: {
      active:        true,
      active_color:  ( 62, 106, 151)
      back_color:    ( 41,  41,  41)
      border_color:  (192, 192, 192)
      border:        true,
      sticky:        true,
   },

   // right column:
   ctrl: {
      id:            220,
      type:          edit,
      cells:         (4,1,1,1)
   },

   ctrl: {
      id:            221,
      type:          combo,
      cells:         (4,3,1,1)
   },

   ctrl: {
      id:            222,
      type:          combo,
      cells:         (4,4,1,1)
   },

   ctrl: {
      id:            223,
      type:          combo,
      cells:         (4,5,1,1)
   },

   ctrl: {
      id:            224,
      type:          edit,
      cells:         (4,6,1,1)
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
