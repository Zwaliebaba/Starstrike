FORM


//  Project:   Starshatter 4.5
//  File:      NavDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 400,  20, 200, 10)
      x_weights:  ( 0,   1,   0,   0,  0)

      y_mins:     (28, 30,  10,  100, 25, 15)
      y_weights:  ( 0,  0,   0,    1,  0,  0)
   },


   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Navigation",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,3,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   },


   // main panels:

   ctrl: {
      id:               800
      type:             panel
      transparent:      true

      cells:            (1,3,1,2)
      cell_insets:      (0,0,0,0)
      hide_partial:  false

      layout: {
         x_mins:     (100, 100, 100, 10, 24, 24)
         x_weights:  (  0,   0,   0,  1,  0,  0)

         y_mins:     (25, 325,  15, 75)
         y_weights:  ( 0,   1,   0,  0)
      }
   }

   ctrl: {
      id:               600
      pid:              800
      type:             panel
      transparent:      false

      texture:          Panel
      margins:          (12,12,12,0),

      cells:            (0,3,6,1)
      cell_insets:      (0,0,0,0)
      hide_partial:  false

      layout: {
         x_mins:     (10, 90, 90, 90, 90, 10)
         x_weights:  ( 0,  0,  1,  0,  1,  0)

         y_mins:     (70)
         y_weights:  ( 1)
      }
   }

   ctrl: {
      id:               850
      type:             panel
      transparent:      false

      texture:          Frame2
      margins:          (60,40,40,40),

      cells:            (3,3,1,1)
      cell_insets:      (0,0,0,0)
      hide_partial:  false

      layout: {
         x_mins:     (100, 100)
         x_weights:  (  0,   0)

         y_mins:     (30, 23, 23, 23, 15, 50, 50, 50, 15)
         y_weights:  ( 0,  0,  0,  0,  0,  1,  1,  0,  0)
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
      cells:         (0,1,6,1)
      fore_color:    (255,255,255)
      back_color:    (41, 41, 41)

      texture:       Panel
      margins:       (12,12,12,0),
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
      cells:      (0,0,1,1)
      text:       Galaxy
   },

   ctrl: {
      id:         102
      pid:        800
      type:       button
      cells:      (1,0,1,1)
      text:       System
   },

   ctrl: {
      id:         103
      pid:        800
      type:       button
      cells:      (2,0,1,1)
      text:       Sector
   },

   defctrl: {
      sticky:           false,
      standard_image:   Button17x17_0
      activated_image:  Button17x17_1
      transition_image: Button17x17_2
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
      cells:      (4,0,1,1)
      text:       "+"
   },

   ctrl: {
      id:         111
      pid:        800
      type:       button
      cells:      (5,0,1,1)
      text:       "-"
   },

   defctrl: {
      sticky:           true
      standard_image:   Tab17_0
      activated_image:  Tab17_1
      transition_image: Tab17_2
      align:            left
      fixed_width:      0
      fixed_height:     19
      cell_insets:      (5,2,0,0)
      margins:          (10,10,0,0)
   },

   ctrl: {
      id:         401
      pid:        850
      type:       button
      cells:      (0,1,1,1)
      text:       System
   }

   ctrl: {
      id:         403
      pid:        850
      type:       button
      cells:      (0,2,1,1)
      text:       Sector
   }

   ctrl: {
      id:         405
      pid:        850
      type:       button
      cells:      (0,3,1,1)
      text:       Starship
   }

   defctrl: {
      cell_insets:   (2,5,0,0)
   }

   ctrl: {
      id:         402
      pid:        850
      type:       button
      cells:      (1,1,1,1)
      text:       Planet
   }

   ctrl: {
      id:         404
      pid:        850
      type:       button
      cells:      (1,2,1,1)
      text:       Station
   }

   ctrl: {
      id:         406
      pid:        850
      type:       button
      cells:      (1,3,1,1)
      text:       Fighter
   }

   defctrl: {
      sticky:        false,
      back_color:    ( 41,  41,  41),
      fore_color:    (255, 255, 255),
      font:          Verdana
      fixed_width:   0
      fixed_height:  0

      scroll_bar:    2
      style:         0
      transparent:   false

      cell_insets:   (5,5,10,0)
   },


   defctrl: {
      back_color: ( 41,  41,  41),
      fore_color: ( 53, 159,  67),
      font:       "Verdana",
      transparent: true
   },

   ctrl: {
      id:         601
      pid:        600
      type:       label
      cells:      (1,0,1,1)
      text:       "Location"
   },

   ctrl: {
      id:         602
      pid:        600
      type:       label
      cells:      (3,0,1,1)
      text:       "Destination"
   },

   defctrl: {
      standard_image:   "",
      activated_image:  "",
      transition_image: "",
      fore_color: (255, 255, 255)
   },

   ctrl: {
      id:         701
      pid:        600
      type:       label
      cells:      (2,0,1,1)
   },

   ctrl: {
      id:         702
      pid:        600
      type:       label
      cells:      (4,0,1,1)
   },

   defctrl: {
      transparent: false
   },

   ctrl: {
      id:         801
      pid:        850
      type:       list
      cells:      (0,5,2,1)
      show_headings: true

      texture:       Panel
      margins:       (12,12,12,0),

      column:     {
         title:   Name
         width:   167
         align:   left
         sort:    0
      }
   }

   ctrl: {
      id:         802
      pid:        850
      type:       list
      cells:      (0,6,2,1)
      show_headings: false

      texture:       Panel
      margins:       (12,12,12,0),

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
      cell_insets:      (5,5,5,0)
      fixed_height:     19
   },

   ctrl: {
      id:            1
      pid:           850
      type:          button
      text:          Commit
      cells:         (0,7,2,1)
   },

   ctrl: {
      id:            2
      type:          button
      text:          Close
      cells:         (3,4,1,1),
   },

}
