FORM


//  Project:   Starshatter 4.5
//  File:      MsnObjDlg.frm
//
//  Destroyer Studios LLC
//  Copyright © 1997-2004. All Rights Reserved.


form: {
   back_color: (  0,   0,   0),
   fore_color: (255, 255, 255),

   texture:    "Frame1.pcx",
   margins:    (1,1,64,8),

   layout: {
      x_mins:     (10, 100,  20, 100, 100, 10),
      x_weights:  ( 0, 0.2, 0.4, 0.2, 0.2,  0),

      y_mins:     (28, 30,  10,  90, 24, 60, 45),
      y_weights:  ( 0,  0,   0,   0,  0,  1,  0)
   },

   // background images:

   ctrl: {
      id:            9990
      type:          background
      texture:       Frame4a
      cells:         (1,3,4,1),
      cell_insets:   (0,0,0,0),
      margins:       (2,2,16,16)
      hide_partial:  false
   }

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame2a,
      cells:         (1,4,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (2,32,40,32)
      hide_partial:  false
   }

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame2b,
      cells:         (3,4,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (0,40,40,32)
      hide_partial:  false
   }

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Mission Briefing",
      align:         left,
      font:          Limerick18,
      fore_color:    (255,255,255),
      transparent:   true,
      cells:         (1,1,3,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false
   },

   // tabs:

   ctrl: {
      id:            999
      type:          panel
      transparent:   true
      cells:         (1,4,4,1)
      cell_insets:   (0,0,0,0)
      hide_partial:  false

      layout: {
         x_mins:     (80, 80, 80, 80,  5)
         x_weights:  ( 1,  1,  1,  1, 15)

         y_mins:     (24),
         y_weights:  ( 0)
      }
   }

   defctrl: {
      align:            left,
      font:             Limerick12,
      fore_color:       (255, 255, 255),
      standard_image:   BlueTab_0,
      activated_image:  BlueTab_1,
      sticky:           true,
      bevel_width:      6,
      margins:          (8,8,0,0),
      cell_insets:      (0,4,0,0)
   },

   ctrl: {
      id:            900
      pid:           999
      type:          button
      text:          SIT
      cells:         (0,0,1,1)
   }

   ctrl: {
      id:            901
      pid:           999
      type:          button
      text:          PKG
      cells:         (1,0,1,1)
   }

   ctrl: {
      id:            902
      pid:           999
      type:          button
      text:          MAP
      cells:         (2,0,1,1)
   }

   ctrl: {
      id:            903
      pid:           999
      type:          button
      text:          WEP
      cells:         (3,0,1,1)
   }

   // info panel:

   ctrl: {
      id:               700
      type:             panel
      transparent:      false

      texture:          Panel
      margins:          (12,12,12,0),

      cells:            (1,3,4,1),
      cell_insets:      (10,10,12,10)

      layout: {
         x_mins:     ( 20, 60, 100, 60, 100, 20)
         x_weights:  (  0,  0,   1,  0,   1,  0)

         y_mins:     (  0,  20,  15,  15,  0)
         y_weights:  (  1,   0,   0,   0,  1)
      }
   }

   defctrl: {
      align:            left
      bevel_width:      0
      font:             Verdana
      fore_color:       (255, 255, 255)
      standard_image:   ""
      activated_image:  ""
      transparent:      true
      margins:          (0,0,0,0)
      cell_insets:      (0,0,0,0)
      text_insets:      (1,1,1,1)
   },

   ctrl: {
      id:            200
      pid:           700
      type:          label
      cells:         (1,1,2,1)
      text:          "title goes here",
      fore_color:    (255, 255, 128)
      font:          Limerick12
   },

   ctrl: {
      id:            201
      pid:           700
      type:          label
      cells:         (1,2,1,1)
      text:          "System:"
   },

   ctrl: {
      id:            202
      pid:           700
      type:          label
      cells:         (2,2,1,1)
      text:          "alpha"
   },

   ctrl: {
      id:            203
      pid:           700
      type:          label
      cells:         (1,3,1,1)
      text:          "Sector:"
   },

   ctrl: {
      id:            204
      pid:           700
      type:          label
      cells:         (2,3,1,1)
      text:          "bravo"
   },


   ctrl: {
      id:            206
      pid:           700
      type:          label
      cells:         (4,1,1,1)
      text:          "Day 7 11:32:04"
      align:         right
   },


   // main panel:

   ctrl: {
      id:               800
      type:             panel
      transparent:      false

      texture:          Panel
      margins:          (12,12,12,0),

      cells:            (1,5,4,2)
      cell_insets:      (10,10,12,54)

      layout: {
         x_mins:     ( 20,  80, 120,  30, 30, 30, 30, 5, 30, 30, 30, 30, 10, 20)
         x_weights:  (  0,   0,   1,   1,  1,  1,  1, 0,  1,  1,  1,  1,  2,  0)

         y_mins:     ( 15,  20,  20,  20, 30, 15, 20,  20,20,20,20, 20,20,20,20, 20)
         y_weights:  (  0,   0,   0,   0,  1,  0,  0,   0, 0, 0, 0,  0, 0, 0, 0,  0)
      }
   }


   defctrl: { 
      pid:           800
      transparent:   true
      font:          Limerick12
   }

   ctrl: {
      id:            604
      type:          list
      cells:         (3,2,9,3)
      font:          Verdana

      back_color:    (41,41,41)
      transparent:   false

      style:            0x02
      item_style:       0x00
      selected_style:   0x02
      scroll_bar:       2
      leading:          2
      show_headings:    true

      column: {
         title:   NAME,
         width:   160,
         align:   left,
         sort:    1,
      },

      column: {
         title:   WEIGHT,
         width:   72,
         align:   right,
         sort:    1,
      },
   },

   ctrl: { id:  90  type: label  cells: (3,1,9,1)  text: "Standard Loadouts"  }
   ctrl: { id:  91  type: label  cells: (1,1,1,1)  text: "Element:"           }
   ctrl: { id: 601  type: label  cells: (2,1,1,1)                             }
   ctrl: { id:  92  type: label  cells: (1,2,1,1)  text: "Type:"              }
   ctrl: { id: 602  type: label  cells: (2,2,1,1)                             }
   ctrl: { id:  93  type: label  cells: (1,3,1,1)  text: "Weight:"            }
   ctrl: { id: 603  type: label  cells: (2,3,1,1)                             }
   ctrl: { id:  94  type: label  cells: (1,6,2,1)  text: "Custom Loadouts"    }

   defctrl: { font: Verdana  align: left },

   ctrl: { id: 401  type: label  cells: ( 3,6,1,1) }
   ctrl: { id: 402  type: label  cells: ( 4,6,1,1) }
   ctrl: { id: 403  type: label  cells: ( 5,6,1,1) }
   ctrl: { id: 404  type: label  cells: ( 6,6,1,1) }

   ctrl: { id: 405  type: label  cells: ( 8,6,1,1) }
   ctrl: { id: 406  type: label  cells: ( 9,6,1,1) }
   ctrl: { id: 407  type: label  cells: (10,6,1,1) }
   ctrl: { id: 408  type: label  cells: (11,6,1,1) }

   defctrl: { transparent: true  align: left },

   ctrl: {
      id:            500
      type:          label
      cells:         (1,7,2,1)
      text:          "Weapon 1"
   }

   ctrl: {
      id:            510
      type:          label
      cells:         (1,8,2,1)
      text:          "Weapon 2"
   }

   ctrl: {
      id:            520
      type:          label
      cells:         (1,9,2,1)
      text:          "Weapon 3"
   }

   ctrl: {
      id:            530
      type:          label
      cells:         (1,10,2,1)
      text:          "Weapon 4"
   }

   ctrl: {
      id:            540
      type:          label
      cells:         (1,11,2,1)
      text:          "Weapon 5"
   }

   ctrl: {
      id:            550
      type:          label
      cells:         (1,12,2,1)
      text:          "Weapon 6"
   }

   ctrl: {
      id:            560
      type:          label
      cells:         (1,13,2,1)
      text:          "Weapon 7"
   }

   ctrl: {
      id:            570
      type:          label
      cells:         (1,14,2,1)
      text:          "Weapon 8"
   }

   defctrl: {
      transparent:      false
      align:            center
      standard_image:   Button17x17_0
      activated_image:  Button17x17_1
      transition_image: Button17x17_2
      fixed_width:      19
      fixed_height:     19
   }

   ctrl: { id: 501  type: button  cells: ( 3,7,1,1)  picture: "LED0.pcx" } 
   ctrl: { id: 502  type: button  cells: ( 4,7,1,1)  picture: "LED1.pcx" } 
   ctrl: { id: 503  type: button  cells: ( 5,7,1,1) }
   ctrl: { id: 504  type: button  cells: ( 6,7,1,1) }

   ctrl: { id: 505  type: button  cells: ( 8,7,1,1) }
   ctrl: { id: 506  type: button  cells: ( 9,7,1,1) }
   ctrl: { id: 507  type: button  cells: (10,7,1,1) }
   ctrl: { id: 508  type: button  cells: (11,7,1,1) }

   ctrl: { id: 511  type: button  cells: ( 3,8,1,1) } 
   ctrl: { id: 512  type: button  cells: ( 4,8,1,1) } 
   ctrl: { id: 513  type: button  cells: ( 5,8,1,1) }
   ctrl: { id: 514  type: button  cells: ( 6,8,1,1) }

   ctrl: { id: 515  type: button  cells: ( 8,8,1,1) }
   ctrl: { id: 516  type: button  cells: ( 9,8,1,1) }
   ctrl: { id: 517  type: button  cells: (10,8,1,1) }
   ctrl: { id: 518  type: button  cells: (11,8,1,1) }

   ctrl: { id: 521  type: button  cells: ( 3,9,1,1) } 
   ctrl: { id: 522  type: button  cells: ( 4,9,1,1) } 
   ctrl: { id: 523  type: button  cells: ( 5,9,1,1) }
   ctrl: { id: 524  type: button  cells: ( 6,9,1,1) }

   ctrl: { id: 525  type: button  cells: ( 8,9,1,1) }
   ctrl: { id: 526  type: button  cells: ( 9,9,1,1) }
   ctrl: { id: 527  type: button  cells: (10,9,1,1) }
   ctrl: { id: 528  type: button  cells: (11,9,1,1) }

   ctrl: { id: 531  type: button  cells: ( 3,10,1,1) } 
   ctrl: { id: 532  type: button  cells: ( 4,10,1,1) } 
   ctrl: { id: 533  type: button  cells: ( 5,10,1,1) }
   ctrl: { id: 534  type: button  cells: ( 6,10,1,1) }

   ctrl: { id: 535  type: button  cells: ( 8,10,1,1) }
   ctrl: { id: 536  type: button  cells: ( 9,10,1,1) }
   ctrl: { id: 537  type: button  cells: (10,10,1,1) }
   ctrl: { id: 538  type: button  cells: (11,10,1,1) }

   ctrl: { id: 541  type: button  cells: ( 3,11,1,1) } 
   ctrl: { id: 542  type: button  cells: ( 4,11,1,1) } 
   ctrl: { id: 543  type: button  cells: ( 5,11,1,1) }
   ctrl: { id: 544  type: button  cells: ( 6,11,1,1) }

   ctrl: { id: 545  type: button  cells: ( 8,11,1,1) }
   ctrl: { id: 546  type: button  cells: ( 9,11,1,1) }
   ctrl: { id: 547  type: button  cells: (10,11,1,1) }
   ctrl: { id: 548  type: button  cells: (11,11,1,1) }

   ctrl: { id: 551  type: button  cells: ( 3,12,1,1) } 
   ctrl: { id: 552  type: button  cells: ( 4,12,1,1) } 
   ctrl: { id: 553  type: button  cells: ( 5,12,1,1) }
   ctrl: { id: 554  type: button  cells: ( 6,12,1,1) }

   ctrl: { id: 555  type: button  cells: ( 8,12,1,1) }
   ctrl: { id: 556  type: button  cells: ( 9,12,1,1) }
   ctrl: { id: 557  type: button  cells: (10,12,1,1) }
   ctrl: { id: 558  type: button  cells: (11,12,1,1) }

   ctrl: { id: 561  type: button  cells: ( 3,13,1,1) } 
   ctrl: { id: 562  type: button  cells: ( 4,13,1,1) } 
   ctrl: { id: 563  type: button  cells: ( 5,13,1,1) }
   ctrl: { id: 564  type: button  cells: ( 6,13,1,1) }

   ctrl: { id: 565  type: button  cells: ( 8,13,1,1) }
   ctrl: { id: 566  type: button  cells: ( 9,13,1,1) }
   ctrl: { id: 567  type: button  cells: (10,13,1,1) }
   ctrl: { id: 568  type: button  cells: (11,13,1,1) }

   ctrl: { id: 571  type: button  cells: ( 3,14,1,1) } 
   ctrl: { id: 572  type: button  cells: ( 4,14,1,1) } 
   ctrl: { id: 573  type: button  cells: ( 5,14,1,1) }
   ctrl: { id: 574  type: button  cells: ( 6,14,1,1) }

   ctrl: { id: 575  type: button  cells: ( 8,14,1,1) }
   ctrl: { id: 576  type: button  cells: ( 9,14,1,1) }
   ctrl: { id: 577  type: button  cells: (10,14,1,1) }
   ctrl: { id: 578  type: button  cells: (11,14,1,1) }



/***
   ctrl: {
      id:            300
      pid:           800
      type:          image
      cells:         (13,2,1,3)
      font:          GUI
      transparent:   true
   },

   ctrl: {
      id:            301
      pid:           800
      type:          label
      cells:         (13,5,1,3)
      text:          "Player Desc"
      align:         center
      single_line:   true
      font:          Limerick12
   }
***/

   // ok and cancel buttons:

   defctrl: {
      pid:              0
      align:            left
      font:             Limerick12
      fore_color:       (0,0,0)
      standard_image:   Button17_0
      activated_image:  Button17_1
      transition_image: Button17_2
      transparent:      false
      bevel_width:      6
      margins:          (3,18,0,0)
      cell_insets:      (0,10,0,0)
      fixed_width:      0
      fixed_height:     19
   },

   ctrl: {
      id:            1
      type:          button
      text:          Accept
      cells:         (3,6,1,1)
   },

   ctrl: {
      id:            2
      type:          button
      text:          Cancel
      cells:         (4,6,1,1),
   },

}
