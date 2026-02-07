FORM


//  Project:   Starshatter 4.5
//  File:      MenuDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),
   font:       Limerick12,

   layout: {
      x_mins:     (20, 20, 28, 180, 20),
      x_weights:  ( 0,  1,  0,  0,  0),

      y_mins:     (60, 60,  40, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30),
      y_weights:  ( 1,  1,   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1)
   },

   // background images:

   defctrl: {
      fore_color:       (4,4,4),
      cell_insets:      (0,0,0,0)
   },

   ctrl: {
      id:            300
      type:          background
      texture:       MenuDlg1
      cells:         (0,0,5,1)
      margins:       (248,2,2,32)
      hide_partial:  false
   },

   ctrl: {
      id:            301
      type:          background
      texture:       Plasma
      cells:         (0,1,5,1)
      margins:       (0,896,0,0)
      hide_partial:  false
   },

   ctrl: {
      id:            302
      type:          background
      texture:       MenuDlg2
      cells:         (0,2,5,12)
      margins:       (2,248,32,2)
      hide_partial:  false
   },

   ctrl: {
      id:            100,
      type:          label,
      align:         right,
      font:          Verdana,
      transparent:   true,
      cells:         (3,2,1,1)
      cell_insets:   (0,0,30,5)
      hide_partial:  false
   },


   // buttons:

   defctrl: {
      align:            left,
      font:             Limerick12,
      fore_color:       (0,0,0),
      standard_image:   Button23_0,
      activated_image:  Button23_1,
      transition_image: Button23_2,
      bevel_width:      6,
      margins:          (3,18,0,0),
      cell_insets:      (0,0,0,5)
   },


   ctrl: {
      id:            120,
      type:          button,
      text:          "Start",
      alt:           "Start a new game, or resume your current game",
      cells:         (3,3,1,1)
   },

   ctrl: {
      id:            101,
      type:          button,
      text:          "Campaign",
      alt:           "Start a new dynamic campaign, or load a saved game",
      cells:         (3,4,1,1),
   },

   ctrl: {
      id:            102,
      type:          button,
      text:          "Mission",
      alt:           "Play or create a scripted mission exercise",
      cells:         (3,5,1,1)
   },

   ctrl: {
      id:            104,
      type:          button,
      text:          "Multiplayer",
      alt:           "Start or join a multiplayer scenario",
      cells:         (3,6,1,1)
   },

   ctrl: {
      id:            103,
      type:          button,
      text:          "Logbook",
      alt:           "Manage your logbook and player preferences",
      cells:         (3,7,1,1)
   },

   ctrl: {
      id:            111,
      type:          button,
      text:          "Options",
      alt:           "Audio, Video, Gameplay, Control, and Mod configuration options",
      cells:         (3,8,1,1)
   },

   ctrl: {
      id:            116,
      type:          button,
      text:          "Tac Reference",
      alt:           "View ship and weapon stats and mission roles",
      cells:         (3,9,1,1)
   },

   ctrl: {
      id:            114,
      type:          button,
      text:          "Exit",
      alt:           "Exit Starshatter and return to Windows",
      cells:         (3,10,1,1)
   },

   ctrl: {
      id:            202,
      type:          label,
      align:         center,
      font:          Verdana,
      transparent:   true,
      cells:         (1,11,3,1)
   }


}
