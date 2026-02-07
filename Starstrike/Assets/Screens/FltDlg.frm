FORM


//  Project:   Starshatter 4.5
//  File:      FltDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 100, 100, 10),
      x_weights:  ( 0,   3,   1,  0),

      y_mins:     (28, 30,  10,  50, 10, 60, 45),
      y_weights:  ( 0,  0,   0,   1,  0,  1,  0)
   },

   // background images:

   ctrl: {
      id:            9990
      type:          background
      texture:       Frame4a
      cells:         (1,3,2,2),
      cell_insets:   (0,0,0,0),
      margins:       (2,2,16,16)
      hide_partial:  false
   }

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame2a,
      cells:         (1,5,1,2),
      cell_insets:   (0,0,0,10),
      margins:       (2,32,40,32)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame2b,
      cells:         (2,5,1,2),
      cell_insets:   (0,0,0,10),
      margins:       (0,40,40,32)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10
      type:          label
      text:          "Flight Operations"
      align:         left
      font:          Limerick18
      fore_color:    (255,255,255)
      transparent:   true
      cells:         (1,1,3,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   }


   // upper panel:

   ctrl: {
      id:               1100
      type:             panel
      transparent:      true

      cells:            (1,3,2,2),

      layout: {
         x_mins:     ( 10, 100, 100, 20, 100, 10)
         x_weights:  (  0,   0,   1,  0,   1,  0)

         y_mins:     ( 10,  25,  25,  25,  25,  25,  25,  10)
         y_weights:  (  0,   0,   0,   0,   0,   0,   1,   0)
      }
   }

   ctrl: {
      id:            11
      pid:           1100
      type:          label
      cells:         (1,1,1,1)
      cell_insets:   (0,0,0,0)
      text:          "Squadron"
      align:         left
      font:          Limerick12
      style:         0x0040
      back_color:    (41,41,41)
      fore_color:    (255,255,255)
      transparent:   true
   }

   ctrl: {
      id:            101
      pid:           1100
      type:          combo
      cells:         (2,1,1,1)

      fore_color:    (255, 255, 255)
      back_color:    ( 60,  60,  60)
      border_color:  (192, 192, 192)
      active_color:  ( 92,  92,  92)

      font:          Verdana
      simple:        true
      bevel_width:   3
      text_align:    left
      transparent:   false
      fixed_height:  20
   },

   ctrl: {
      id:               102
      pid:              1100
      type:             list
      cells:            (1,2,2,5)

      transparent:      false
      texture:          Panel
      margins:          (12,12,12,0)

      font:             Verdana
      fore_color:       (255, 255, 255)
      back_color:       ( 61,  61,  59)
      multiselect:      1
      style:            0x02
      item_style:       0x00
      selected_style:   0x02
      scroll_bar:       2
      leading:          2
      show_headings:    true

      column: {
         title:   INDEX,
         width:   90,
         align:   right,
         sort:    2,
      },

      column: {
         title:   NAME,
         width:   255,
         align:   left,
         sort:    1,
      },

      column: {
         title:   STATUS,
         width:   120,
         align:   left,
         sort:    1,
      },

      column: {
         title:   MISSION,
         width:   170,
         align:   left,
         sort:    1,
      },

      column: {
         title:   TIME,
         width:   97,
         align:   left,
         sort:    1,
      },
   },


   defctrl: {
      transparent:      false,
      align:            left,
      fore_color:       (0,0,0),
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      font:             Limerick12
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,0,0,0)
      fixed_height:     19
      pid:              1100
   },

   ctrl: {
      id:            110
      type:          button
      cells:         (4,2,1,1)
      text:          "Package"
   },

   ctrl: {
      id:            111
      type:          button
      cells:         (4,3,1,1)
      text:          "Alert"
   },

   ctrl: {
      id:            112
      type:          button
      cells:         (4,4,1,1)
      text:          "Launch"
   },

   ctrl: {
      id:            113
      type:          button
      cells:         (4,5,1,1)
      text:          "Standby"
   },

   ctrl: {
      id:            114
      type:          button
      cells:         (4,6,1,1)
      text:          "Recall"
   },




   // lower panel:

   ctrl: {
      id:               1200
      pid:              0
      type:             panel
      cells:            (1,5,2,1)
      transparent:      true
      fixed_height:     0
      fixed_width:      0


      layout: {
         x_mins:     ( 10, 100, 20, 200, 20, 200, 10)
         x_weights:  (  0,   1,  0,   2,  0,   2,  0)

         y_mins:     ( 30,  25,  25,  25,  25,  25,  25,  25,  10)
         y_weights:  (  0,   0,   0,   0,   0,   0,   0,   1,   0)
      }
   }

   defctrl: {
      back_color:    (41,41,41)
      fore_color:    (255, 255, 255)
      font:          Limerick12
      border:        false
      transparent:   true
      pid:           1200
   }

   ctrl: {
      id:            401
      type:          label
      text:          Objective
      cells:         (3,1,1,1)
      style:         0x0040
   }

   ctrl: {
      id:            402
      type:          label
      text:          Loadout
      cells:         (5,1,1,1)
      style:         0x0040
   }

   defctrl: {
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      font:             Limerick12
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,0,0,0)
      fixed_height:     19
      pid:              1200
   },

   ctrl: {
      id:            210
      type:          button
      cells:         (1,2,1,1)
      text:          "Patrol"
   },

   ctrl: {
      id:            211,
      type:          button,
      cells:         (1,3,1,1)
      text:          "Intercept"
   },

   ctrl: {
      id:            212,
      type:          button,
      cells:         (1,4,1,1)
      text:          "Assault"
   },

   ctrl: {
      id:            213,
      type:          button,
      cells:         (1,5,1,1)
      text:          "Strike"
   },

   ctrl: {
      id:            214,
      type:          button,
      cells:         (1,6,1,1)
      text:          "Escort"
   },

   ctrl: {
      id:            215,
      type:          button,
      cells:         (1,7,1,1)
      text:          "Scout"
   },

   defctrl: {
      font:             Verdana
      fore_color:       (255, 255, 255)
      back_color:       ( 61,  61,  59)
      transparent:      false
      texture:          Panel
      margins:          (12,12,12,0)
      fixed_height:     0
      style:            0x02
      item_style:       0x00
      selected_style:   0x02
      scroll_bar:       2
      leading:          2
      show_headings:    true
   }

   ctrl: {
      id:               221
      type:             list
      cells:            (3,2,1,6)

      column: {
         title:   NAME
         width:   130
         align:   left
         sort:    1
      }

      column: {
         title:   SECTOR
         width:   90
         align:   left
         sort:    1
      }

      column: {
         title:   RANGE
         width:   82
         align:   right
         sort:    1
      }
   }

   ctrl: {
      id:               222
      type:             list
      cells:            (5,2,1,6)

      column: {
         title:   NAME
         width:   190
         align:   left
         sort:    1
      }

      column: {
         title:   WEIGHT
         width:   112
         align:   right
         sort:    1
      }
   }


   // ok and cancel buttons:

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
      fixed_height:     19
      fixed_width:      0
      pid:              0
   },

   ctrl: {
      id:            1
      type:          button
      text:          Close
      cells:         (2,6,1,1)
   },
}
