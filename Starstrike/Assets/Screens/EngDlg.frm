FORM


//  Project:   Starshatter 4.5
//  File:      EngDlg.frm
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
      y_weights:  ( 0,  0,   0,   1,  0,  2,  0)
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
      text:          Engineering
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
         x_mins:     ( 10, 100, 10, 100, 10, 100, 10, 100, 10)
         x_weights:  (  0,   1,  0,   1,  0,   1,  0,   1,  0)

         y_mins:     ( 10,  25,  10,  25,  10)
         y_weights:  (  0,   0,   0,   1,   0)
      }
   }


   defctrl: {
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
  }

   ctrl: {
      id:            201
      type:          button
      cells:         (1,1,1,1)
      text:          "Reac 1"
   },

   ctrl: {
      id:            202
      type:          button
      cells:         (3,1,1,1)
      text:          "Reac 2"
   },

   ctrl: {
      id:            203
      type:          button
      cells:         (5,1,1,1)
      text:          "Reac 3"
   },

   ctrl: {
      id:            204
      type:          button
      cells:         (7,1,1,1)
      text:          "Reac 4"
   },

   defctrl: {
      standard_image:   ""
      activated_image:  ""
      transition_image: ""
      fixed_height:     0
      active:           false
      active_color:     (255,255,128)
      border:           false
   },

   ctrl: {
      id:            211
      type:          slider
      cells:         (1,2,1,1)
   },

   ctrl: {
      id:            212
      type:          slider
      cells:         (3,2,1,1)
   },

   ctrl: {
      id:            213
      type:          slider
      cells:         (5,2,1,1)
   },

   ctrl: {
      id:            214
      type:          slider
      cells:         (7,2,1,1)
   },

   defctrl: {
      back_color:       ( 61,  61,  59),
      fore_color:       (255, 255, 255),
      font:             Verdana,
      leading:          2,
      multiselect:      1,
      dragdrop:         1,
      scroll_bar:       2,
      show_headings:    true,
      style:            0x20,

      texture:          Panel
      margins:          (12,12,12,0)
   },

   ctrl: {
      id:         301
      type:       list
      cells:      (1,3,1,1)

      column:     {
         title:   SYSTEM,
         width:   107,
         align:   left,
         sort:    1 },

      column:     {
         title:   POWER,
         width:   59,
         align:   right,
         sort:    -2 }
   },

   ctrl: {
      id:         302,
      type:       list
      cells:      (3,3,1,1)

      column:     {
         title:   SYSTEM,
         width:   107,
         align:   left,
         sort:    1 },

      column:     {
         title:   POWER,
         width:   59,
         align:   right,
         sort:    -2 }
   },

   ctrl: {
      id:         303,
      type:       list
      cells:      (5,3,1,1)

      column:     {
         title:   SYSTEM,
         width:   107,
         align:   left,
         sort:    1 },

      column:     {
         title:   POWER,
         width:   59,
         align:   right,
         sort:    -2 }
   },

   ctrl: {
      id:         304,
      type:       list
      cells:      (7,3,1,1)

      column:     {
         title:   SYSTEM,
         width:   107,
         align:   left,
         sort:    1 },

      column:     {
         title:   POWER,
         width:   59,
         align:   right,
         sort:    -2 }
   },


   // lower panel:

   ctrl: {
      id:               1200
      pid:              0
      type:             panel
      transparent:      true

      cells:            (1,5,2,1),

      layout: {
         x_mins:     ( 10, 50, 50, 10, 50, 50, 50, 10, 100, 10)
         x_weights:  (  0,  3,  3,  0,  2,  2,  2,  0,   6,  0)

         y_mins:     ( 30, 20, 30, 30, 20, 30, 20, 30, 23, 18,  5)
         y_weights:  (  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0)
      }
   }


   defctrl: {
      back_color:    ( 92,  92,  92)
      fore_color:    (255, 255, 255)
      font:          Limerick12
      border:        false
      transparent:   false
      pid:           1200
   },

   ctrl: {
      id:            401
      pid:           1200
      type:          label
      cells:         (1,1,2,1)
      style:         0x0040
      back_color:    (41,41,41)
      transparent:   true
   },

   defctrl: {
      bevel_width:      0
      border:           false
      fore_color:       (0,0,0)
      align:            left
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      margins:          (3,18,0,0)
      fixed_height:     19
   }

   ctrl: {
      id:            402
      pid:           1200
      type:          button
      cells:         (1,2,1,1)
      cell_insets:   (0,3,0,0)
      text:          "PWR OFF"
      sticky:        true
   }

   ctrl: {
      id:            403
      pid:           1200
      type:          button
      cells:         (2,2,1,1)
      cell_insets:   (3,0,0,0)
      text:          "PWR ON"
      sticky:        true
   }

   ctrl: {
      id:            410
      pid:           1200
      type:          button
      cells:         (1,3,2,1)
      text:          OVERRIDE
      sticky:        true
   }

   ctrl: {
      id:            700
      pid:           1200
      type:          button
      cells:         (1,8,2,1)
      text:          "Auto Repair"
      sticky:        true
   }

   defctrl: {
      standard_image:   ""
      activated_image:  ""
      transition_image: ""
      fixed_height:     0
   }

   ctrl: {
      id:            450
      pid:           1200
      type:          label
      cells:         (1,4,2,1)
      text:          "Power Allocation",
      font:          Verdana,
      transparent:   true
   },

   ctrl: {
      id:            404
      pid:           1200
      type:          slider
      cells:         (1,5,2,1)
      active:        true
      active_color:  (255,255,128)
      border:        false
      fixed_height:  10
   }

   ctrl: {
      id:            451
      pid:           1200
      type:          label
      cells:         (1,6,2,1)
      text:          "Capacitor Charge"
      font:          Verdana
      transparent:   true
   }

   ctrl: {
      id:            405
      pid:           1200
      type:          slider
      cells:         (1,7,2,1)
      active:        false
      active_color:  (255,255,128)
      border:        false
      fixed_height:  10
   }



   ctrl: {
      id:            500
      pid:           1200
      type:          label
      cells:         (4,1,3,1)
      style:         0x0040,
      back_color:    (41,41,41),
      text:          "Components",
      transparent:   true,
      font:          Limerick12,
      fore_color:    (255,255,255),
   },

   ctrl: {
      id:            501
      pid:           1200
      type:          list
      cells:         (4,2,3,6)
      cell_insets:   (0,0,0,6)

      font:          Verdana
      fore_color:    (255, 255, 255)
      back_color:    ( 61,  61,  59)
      leading:       2
      multiselect:   0
      dragdrop:      0
      scroll_bar:    2
      show_headings: true
      style:         0x20
      transparent:   false

      texture:       Panel
      margins:       (12,12,12,0)

      column:     {
         title:   COMPONENT,
         width:   155,
         align:   left,
         sort:    1 },

      column:     {
         title:   STATUS,
         width:   82,
         align:   left,
         sort:    -2 },

      column:     {
         title:   SPARES,
         width:   65,
         align:   right,
         sort:    -2 }
   },

   defctrl: {
      bevel_width:      0
      border:           false
      fore_color:       (0,0,0)
      align:            left
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      margins:          (3,18,0,0)
      fixed_height:     19
   }

   ctrl: {
      id:            502
      pid:           1200
      type:          button
      cells:         (4,8,1,1)
      cell_insets:   (0,3,0,0)
      text:          "Repair"
      sticky:        false
   },

   ctrl: {
      id:            503,
      pid:           1200
      type:          button
      cells:         (5,8,1,1)
      cell_insets:   (3,0,0,0)
      text:          "Replace"
      sticky:        false
   },

   ctrl: {
      id:            512
      pid:           1200
      type:          label
      cells:         (4,9,1,1)
      cell_insets:   (0,3,0,0)
      // text:          "Repair Time"
      font:          Verdana
      align:         right
      transparent:   true
   },

   ctrl: {
      id:            513,
      pid:           1200
      type:          label
      cells:         (5,9,1,1)
      cell_insets:   (3,0,0,0)
      // text:          "Replace Time"
      font:          Verdana
      align:         right
      transparent:   true
   },

   ctrl: {
      id:            600
      pid:           1200
      type:          label
      cells:         (8,1,1,1)
      style:         0x0040
      text:          "Repair Queue"
      transparent:   true
      font:          Limerick12
      fore_color:    (255,255,255)
      back_color:    (41,41,41)
   }

   ctrl: {
      id:            601
      pid:           1200
      type:          list
      cells:         (8,2,1,6)
      cell_insets:   (0,0,0,6)
      fixed_height:  0

      font:          Verdana
      fore_color:    (255, 255, 255)
      back_color:    ( 61,  61,  59)
      leading:       2
      multiselect:   0
      dragdrop:      0
      scroll_bar:    2
      show_headings: true
      style:         0x20
      transparent:   false

      texture:       Panel
      margins:       (12,12,12,0)


      column:     {
         title:   SYSTEM,
         width:   212,
         align:   left,
         sort:    3 },

      column:     {
         title:   ETR,
         width:   90,
         align:   right,
         sort:    3 }
   },

   defctrl: {
      align:            center
      standard_image:   Button17x17_0
      activated_image:  Button17x17_1
      transition_image: Button17x17_2
      cell_insets:      (0,0,0,0)
      fixed_width:      19
      fixed_height:     19
   }

   ctrl: {
      id:            602
      pid:           1200
      type:          button
      cells:         (8,8,1,1)
      text:          "^"
   },

   ctrl: {
      id:            603
      pid:           1200
      type:          button
      cells:         (8,8,1,1)
      cell_insets:   (25,0,0,0)
      text:          "v"
   },


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
