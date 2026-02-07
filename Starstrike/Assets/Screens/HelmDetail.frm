FORM


//  Project:   STARS
//  File:      HelmDetail.frm
//
//  John DiCamillo Software Consulting
//  Copyright © 1999. All Rights Reserved.


form: {
   rect:       (10, 345, 620, 127),

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "WepDetail_640.pcx",

   defctrl: {
      base_color: ( 92,  92,  92),
      back_color: ( 92,  92,  92),
      fore_color: (255, 255, 255),
      font:       GUI,
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
   },

   ctrl: {
      id:         100,
      type:       label,
      rect:       (15, 345, 180, 20),
      text:       "Helm Control",
      transparent: true
   },

   ctrl: {
      id:         201,
      type:       slider,
      rect:       (260, 365, 20, 100),
      active_color: (192,192,64),
      text:       "",
      style:      0x21,
      orientation: 1,
      active:     true,
      back_color: ( 92,  92,  92)
   },

   ctrl: {
      id:         202,
      type:       slider,
      rect:       (295, 365, 20, 100),
      active_color: (192,192,64),
      text:       "",
      style:      0x21,
      orientation: 1,
      active:     true,
      back_color: ( 92,  92,  92)
   },

   defctrl: {
      back_color:  (62,106,151),
      font:        GUI,
      sticky:      true,
      transparent: false
   },

   ctrl: {
      id:         251,
      type:       button,
      rect:       (365, 365, 75, 25),
      text:       "Manual"
   },

   ctrl: {
      id:         252,
      type:       button,
      rect:       (365, 392, 75, 25),
      text:       "Helm"
   },

   ctrl: {
      id:         253,
      type:       button,
      rect:       (365, 419, 75, 25),
      text:       "Nav"
   },

   ctrl: {
      id:         300,
      type:       label,
      rect:       (445, 345, 180, 20),
      text:       "Contact List",
      transparent: true
   },

   ctrl: {
      id:         301,
      type:       list,
      rect:       (445, 365, 180, 100),
      text:       "contact list",
      back_color: (21,21,21),
      font:       GUIsmall,
      scroll_bar: 2,
      style:      0x20
   },

   ctrl: {
      id:         400,
      type:       compass,
      rect:       (135, 365, 100, 100),
      font:       GUIsmall,
      transparent: true
   }
}








form: {
   rect:       (10, 465, 620, 127),

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "WepDetail_640.pcx",

   defctrl: {
      base_color: ( 92,  92,  92),
      back_color: ( 92,  92,  92),
      fore_color: (255, 255, 255),
      font:       GUI,
      bevel_width: 4,
      bevel_depth: 128,
      border: true,
      border_color: (0,0,0)
   },

   ctrl: {
      id:         100,
      type:       label,
      rect:       (15, 465, 180, 20),
      text:       "Helm Control",
      transparent: true
   },

   ctrl: {
      id:         201,
      type:       slider,
      rect:       (260, 485, 20, 100),
      active_color: (192,192,64),
      text:       "",
      style:      0x21,
      orientation: 1,
      active:     true,
      back_color: ( 92,  92,  92)
   },

   ctrl: {
      id:         202,
      type:       slider,
      rect:       (295, 485, 20, 100),
      active_color: (192,192,64),
      text:       "",
      style:      0x21,
      orientation: 1,
      active:     true,
      back_color: ( 92,  92,  92)
   },

   defctrl: {
      back_color:  (62,106,151),
      font:        GUI,
      sticky:      true,
      transparent: false
   },

   ctrl: {
      id:         251,
      type:       button,
      rect:       (365, 485, 75, 25),
      text:       "Manual"
   },

   ctrl: {
      id:         252,
      type:       button,
      rect:       (365, 512, 75, 25),
      text:       "Helm"
   },

   ctrl: {
      id:         253,
      type:       button,
      rect:       (365, 539, 75, 25),
      text:       "Nav"
   },

   ctrl: {
      id:         300,
      type:       label,
      rect:       (445, 465, 180, 20),
      text:       "Contact List",
      transparent: true
   },

   ctrl: {
      id:         301,
      type:       list,
      rect:       (445, 485, 180, 100),
      text:       "contact list",
      back_color: (21,21,21),
      font:       GUIsmall,
      scroll_bar: 2,
      style:      0x20
   },

   ctrl: {
      id:         400,
      type:       compass,
      rect:       (135, 485, 100, 100),
      font:       GUIsmall,
      transparent: true
   }
}

