FORM


//  Project:   Starshatter 4.5
//  File:      MenuDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),
   
   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 100,  20, 100, 100, 10),
      x_weights:  ( 0, 0.2, 0.4, 0.2, 0.2,  0),

      y_mins:     (28, 30,  20,  24, 60, 45),
      y_weights:  ( 0,  0,   0,   0,  1,  0)
   },

   // background images:

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame2a,
      cells:         (1,3,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (2,32,40,32)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame2b,
      cells:         (3,3,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (0,40,40,32)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Options",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,3,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   }


   // tabs:

   ctrl: {
      id:            900
      type:          panel
      transparent:   true
      cells:         (1,3,4,1)
      hide_partial:  false

      layout: {
         x_mins:     (100, 100, 100, 100, 100, 0),
         x_weights:  (0.2, 0.2, 0.2, 0.2, 0.2, 1),

         y_mins:     (24),
         y_weights:  ( 1)
      }
   }

   defctrl: {
      align:            left,
      font:             Limerick12,
      fore_color:       (255, 255, 255),
      standard_image:   BlueTab_0,
      activated_image:  BlueTab_1,
      sticky:           true,
      bevel_width:      6,
      margins:          (8,8,0,0),
      cell_insets:      (0,4,0,0)
   },

   ctrl: {
      id:            901
      pid:           900
      type:          button
      text:          Video
      cells:         (0,0,1,1)
   }

   ctrl: {
      id:            902
      pid:           900
      type:          button
      text:          Audio
      cells:         (1,0,1,1)
   }

   ctrl: {
      id:            903
      pid:           900
      type:          button
      text:          Controls
      cells:         (2,0,1,1)
   }

   ctrl: {
      id:            904
      pid:           900
      type:          button
      text:          Gameplay
      cells:         (3,0,1,1)
   }

   // main panel:

   ctrl: {
      id:               300
      type:             panel
      transparent:      false //true

      texture:          Panel
      margins:          (12,12,12,0),

      cells:            (1,4,4,2)
      cell_insets:      (10,10,12,54)

      layout: {
         x_mins:     (10,  90,  90,  90,  90,  20,  80, 120,  10)
         x_weights:  ( 2,   3,   3,   3,   3,   2,   3,   3,   2)

         y_mins:     (20,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  0, 20)
         y_weights:  ( 1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  3,  2)
      }
   }

   defctrl: {
      fore_color:       (0,0,0)
      font:             Limerick12,
      align:            left
      sticky:           true
      standard_image:   Tab17_0
      activated_image:  Tab17_1
      transition_image: Tab17_2
      bevel_width:      6
      margins:          (10,10,0,0)
      cell_insets:      (2,3,0,6)
      transparent:      false
   },

   ctrl: {
      id:               101
      pid:              300
      type:             button
      text:             " Flight"
      cells:            (1,1,1,1)
      cell_insets:      (0,3,0,6)
   },

   ctrl: {
      id:               102
      pid:              300
      type:             button
      text:             " Wep"
      cells:            (2,1,1,1)
   },

   ctrl: {
      id:               103
      pid:              300
      type:             button
      text:             " View"
      cells:            (3,1,1,1)
   },

   ctrl: {
      id:               104
      pid:              300
      type:             button
      text:             " Misc"
      cells:            (4,1,1,1)
      cell_insets:      (2,0,0,6)
   },

   defctrl: {
      font:             Verdana
      fore_color:       (255, 255, 255)
      back_color:       ( 41,  41,  41)
      simple:           true
      bevel_width:      3
      text_align:       left
      cell_insets:      (0,0,0,0)

      standard_image:   ""
      activated_image:  ""
      transition_image: ""
      sticky:           false
   },

   ctrl: {
      id:               200
      pid:              300
      type:             list
      cells:            (1,2,4,10)

      style:            0x02
      scroll_bar:       2
      show_headings:    true
      transparent:      false

      column:     {
         title:   COMMAND
         width:   160
         align:   left
         sort:    0 },

      column:     {
         title:   KEY
         width:   197
         align:   left
         sort:    0 }
   }


   defctrl: {
      transparent:      true
   }

   ctrl: {
      id:               110
      pid:              300
      type:             label
      text:             "Control Style:"
      cells:            (6,1,1,1)
   }

   ctrl: {
      id:               112
      pid:              300
      type:             label
      text:             "Throttle:"
      cells:            (6,3,1,1)
   }

   ctrl: {
      id:               113
      pid:              300
      type:             label
      text:             "Rudder:"
      cells:            (6,4,1,1)
   }

   ctrl: {
      id:               114
      pid:              300
      type:             label
      text:             "Sensitivity:"
      cells:            (6,5,1,1)
   }

   ctrl: {
      id:               115
      pid:              300
      type:             label
      text:             "Joy Axis:"
      cells:            (6,6,1,1)
   }

   ctrl: {
      id:               411
      pid:              300
      type:             label
      text:             "Mouse:"
      cells:            (6,8,1,1)
   }

   ctrl: {
      id:               414
      pid:              300
      type:             label
      text:             "Sensitivity:"
      cells:            (6,9,1,1)
   }

   ctrl: {
      id:               415
      pid:              300
      type:             label
      text:             "Inverted:"
      cells:            (6,10,1,1)
   }

   defctrl: {
      back_color:    ( 41,  41,  41)
      border_color:  (192, 192, 192)
      active_color:  ( 92,  92,  92)

      cell_insets:   (0,0,0,5)
      font:          Verdana
      simple:        true
      bevel_width:   3
      text_align:    left
      transparent:   false
   }

   ctrl: {
      id:               210
      pid:              300
      type:             combo
      cells:            (7,1,1,1)

      item:          "Aircraft",
      item:          "Spacecraft",
   }

   ctrl: {
      id:               211
      pid:              300
      type:             combo
      cells:            (7,2,1,1)

      item:          "Disable",
      item:          "Joystick 1",
      item:          "Joystick 2",
      item:          "Both"
   }

   ctrl: {
      id:               212
      pid:              300
      type:             combo
      cells:            (7,3,1,1)

      item:             Disable
      item:             Enable
   }

   ctrl: {
      id:               213
      pid:              300
      type:             combo
      cells:            (7,4,1,1)

      item:             Disable
      item:             Enable
   }

   ctrl: {
      id:               214
      pid:              300
      type:             slider
      cells:            (7,5,1,1)
      cell_insets:      (0,0,6,10)

      active_color:     (250, 250, 100)
      back_color:       (  0,   0,   0)
      border_color:     ( 92,  92,  92)
      active:           true
   }

   ctrl: {
      id:               215
      pid:              300
      type:             button
      cells:            (7,6,1,1)
      text:             "Setup...",

      fore_color:       (0,0,0)
      font:             Limerick12,
      align:            left
      sticky:           true
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (2,3,0,6)
   }

   ctrl: {
      id:               511
      pid:              300
      type:             combo,
      cells:            (7,8,1,1)

      item:          "Disable",
      item:          "Mouse Look",
      item:          "Virtual Stick",
   }

   ctrl: {
      id:               514
      pid:              300
      type:             slider,
      cells:            (7,9,1,1)
      cell_insets:      (0,0,6,10)

      active_color:  (250, 250, 100),
      back_color:    (  0,   0,   0),
      border_color:  ( 92,  92,  92),
      active:        true,
   },

   ctrl: {
      id:               515
      pid:              300
      type:             button
      cells:            (7,10,1,1)
      fixed_width:      16
      fixed_height:     16

      standard_image:   Checkbox_0
      activated_image:  Checkbox_2
      transition_image: Checkbox_1
      sticky:           true
   },


   // buttons:

   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,10,0,26)
      transparent:      false
   },

   ctrl: {
      id:            1,
      type:          button,
      text:          "Apply",
      cells:         (3,5,1,1)
   },

   ctrl: {
      id:            2,
      type:          button,
      text:          "Cancel",
      cells:         (4,5,1,1),
   },

}
