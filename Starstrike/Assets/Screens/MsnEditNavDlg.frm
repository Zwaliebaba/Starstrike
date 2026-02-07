FORM


//  Project:   Starshatter 4.5
//  File:      MsnEditNavDlg.frm
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
      x_weights:  ( 0,   2,   4,   2,   2,  0),

      y_mins:     (28, 30,  10,  90, 24, 60, 45),
      y_weights:  ( 0,  0,   0,   0,  0,  1,  0)
   },

   // background images:

   ctrl: {
      id:            20
      type:          background
      texture:       Frame4a
      cells:         (1,3,4,1),
      cell_insets:   (0,0,0,0),
      margins:       (2,2,16,16)
      hide_partial:  false
   }

   ctrl: {
      id:            21
      type:          background
      texture:       Frame2a
      cells:         (1,4,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (2,32,40,32)
      hide_partial:  false
   }

   ctrl: {
      id:            22
      type:          background
      texture:       Frame2b
      cells:         (3,4,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (0,40,40,32)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Mission Editor - Navigation Form",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,3,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   },

   // tabs:

   ctrl: {
      id:            90
      type:          panel
      transparent:   true
      cells:         (1,4,4,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false

      layout: {
         x_mins:     (80, 80, 80, 80,  5)
         x_weights:  ( 1,  1,  1,  1, 15)

         y_mins:     (24),
         y_weights:  ( 0)
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
      margins:          (8,8,0,0)
      cell_insets:      (0,4,0,0)
      pid:              90
   },

   ctrl: {
      id:            301
      type:          button
      text:          SIT
      cells:         (0,0,1,1)
   }

   ctrl: {
      id:            302
      type:          button
      text:          PKG
      cells:         (1,0,1,1)
   }

   ctrl: {
      id:            303
      type:          button
      text:          MAP
      cells:         (2,0,1,1)
   }

   // info panel:

   ctrl: {
      id:               70
      pid:              0
      type:             panel
      transparent:      false

      texture:          Panel
      margins:          (12,12,12,0),

      cells:            (1,3,4,1),
      cell_insets:      (10,10,12,10)

      layout: {
         x_mins:     ( 20, 60, 100, 60, 100, 20)
         x_weights:  (  0,  0,   1,  0,   1,  0)

         y_mins:     ( 10,  20,  20, 10)
         y_weights:  (  1,   1,   1,  1)
      }
   }

   defctrl: {
      align:            left
      bevel_width:      0
      font:             Verdana
      fore_color:       (255, 255, 255)
      standard_image:   ""
      activated_image:  ""
      transparent:      true
      margins:          (0,0,0,0)
      cell_insets:      (0,0,0,0)
      text_insets:      (1,1,1,1)
      pid:              70
   },

   ctrl: {
      id:            71
      type:          label
      cells:         (1,1,1,1)
      text:          "Name:"
   },

   ctrl: {
      id:            72
      type:          label
      cells:         (1,2,1,1)
      text:          "Type:"
   },

   ctrl: {
      id:            73
      type:          label
      cells:         (3,1,1,1)
      text:          "System:"
   },

   ctrl: {
      id:            74
      type:          label
      cells:         (3,2,1,1)
      text:          "Sector:"
   },

   defctrl: {
      style:         2
      scroll_bar:    0
      back_color:    ( 41,  41,  41)
      active_color:  ( 62, 106, 151)
      border_color:  (192, 192, 192)
      fore_color:    (255, 255, 255)
      border:        true
      simple:        true
      bevel_width:   3
      transparent:   false
      cell_insets:   (0,20,0,5)
      fixed_height:  18
   },

   ctrl: {
      id:            201,
      type:          edit,
      cells:         (2,1,1,1)
      single_line:   true,
   },

   ctrl: {
      id:            202,
      type:          combo,
      cells:         (2,2,1,1)

      item: "Patrol",
      item: "Sweep",
      item: "Intercept",
      item: "Airborne Patrol",
      item: "Airborne Sweep",
      item: "Airborne Intercept",
      item: "Strike",
      item: "Assault",
      item: "Defend",
      item: "Escort",
      item: "Freight Escort",
      item: "Shuttle Escort",
      item: "Strike Escort",
      item: "Intel",
      item: "Scout",
      item: "Recon",
      item: "Blockade",
      item: "Fleet",
      item: "Attack",
      item: "Flight Ops",
      item: "Transport",
      item: "Cargo",
      item: "Training",
      item: "Misc",
   }

   ctrl: {
      id:            203,
      type:          combo,
      cells:         (4,1,1,1)
   }

   ctrl: {
      id:            204,
      type:          combo,
      cells:         (4,2,1,1)
   }

   defctrl: {
      fixed_height:  0
      fixed_width:   0
      pid:           0
   }


   // main panel:

   ctrl: {
      id:               800
      type:             panel
      transparent:      false

      texture:          Panel
      margins:          (12,12,12,0),

      cells:            (1,5,4,2)
      cell_insets:      (10,10,12,54)

      layout: {
         x_mins:     ( 10, 100, 100, 100, 10, 24, 24, 15, 95, 90, 10)
         x_weights:  (  0,   0,   0,   0,  1,  0,  0,  0,  0,  0,  0)

         y_mins:     ( 15,  25,  25,  25, 50, 50, 15)
         y_weights:  (  0,   0,   0,   0,  1,  1,  0)
      }
   }

   defctrl: {
      fore_color:    (0,0,0)
      font:          Limerick12
      bevel_width:   0
      bevel_depth:   128
      border:        true
      border_color:  (0,0,0)
      transparent:   false
      sticky:        true
   },

   ctrl: {
      id:            100
      pid:           800
      type:          label
      cells:         (1,2,6,4)
      cell_insets:   (0,0,0,0)
      fore_color:    (255,255,255)
      back_color:    (41, 41, 41)
      style:         2
      font:          Limerick12
   },

   defctrl: {
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,5,0,6)
   }

   ctrl: {
      id:         101
      pid:        800
      type:       button
      cells:      (1,1,1,1)
      text:       Galaxy
   },

   ctrl: {
      id:         102
      pid:        800
      type:       button
      cells:      (2,1,1,1)
      text:       System
   },

   ctrl: {
      id:         103
      pid:        800
      type:       button
      cells:      (3,1,1,1)
      text:       Sector
   },

   defctrl: {
      sticky:           false,
      standard_image:   Button17x17_0
      activated_image:  Button17x17_1
      transition_image: Button17x17_2
      //font:             Verdana,
      align:            center
      sticky:           false
      margins:          (0,0,0,0)
      cell_insets:      (5,0,0,1)
      fixed_width:      19
      fixed_height:     19
   },

   ctrl: {
      id:         110
      pid:        800
      type:       button
      cells:      (5,1,1,1)
      text:       "+"
   },

   ctrl: {
      id:         111
      pid:        800
      type:       button
      cells:      (6,1,1,1)
      text:       "-"
   },

   defctrl: {
      sticky:           true
      standard_image:   Tab17_0
      activated_image:  Tab17_1
      transition_image: Tab17_2
      align:            left
      fixed_width:      0
      fixed_height:     0
      cell_insets:      (0,2,0,6)
      margins:          (10,10,0,0)
   },

   ctrl: {
      id:         401
      pid:        800
      type:       button
      cells:      (8,1,1,1)
      text:       System
   }

   ctrl: {
      id:         403
      pid:        800
      type:       button
      cells:      (8,2,1,1)
      text:       Sector
   }

   ctrl: {
      id:         405
      pid:        800
      type:       button
      cells:      (8,3,1,1)
      text:       Starship
   }

   defctrl: {
      cell_insets:   (2,0,0,6)
   }

   ctrl: {
      id:         402
      pid:        800
      type:       button
      cells:      (9,1,1,1)
      text:       Planet
   }

   ctrl: {
      id:         404
      pid:        800
      type:       button
      cells:      (9,2,1,1)
      text:       Station
   }

   ctrl: {
      id:         406
      pid:        800
      type:       button
      cells:      (9,3,1,1)
      text:       Fighter
   }

   defctrl: {
      sticky:        false,
      back_color:    ( 41,  41,  41),
      fore_color:    (255, 255, 255),
      font:          Verdana

      scroll_bar:    2
      style:         0x02
      transparent:   false

      cell_insets:   (0,0,5,0)
   },

   ctrl: {
      id:         801
      pid:        800
      type:       list
      cells:      (8,4,2,1)
      show_headings: true

      column:     {
         title:   Name
         width:   167
         align:   left
         sort:    0
      }
   }

   ctrl: {
      id:         802
      pid:        800
      type:       list
      cells:      (8,5,2,1)
      show_headings: false

      column:     {
         title:   Name,
         width:   70,
         align:   left,
         sort:    0,
         color:   ( 53, 159,  67) 
      },

      column:     {
         title:   Value,
         width:   92,
         align:   left,
         sort:    0,
         color:   (255, 255, 255) 
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
      pid:              0
      fixed_height:     19
      fixed_width:      0
   },

   ctrl: {
      id:            1
      type:          button
      text:          Accept
      cells:         (3,6,1,1)
   },

   ctrl: {
      id:            2
      type:          button
      text:          Cancel
      cells:         (4,6,1,1),
   },

}
