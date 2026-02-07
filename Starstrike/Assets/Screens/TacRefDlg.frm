FORM


//  Project:   Starshatter 4.5
//  File:      TacRefDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 80, 80, 10, 100, 30, 100, 10)
      x_weights:  ( 0,  1,  1,  0,   3,  0,   3,  0)

      y_mins:     (28, 25, 20, 42, 90, 10, 90, 45)
      y_weights:  ( 0,  0,  0,  0,  3,  1,  2,  0)
   },

   // background images:

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame3a
      cells:         (1,3,4,5),
      cell_insets:   (0,0,0,10),
      margins:       (48,80,48,48)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame3b
      cells:         (5,3,2,5),
      cell_insets:   (0,0,0,10),
      margins:       (80,48,48,48)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Tactical Reference",
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
      text:          "Ships"
      sticky:        true
      fixed_height:  19
   },

   ctrl: {
      id:            102,
      type:          button,
      cells:         (2,3,1,1)
      cell_insets:   (3,10,17,6)
      text:          "Weapons",
      sticky:        true
      fixed_height:  19
   }

   ctrl: {
      id:            301,
      type:          label,
      cells:         (4,3,1,1)
      cell_insets:   (2,10,17,0)
      text:          "Item Name"
      fore_color:    (255,255,255)
      back_color:    (32,32,32)
      style:         0x40
      transparent:   true
   }


   defctrl: {
      fore_color:    (255,255,255),
      standard_image:   "",
      activated_image:  "",
      transition_image: "",

      font:          Verdana
      style:         0
      scroll_bar:    0
      show_headings: false
      transparent:   false

      texture:       Panel
      margins:       (12,12,16,4)
   },

   ctrl: {
      id:            200
      type:          list
      cells:         (1,4,2,3)
      cell_insets:   (10,10,0,10)
      scroll_bar:    2
      selected_style: 2

      column:     {
         title:   "Name",
         width:   182,
         align:   left,
         sort:    0 },
   }


   ctrl: {
      id:            400
      type:          label
      cells:         (4,4,3,2)
      cell_insets:   (0,10,0,10)
   }

   ctrl: {
      id:            401
      type:          label
      cells:         (4,4,3,2)
      cell_insets:   (0,10,0,10)
      transparent:   true
   }

   ctrl: {
      id:            410
      type:          label
      cells:         (4,6,3,1)
      cell_insets:   (0,10,0,10)
   }

   ctrl: {
      id:            402
      type:          text
      cells:         (4,6,1,1)
      cell_insets:   (0,10,0,10)
      transparent:   true

      text: "Item stats go here..."
   }

   ctrl: {
      id:            403
      type:          text
      cells:         (5,6,2,1)
      cell_insets:   (0,10,0,10)
      transparent:   true

      text: "Item description goes here..."
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
   },

   ctrl: {
      id:            1
      type:          button
      text:          Close
      cells:         (6,7,1,1)
   },

}
