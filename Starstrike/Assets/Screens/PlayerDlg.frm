FORM


//  Project:   Starshatter 4.5
//  File:      PlayerDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10,  80,  80, 10, 100, 100, 100, 10),
      x_weights:  ( 0, 0.1, 0.1,  0, 0.3, 0.3, 0.3,  0),

      y_mins:     (28, 25,  20,  42, 60, 45),
      y_weights:  ( 0,  0,   0,   0,  1,  0)
   },

   // background images:

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame3a
      cells:         (1,3,4,3),
      cell_insets:   (0,0,0,10),
      margins:       (48,80,48,48)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame3b
      cells:         (5,3,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (80,48,48,48)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Player Logbook",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,4,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   },


   defctrl: {
      base_color:       (191, 191, 184)
      back_color:       ( 88,  88,  88)
      fore_color:       (0,0,0)
      font:             Limerick12
      bevel_width:      0

      align:            left
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
   },

   ctrl: {
      id:            101,
      type:          button,
      cells:         (1,3,1,1)
      cell_insets:   (10,3,17,6)
      text:          "Create"
   },

   ctrl: {
      id:            102,
      type:          button,
      cells:         (2,3,1,1)
      cell_insets:   (3,10,17,6)
      text:          "Delete",
   }

   ctrl: {
      id:            601,
      type:          label,
      cells:         (4,3,1,1)
      cell_insets:   (2,10,17,0)
      text:          "Player Stats"
      fore_color:    (255,255,255)
      back_color:    (32,32,32)
      style:         0x40
      transparent:   true
   }

   defctrl: {
      font:          Verdana,
      fore_color:    (255,255,255),
      standard_image:   "",
      activated_image:  "",
      transition_image: "",
      transparent:   true,
   },

   ctrl: {
      id:            200
      type:          list
      cells:         (1,4,2,2)
      cell_insets:   (10,10,0,54)
      font:          Verdana
      style:         0
      scroll_bar:    2
      show_headings: false
      transparent:   false

      texture:       Panel
      margins:       (12,12,16,4)

      column:     {
         title:   "Name",
         width:   182,
         align:   left,
         sort:    0 },
   }

   ctrl: {
      id:            600
      type:          background
      cells:         (4,4,3,2)
      cell_insets:   (0,10,0,54)
      transparent:   false
      texture:       Panel
      margins:       (12,12,16,4)

      layout: {
         x_mins:     ( 8,  80, 100,  20, 30, 100,  15)
         x_weights:  ( 0,   0,   3,   1,  0,   3,   0)

         y_mins:     (20,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  65,  75)
         y_weights:  ( 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1)
      }
   }

   ctrl: {
      id:      103
      pid:     600
      type:    label
      cells:   (1,1,1,1)
      text:    "Name:"
   },

   ctrl: {
      id:      104,
      pid:     600
      type:    label,
      cells:   (1,2,1,1)
      text:    "Password:"
   },

   ctrl: {
      id:      105
      pid:     600
      type:    label
      cells:   (1,3,1,1)
      text:    "Squadron:"
   },

   ctrl: {
      id:      106
      pid:     600
      type:    label
      cells:   (1,4,1,1)
      text:    "Signature:"
   },

   ctrl: {
      id:      107
      pid:     600
      type:    label
      cells:   (1,5,1,1)
      text:    "Created:"
   }

   ctrl: {
      id:      109
      pid:     600
      type:    label
      cells:   (1,6,1,1)
      text:    "Flight Time:"
   },

   ctrl: {
      id:      110
      pid:     600
      type:    label
      cells:   (1,7,1,1)
      text:    "Missions:"
   },

   ctrl: {
      id:      111
      pid:     600
      type:    label
      cells:   (1,8,1,1)
      text:    "Kills:"
   }

   ctrl: {
      id:      112
      pid:     600
      type:    label
      cells:   (1,9,1,1)
      text:    "Losses:"
   }

   ctrl: {
      id:      113
      pid:     600
      type:    label
      cells:   (1,10,1,1)
      text:    "Points:"
   }

   ctrl: {
      id:      108
      pid:     600
      type:    label
      cells:   (1,11,1,1)
      text:    "Rank:"
   }

   ctrl: {
      id:      125
      pid:     600
      type:    label
      cells:   (1,13,1,1)
      cell_insets:   (0,0,0,5)
      text:    "Medals:"
   }

   defctrl: {
      style:         0x02,
      scroll_bar:    0,
      single_line:   true,
      back_color:    ( 41,  41,  41),
      border_color:  ( 92,  92,  92),
      single_line:   true,
      transparent:   false
      cell_insets:   (0,0,0,5)
   },

   ctrl: {
      id:      201,
      pid:     600
      type:    edit,
      cells:   (2,1,1,1)
   },

   ctrl: {
      id:      202,
      pid:     600
      type:    edit,
      cells:   (2,2,1,1)
      password: "*",
   },

   ctrl: {
      id:      203,  // squad
      pid:     600
      type:    edit,
      cells:   (2,3,1,1)
   },

   ctrl: {
      id:      204,  // sig
      pid:     600
      type:    edit,
      cells:   (2,4,1,1)
   },

   ctrl: {
      id:      205,  // create date
      pid:     600
      type:    label,
      cells:   (2,5,1,1)
   },

   ctrl: {
      id:      207,  // flight time
      pid:     600
      type:    label,
      cells:   (2,6,1,1)
   },

   ctrl: {
      id:      208,  // missions
      pid:     600
      type:    label,
      cells:   (2,7,1,1)
   },

   ctrl: {
      id:      209,  // kills
      pid:     600
      type:    label,
      cells:   (2,8,1,1)
   },

   ctrl: {
      id:      210,  // losses
      pid:     600
      type:    label,
      cells:   (2,9,1,1)
   },

   ctrl: {
      id:      211,  // points
      pid:     600
      type:    label,
      cells:   (2,10,1,1)
   },

   // rank name and insignia:

   ctrl: {
      id:      206,
      pid:     600
      type:    label,
      cells:   (2,11,1,1)
      align:   center,
      transparent: true,
      border: false,
   },

   ctrl: {
      id:      220,
      pid:     600
      type:    image,
      cells:   (2,12,1,1)
      align:   center
      transparent: true
      border:  false
      style:   0
   },

   // medal ribbon insignia:

   ctrl: {
      id:            330
      pid:           600
      type:          label
      transparent:   true
      cells:         (2,13,4,1)

      layout: {
      x_mins:     (85, 85, 85, 85, 85,  0),
      x_weights:  ( 1,  1,  1,  1,  1,  5),

      y_mins:     (24, 24, 24,  0),
      y_weights:  ( 1,  1,  1,  5)
      }
   }

   defctrl: {
      align:         center
      transparent:   true
      border:        false
      style:         0
      fixed_width:   82
      fixed_height:  21
   }

   ctrl: {
      id:      230,
      pid:     330
      type:    image,
      cells:   (0,0,1,1)
   },

   ctrl: {
      id:      231
      pid:     330
      type:    image
      cells:   (1,0,1,1)
   }

   ctrl: {
      id:      232,
      pid:     330
      type:    image
      cells:   (2,0,1,1)
   },

   ctrl: {
      id:      233,
      pid:     330
      type:    image
      cells:   (3,0,1,1)
   },

   ctrl: {
      id:      234,
      pid:     330
      type:    image
      cells:   (4,0,1,1)
   },

   ctrl: {
      id:      235,
      pid:     330
      type:    image
      cells:   (0,1,1,1)
   },

   ctrl: {
      id:      236,
      pid:     330
      type:    image
      cells:   (1,1,1,1)
   },

   ctrl: {
      id:      237,
      pid:     330
      type:    image
      cells:   (2,1,1,1)
   },

   ctrl: {
      id:      238,
      pid:     330
      type:    image
      cells:   (3,1,1,1)
   },

   ctrl: {
      id:      239,
      pid:     330
      type:    image
      cells:   (4,1,1,1)
   },

   ctrl: {
      id:      240,
      pid:     330
      type:    image
      cells:   (0,2,1,1)
   },

   ctrl: {
      id:      241,
      pid:     330
      type:    image
      cells:   (1,2,1,1)
   },

   ctrl: {
      id:      242,
      pid:     330
      type:    image
      cells:   (2,2,1,1)
   },

   ctrl: {
      id:      243,
      pid:     330
      type:    image
      cells:   (3,2,1,1)
   },

   ctrl: {
      id:      244,
      pid:     330
      type:    image
      cells:   (4,2,1,1)
   },

   // chat macro entries:

   defctrl: {
      align:         left
      transparent:   false
      border:        true
      style:         2  // white frame
      fixed_width:   0
      fixed_height:  0
   }

   ctrl: {
      id:      301,
      pid:     600
      type:    edit,
      cells:   (5,2,1,1)
   },

   ctrl: {
      id:      302,
      pid:     600
      type:    edit,
      cells:   (5,3,1,1)
   },

   ctrl: {
      id:      303,
      pid:     600
      type:    edit,
      cells:   (5,4,1,1)
   },

   ctrl: {
      id:      304,
      pid:     600
      type:    edit,
      cells:   (5,5,1,1)
   },

   ctrl: {
      id:      305,
      pid:     600
      type:    edit,
      cells:   (5,6,1,1)
   },

   ctrl: {
      id:      306,
      pid:     600
      type:    edit,
      cells:   (5,7,1,1)
   },

   ctrl: {
      id:      307,
      pid:     600
      type:    edit,
      cells:   (5,8,1,1)
   },

   ctrl: {
      id:      308,
      pid:     600
      type:    edit,
      cells:   (5,9,1,1)
   },

   ctrl: {
      id:      309,
      pid:     600
      type:    edit,
      cells:   (5,10,1,1)
   },

   ctrl: {
      id:      300,
      pid:     600
      type:    edit,
      cells:   (5,11,1,1)
   },

   defctrl: { transparent: true },

   ctrl: {
      id:      444
      pid:     600
      type:    label
      cells:   (4,1,2,1)
      text:    "Chat Macros:",
   },

   ctrl: {
      id:      401,
      pid:     600
      type:    label,
      cells:   (4,2,1,1)
      text:    "1",
   },

   ctrl: {
      id:      402,
      pid:     600
      type:    label,
      cells:   (4,3,1,1)
      text:    "2",
   },

   ctrl: {
      id:      403,
      pid:     600
      type:    label,
      cells:   (4,4,1,1)
      text:    "3",
   },

   ctrl: {
      id:      404,
      pid:     600
      type:    label,
      cells:   (4,5,1,1)
      text:    "4",
   },

   ctrl: {
      id:      405,
      pid:     600
      type:    label,
      cells:   (4,6,1,1)
      text:    "5",
   },

   ctrl: {
      id:      406,
      pid:     600
      type:    label,
      cells:   (4,7,1,1)
      text:    "6",
   },

   ctrl: {
      id:      407,
      pid:     600
      type:    label,
      cells:   (4,8,1,1)
      text:    "7",
   },

   ctrl: {
      id:      408,
      pid:     600
      type:    label,
      cells:   (4,9,1,1)
      text:    "8",
   },

   ctrl: {
      id:      409,
      pid:     600
      type:    label,
      cells:   (4,10,1,1)
      text:    "9",
   },

   ctrl: {
      id:      400,
      pid:     600
      type:    label,
      cells:   (4,11,1,1)
      text:    "0",
   },

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
   },

   ctrl: {
      id:            1
      type:          button
      text:          Apply
      cells:         (5,5,1,1)
   },

   ctrl: {
      id:            2
      type:          button
      text:          Cancel
      cells:         (6,5,1,1),
   },
}

