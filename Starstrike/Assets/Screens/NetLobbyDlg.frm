FORM


//  Project:   Starshatter 4.5
//  File:      NetLobbyDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 150, 100, 50, 100, 100, 10)
      x_weights:  ( 0,   1,   1,  1,   1,   1,  0)

      y_mins:     (28, 25, 20, 20, 20, 25, 50, 25, 50, 25, 45)
      y_weights:  ( 0,  0,  0,  0,  0,  0,  1,  0,  2,  0,  0)
   },

   // background images:

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame3a
      cells:         (1,3,3,8),
      cell_insets:   (0,0,0,10),
      margins:       (48,80,48,48)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame3b
      cells:         (4,3,2,8),
      cell_insets:   (0,0,0,10),
      margins:       (80,48,48,48)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Multiplayer Game Lobby",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,3,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   },


   // main panel:

   defctrl: {
      font:          Limerick12
      fore_color:    (255,255,255)
      back_color:    ( 41, 41, 41)
      cell_insets:   (10,10,0,0)
      transparent:   true
      style:         0x40
   }

   ctrl: {
      id:            101
      type:          label
      cells:         (1,4,2,1)
      text:          Missions
   }

   ctrl: {
      id:            102
      type:          label
      cells:         (1,7,1,1)
      text:          Players
   }

   ctrl: {
      id:            103
      type:          label
      cells:         (2,7,3,1)
      text:          Chat
   }


   defctrl: {
      font:          Verdana
      transparent:   false
      style:         0x02
      cell_insets:   (10,10,0,5)
      texture:       Panel
      margins:       (12,12,12,0)

      border_color:  (192, 192, 192)
      active_color:  ( 92,  92,  92)
      show_headings: true
      simple:        true
      text_align:    left
      fixed_height:  0
      scroll_bar:    2
   }

   ctrl: {
      id:            200,
      type:          combo,
      cells:         (1,5,2,1)
      fixed_height:  18
   },

   ctrl: {
      id:            201,
      type:          list,
      cells:         (1,6,2,1)

      column:     {
         title:   Missions,
         width:   100,
         align:   left,
         sort:    3 },	// sort never
   },

   ctrl: {
      id:            202,
      type:          label,
      cells:         (3,5,3,2)
      scroll_bar:    0,
      text:          "" // description
   },

   ctrl: {
      id:            210,
      type:          list,
      cells:         (1,8,1,2)

      column:     {
         title:   "HOST",
         width:   40,
         align:   center,
         sort:    3 },	// sort never

      column:     {
         title:   "PLAYER",
         width:   100,
         align:   left,
         sort:    3 },	// sort never
   },

   ctrl: {
      id:            211,
      type:          list,
      cells:         (2,8,4,1)

      column:     {
         title:   NAME,
         width:   100,
         align:   left,
         sort:    0 },

      column:     {
         title:   "CHAT MESSAGE",
         width:   250,
         align:   left,
         sort:    0 },
   },

   ctrl: {
      id:            212,
      type:          edit,
      cells:         (2,9,4,1)
      scroll_bar:    0
      fixed_height:  18
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
      cell_insets:      (5,5,0,0)
      fixed_height:     19
      fixed_width:      0
      pid:              0
   },

   ctrl: {
      id:            1
      type:          button
      text:          Accept
      cells:         (4,10,1,1)
   },

   ctrl: {
      id:            2
      type:          button
      text:          Cancel
      cells:         (5,10,1,1)
   },

}
