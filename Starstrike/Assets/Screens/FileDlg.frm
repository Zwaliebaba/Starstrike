FORM


//  Project:   Starshatter 4.5
//  File:      CmdMsgDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   rect:       (0,0,400,380)
   back_color: (0,0,0)
   fore_color: (255,255,255)
   font:       Limerick12

   texture:    "Message.pcx"
   margins:    (50,40,48,40)

   layout: {
      x_mins:     (20, 50, 100, 100, 20)
      x_weights:  ( 0,  0,   1,   1,  0)

      y_mins:     (44,  30, 10, 25, 80, 35)
      y_weights:  ( 0,   0,  0,  0,  1,  0)
   }

   defctrl: {
      fore_color:    (255, 255, 255)
      font:          Limerick12
      transparent:   true
   }

   ctrl: {
      id:            100,
      type:          label,
      align:         center
      cells:         (1,1,3,1)
      font:          Limerick18
      text:          "Save Game to File",
   },

   defctrl: {
      font:          Verdana,
      transparent:   false,
      style:         0,
   },


   ctrl: {
      id:            110,
      type:          label,
      cells:         (1,3,1,1)
      text:          "Name: ",
      transparent:   true,
   },

   ctrl: {
      id:            111,
      type:          label,
      cells:         (1,4,1,1)
      text:          "Files: ",
      transparent:   true,
   },

   defctrl: {
      back_color:    (41, 41, 41),
      style:         0x02,
      scroll_bar:    0,
   },

   ctrl: {
      id:            200
      type:          edit
      cells:         (2,3,2,1)
      cell_insets:   (10,0,0,0)
      fixed_height:  19
   },

   ctrl: {
      id:            201
      type:          list
      cells:         (2,4,2,1)
      cell_insets:   (10,0,0,20)
      scroll_bar:    2,

      column:     {
         title:   Files,
         width:   200,
         align:   left,
         sort:    0 }
   },


   defctrl: {
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (10,0,0,16)
      transparent:      false
      fixed_height:     19
   }

   ctrl: {
      id:               1
      type:             button
      cells:            (2,5,1,1)
      text:             Save
   },

   ctrl: {
      id:               2
      type:             button
      cells:            (3,5,1,1)
      text:             Cancel
   },
}
