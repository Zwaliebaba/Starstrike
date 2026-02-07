FORM


//  Project:   Starshatter 4.5
//  File:      AudDlg.frm
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
         x_weights:  (0.2, 0.3, 0.3, 0.2, 0.3, 0.3, 0.2)

         y_mins:     ( 20,  25,  25,  25,  25,  25,  25,  25,  20)
         y_weights:  (0.3,   0,   0,   0,   0,   0,   0,   0, 0.7)
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
   },

   ctrl: {
      id:               101
      pid:              300
      type:             label
      text:             "Effects Volume:"
      cells:            (1,1,1,1)
   },

   ctrl: {
      id:               102
      pid:              300
      type:             label
      text:             "GUI Volume:"
      cells:            (1,2,1,1)
   },

   ctrl: {
      id:               103
      pid:              300
      type:             label
      text:             "Warning Volume:"
      cells:            (1,3,1,1)
   },

   ctrl: {
      id:               104
      pid:              300
      type:             label
      text:             "Vox Volume:"
      cells:            (1,4,1,1)
   },

   ctrl: {
      id:               105
      pid:              300
      type:             label
      text:             "Menu Music:"
      cells:            (1,6,1,1)
   },

   ctrl: {
      id:               106
      pid:              300
      type:             label
      text:             "In Game Music:"
      cells:            (1,7,1,1)
   },

   defctrl: {
      cell_insets:      (0,0,0,16)
      simple:           true
      bevel_width:      3
      text_align:       left
      transparent:      false

      active_color:     (250, 250, 100)
      back_color:       ( 41,  41,  41)
      border:           false
      active:           true
   },

   ctrl: {
      id:               201
      pid:              300
      type:             slider
      cells:            (2,1,1,1)
   },

   ctrl: {
      id:               202
      pid:              300
      type:             slider
      cells:            (2,2,1,1)
   },

   ctrl: {
      id:               203
      pid:              300
      type:             slider
      cells:            (2,3,1,1)
   },

   ctrl: {
      id:               204
      pid:              300
      type:             slider
      cells:            (2,4,1,1)
   },

   ctrl: {
      id:               205
      pid:              300
      type:             slider
      cells:            (2,6,1,1)
   },

   ctrl: {
      id:               206
      pid:              300
      type:             slider
      cells:            (2,7,1,1)
   },


   // buttons:

   defctrl: {
      align:            left,
      font:             Limerick12,
      fore_color:       (0,0,0),
      standard_image:   Button17_0,
      activated_image:  Button17_1,
      transition_image: Button17_2,
      bevel_width:      6,
      margins:          (3,18,0,0),
      cell_insets:      (0,10,0,26)
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
