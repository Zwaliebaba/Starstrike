FORM


//  Project:   STARS
//  File:      MessageDlg.frm
//
//  John DiCamillo Software Consulting
//  Copyright © 1997-2003. All Rights Reserved.


form: {
   rect:       (145, 115, 350, 250),
   screen_width:  640,
   screen_height: 480,

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "message.pcx",

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
      id:            100,
      type:          label,
      rect:          (160, 140, 320, 20),
      text:          "Message Title",
      transparent:   true,
      align:         center
   },


   ctrl: {
      id:            101,
      type:          label,
      rect:          (180, 170, 280, 120),
      text:          "Message Text",
      font:          Verdana,
      transparent:   true
   },

   ctrl: {
      id:            1,
      type:          button,
      rect:          (325, 310, 100, 25),
      back_color:    ( 62, 106, 151),
      text:          "Close"
   }
}






form: {
   rect:       (225, 175, 350, 250),
   screen_width:  800,
   screen_height: 600,

   base_color: ( 92,  92,  92),
   back_color: ( 92,  92,  92),
   fore_color: (255, 255, 255),
   font:       "GUI",

   texture:    "message.pcx",

   defctrl: {
      base_color: ( 92,  92,  92),
      back_color: ( 92,  92,  92),
      fore_color: (255, 255, 255),
      font:       GUI,
      bevel_width: 0,
      transparent:   true,
      align:         center
   },

   ctrl: {
      id:            100,
      type:          label,
      rect:          (240, 230, 320, 20),
      text:          "Message Title",
   },

   ctrl: {
      id:            101,
      type:          label,
      rect:          (260, 280, 280, 120),
      text:          "Message Text",
      font:          Verdana,
   },

   defctrl: { 
      transparent:   false,
      font:          Terminal,
      fore_color:    (5,5,5),
      align:         left,
   },

   ctrl: {
      id:               1,
      type:             button,
      rect:             (405, 380, 130, 17),
      text:             Close,
      standard_image:   grn_130x17_0,
      activated_image:  grn_130x17_1,
      transition_image: grn_130x17_2,
   },
}
