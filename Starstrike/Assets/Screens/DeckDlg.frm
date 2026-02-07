FORM


//  Project:   STARS
//  File:      DeckWin.frm
//
//  John DiCamillo Software Consulting
//  Copyright © 1998. All Rights Reserved.


form: {
   rect:       (  0,   0, 640, 480),
   centered:   true,

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "DeckWin.pcx",

   defctrl: {
      base_color: ( 92,  92,  92),
      back_color: ( 92,  92,  92),
      fore_color: (255, 255, 255),
      font:       "GUI",
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
   },

   ctrl: {
      id:         100,
      type:       map,
      rect:       (390, 66, 237, 237),
      back_color: ( 32,  32,  32),
      text:       "System"
   },

   defctrl: {
      back_color: ( 62, 106, 151),
      sticky: true
   },


   ctrl: {
      id:         101,
      type:       button,
      rect:       ( 11,  36, 68, 25),
      text:       "Bay 1"
   },

   ctrl: {
      id:         102,
      type:       button,
      rect:       ( 82,  36, 68, 25),
      text:       "Bay 2"
   },

   ctrl: {
      id:         103,
      type:       button,
      rect:       (153,  36, 68, 25),
      text:       "Bay 3"
   },

   ctrl: {
      id:         104,
      type:       button,
      rect:       (224,  36, 68, 25),
      text:       "Bay 4"
   },

   ctrl: {
      id:         201,
      type:       button,
      rect:       (295,  76, 72, 25),
      text:       "Launch"
   },

   ctrl: {
      id:         202,
      type:       button,
      rect:       (295, 103, 72, 25),
      text:       "Recover"
   },


   defctrl: {
      back_color: (62,106,151),
      sticky:     false
   },

   ctrl: {
      id:         301,
      type:       button,
      rect:       (520, 36, 25, 25),
      text:       "+"
   },

   ctrl: {
      id:         302,
      type:       button,
      rect:       (549, 36, 25, 25),
      text:       "-"
   },

   ctrl: {
      id:         303,
      type:       button,
      rect:       (577, 36, 25, 25),
      text:       "<"
   },

   ctrl: {
      id:         304,
      type:       button,
      rect:       (605, 36, 25, 25),
      text:       ">"
   },

   // Package controls:

   ctrl: {
      id:         401,
      type:       button,
      rect:       ( 35, 317, 72, 25),
      text:       "Package"
   },

   ctrl: {
      id:         402,
      type:       button,
      rect:       (111, 317, 72, 25),
      text:       "Launch"
   },

   ctrl: {
      id:         403,
      type:       button,
      rect:       (187, 317, 72, 25),
      text:       "Scramble"
   },

   ctrl: {
      id:         404,
      type:       button,
      rect:       (263, 317, 72, 25),
      text:       "Cancel"
   },


   // Order and Loadout controls:

   defctrl: {
      sticky:     true
   },

   ctrl: {
      id:         501,
      type:       button,
      rect:       (390, 327, 115, 25),
      text:       "Vector"
   },

   ctrl: {
      id:         502,
      type:       button,
      rect:       (390, 354, 115, 25),
      text:       "Patrol"
   },

   ctrl: {
      id:         503,
      type:       button,
      rect:       (390, 381, 115, 25),
      text:       "Engage"
   },

   ctrl: {
      id:         504,
      type:       button,
      rect:       (390, 408, 115, 25),
      text:       "Defend"
   },

   ctrl: {
      id:         505,
      type:       button,
      rect:       (390, 435, 115, 25),
      text:       "Escort"
   },

   ctrl: {
      id:         601,
      type:       button,
      rect:       (510, 327, 115, 25),
      text:       "Intercept 1"
   },

   ctrl: {
      id:         602,
      type:       button,
      rect:       (510, 354, 115, 25),
      text:       "Intercept 2"
   },

   ctrl: {
      id:         603,
      type:       button,
      rect:       (510, 381, 115, 25),
      text:       "Strike"
   },

   // bay contents:

   defctrl: {
      back_color: (32,  32, 32),
      fore_color: (53, 159, 67),
      font:       "GUIsmall",
      transparent: true
   },

   ctrl: { id: 1001, type: label, rect: ( 20, 175, 16, 18) },
   ctrl: { id: 1002, type: label, rect: ( 20, 191, 16, 18) },
   ctrl: { id: 1003, type: label, rect: ( 20, 207, 16, 18) },
   ctrl: { id: 1004, type: label, rect: ( 20, 223, 16, 18) },
   ctrl: { id: 1005, type: label, rect: ( 20, 239, 16, 18) },
   ctrl: { id: 1006, type: label, rect: ( 20, 255, 16, 18) },
   ctrl: { id: 1007, type: label, rect: ( 20, 271, 16, 18) },
   ctrl: { id: 1008, type: label, rect: ( 20, 287, 16, 18) },

   defctrl: {
      fore_color: (255, 255, 255)
   },

   ctrl: { id: 1101, type: label, rect: ( 40, 175, 80, 18) },
   ctrl: { id: 1102, type: label, rect: ( 40, 191, 80, 18) },
   ctrl: { id: 1103, type: label, rect: ( 40, 207, 80, 18) },
   ctrl: { id: 1104, type: label, rect: ( 40, 223, 80, 18) },
   ctrl: { id: 1105, type: label, rect: ( 40, 239, 80, 18) },
   ctrl: { id: 1106, type: label, rect: ( 40, 255, 80, 18) },
   ctrl: { id: 1107, type: label, rect: ( 40, 271, 80, 18) },
   ctrl: { id: 1108, type: label, rect: ( 40, 287, 80, 18) },

   ctrl: { id: 1201, type: label, rect: (120, 175, 80, 18) },
   ctrl: { id: 1202, type: label, rect: (120, 191, 80, 18) },
   ctrl: { id: 1203, type: label, rect: (120, 207, 80, 18) },
   ctrl: { id: 1204, type: label, rect: (120, 223, 80, 18) },
   ctrl: { id: 1205, type: label, rect: (120, 239, 80, 18) },
   ctrl: { id: 1206, type: label, rect: (120, 255, 80, 18) },
   ctrl: { id: 1207, type: label, rect: (120, 271, 80, 18) },
   ctrl: { id: 1208, type: label, rect: (120, 287, 80, 18) },

   ctrl: { id: 1301, type: label, rect: (200, 175, 80, 18) },
   ctrl: { id: 1302, type: label, rect: (200, 191, 80, 18) },
   ctrl: { id: 1303, type: label, rect: (200, 207, 80, 18) },
   ctrl: { id: 1304, type: label, rect: (200, 223, 80, 18) },
   ctrl: { id: 1305, type: label, rect: (200, 239, 80, 18) },
   ctrl: { id: 1306, type: label, rect: (200, 255, 80, 18) },
   ctrl: { id: 1307, type: label, rect: (200, 271, 80, 18) },
   ctrl: { id: 1308, type: label, rect: (200, 287, 80, 18) },

   ctrl: { id: 1401, type: label, rect: (280, 175, 80, 18) },
   ctrl: { id: 1402, type: label, rect: (280, 191, 80, 18) },
   ctrl: { id: 1403, type: label, rect: (280, 207, 80, 18) },
   ctrl: { id: 1404, type: label, rect: (280, 223, 80, 18) },
   ctrl: { id: 1405, type: label, rect: (280, 239, 80, 18) },
   ctrl: { id: 1406, type: label, rect: (280, 255, 80, 18) },
   ctrl: { id: 1407, type: label, rect: (280, 271, 80, 18) },
   ctrl: { id: 1408, type: label, rect: (280, 287, 80, 18) }

}

