FORM


//  Project:   Starshatter 4.5
//  File:      VidDlg.frm
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
         x_mins:     ( 20, 100, 100,  20, 100, 100,  20)
         x_weights:  (  2,   3,   3,   2,   3,   3,   2)

         y_mins:     ( 20,  25,  25,  25,  25,  25,  25,  25,  25,  20)
         y_weights:  (  3,   0,   0,   0,   0,   0,   0,   0,   0,   7)
      }
   }

   defctrl: {
      fore_color:       (255,255,255)
      font:             Verdana,
      standard_image:   ""
      activated_image:  ""
      align:            left
      sticky:           false
      transparent:      true
      pid:              300
   },

   ctrl: {
      id:               101
      type:             label
      text:             "Video Mode:"
      cells:            (1,1,1,1)
   },

   ctrl: {
      id:               104
      type:             label
      text:             "Max Texture Size:"
      cells:            (1,2,1,1)
   },

   ctrl: {
      id:               122
      type:             label
      text:             "Shadows:"
      cells:            (1,3,1,1)
   },

   ctrl: {
      id:               123
      type:             label
      text:             "Spec Maps:"
      cells:            (1,4,1,1)
   },

   ctrl: {
      id:               124
      type:             label
      text:             "Bump Maps:"
      cells:            (1,5,1,1)
   },

   ctrl: {
      id:               105
      type:             label
      text:             "Terrain Detail:"
      cells:            (1,7,1,1)
   },

   ctrl: {
      id:               106
      type:             label
      text:             "Terrain Texture:"
      cells:            (1,8,1,1)
   },


   ctrl: {
      id:               111
      type:             label
      text:             "Lens Flare:"
      cells:            (4,1,1,1)
   },

   ctrl: {
      id:               112
      type:             label
      text:             "Corona:"
      cells:            (4,2,1,1)
   },

   ctrl: {
      id:               113
      type:             label
      text:             "Nebula:"
      cells:            (4,3,1,1)
   },

   ctrl: {
      id:               114
      type:             label
      text:             "Space Dust:"
      cells:            (4,4,1,1)
   },

   ctrl: {
      id:               115
      type:             label
      text:             "Gamma Level:"
      cells:            (4,7,1,1)
   },


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
   },


   ctrl: {
      id:               203
      type:             combo
      cells:            (2,1,1,1)

      item:             "800 x 600 x 8"
      item:             "800 x 600 x 16"
      item:             "800 x 600 x 32"
   }

   ctrl: {
      id:               204
      type:             combo
      cells:            (2,2,1,1)

      item:             "64 x 64"
      item:             "128 x 128"
      item:             "256 x 256"
      item:             "512 x 512"
      item:             "1024 x 1024"
      item:             "2048 x 2048"
      item:             "4096 x 4096"
   }

   ctrl: {
      id:               222
      type:             combo
      cells:            (2,3,1,1)

      item:             Disable
      item:             Enable
   }

   ctrl: {
      id:               223
      type:             combo
      cells:            (2,4,1,1)

      item:             Disable
      item:             Enable
   }

   ctrl: {
      id:               224
      type:             combo
      cells:            (2,5,1,1)

      item:             Disable
      item:             Enable
   }

   ctrl: {
      id:               205
      type:             combo
      cells:            (2,7,1,1)

      item:             Low
      item:             Medium
      item:             High
   }

   ctrl: {
      id:               206
      type:             combo
      cells:            (2,8,1,1)

      item:             Disable
      item:             Enable
   }

   ctrl: {
      id:               211
      type:             combo
      cells:            (5,1,1,1)

      item:             Disable
      item:             Enable
   }

   ctrl: {
      id:               212
      type:             combo
      cells:            (5,2,1,1)

      item:             Disable
      item:             Enable
   }

   ctrl: {
      id:               213
      type:             combo
      cells:            (5,3,1,1)

      item:             Disable
      item:             Enable
   }

   ctrl: {
      id:               214
      type:             combo
      cells:            (5,4,1,1)

      item:             None
      item:             Some
      item:             Lots
   }

   ctrl: {
      id:               215
      type:             slider
      cells:            (5,7,1,1)
      cell_insets:      (0,0,0,16)

      active_color:     (250, 250, 100)
      back_color:       ( 41,  41,  41)
      border:           false
      active:           true
   }

   ctrl: {
      id:               315
      type:             label
      cells:            (5,8,1,1)
      cell_insets:      (0,0,0,0)

      texture:          gamma_test
      margins:          (0,0,0,0)
   }

   // apply and cancel buttons:

   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,10,0,26)
      pid:              0
   },

   ctrl: {
      id:            1
      type:          button
      text:          Apply
      cells:         (3,5,1,1)
   },

   ctrl: {
      id:            2
      type:          button
      text:          Cancel
      cells:         (4,5,1,1),
   },

}
