FORM


//  Project:   Starshatter 4.5
//  File:      MenuDlg.frm
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

      y_mins:     (28, 30,  20,  24, 60, 45),
      y_weights:  ( 0,  0,   0,   0,  1,  0)
   },

   // background images:

   ctrl: {
      id:            9991,
      type:          background,
      texture:       Frame2a,
      cells:         (1,3,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (2,32,40,32)
      hide_partial:  false
   },

   ctrl: {
      id:            9992,
      type:          background,
      texture:       Frame2b,
      cells:         (3,3,2,3),
      cell_insets:   (0,0,0,10),
      margins:       (0,40,40,32)
      hide_partial:  false
   },

   // title:

   ctrl: {
      id:            10,
      type:          label,
      text:          "Options",
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
      id:               900
      type:             panel
      transparent:      true
      cells:            (1,3,4,1)
      layout: {
         x_mins:     (100, 100, 100, 100, 100, 0),
         x_weights:  (0.2, 0.2, 0.2, 0.2, 0.2, 1),

         y_mins:     (24),
         y_weights:  ( 1)
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
      id:            901
      pid:           900
      type:          button
      text:          Video
      cells:         (0,0,1,1)
   }

   ctrl: {
      id:            902
      pid:           900
      type:          button
      text:          Audio
      cells:         (1,0,1,1)
   }

   ctrl: {
      id:            903
      pid:           900
      type:          button
      text:          Controls
      cells:         (2,0,1,1)
   }

   ctrl: {
      id:            904
      pid:           900
      type:          button
      text:          Gameplay
      cells:         (3,0,1,1)
   }

   // main panel:

   ctrl: {
      id:               300
      type:             panel
      transparent:      false //true

      texture:          Panel
      margins:          (12,12,12,0),

      cells:            (1,4,4,2)
      cell_insets:      (10,10,12,54)

      layout: {
         x_mins:     ( 20, 100, 100,  20, 100, 100,  20)
         x_weights:  (0.2, 0.3, 0.3, 0.2, 0.3, 0.3, 0.2)

         y_mins:     ( 20,  30,  30,  30,  30,  30,  30,  30,  30,  30,  20)
         y_weights:  (0.3,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0.7)
      }
   }

   defctrl: {
      font:             Verdana
      fore_color:       (255, 255, 255)

      standard_image:   ""
      activated_image:  ""
      sticky:           false

      active_color:     ( 92,  92,  92)
      back_color:       ( 41,  41,  41)
      base_color:       ( 92,  92,  92)
      border_color:     (192, 192, 192)

      border:           true
      simple:           true
      bevel_width:      3
      text_align:       left
      transparent:      true
   }

   ctrl: {
      id:               101
      pid:              300
      type:             label
      text:             "Flight Model:"
      cells:            (1,1,1,1)
   }

   ctrl: {
      id:               111
      pid:              300
      type:             label
      text:             "Flying Start:"
      cells:            (1,2,1,1)
   }

   ctrl: {
      id:               102
      pid:              300
      type:             label
      text:             "Landings:"
      cells:            (1,3,1,1)
   }

   ctrl: {
      id:               103
      pid:              300
      type:             label
      text:             "AI Difficulty:"
      cells:            (1,4,1,1)
   }

   ctrl: {
      id:               104
      pid:              300
      type:             label
      text:             "HUD Mode:"
      cells:            (1,5,1,1)
   }

   ctrl: {
      id:               105
      pid:              300
      type:             label
      text:             "HUD Color:"
      cells:            (1,6,1,1)
   }

   ctrl: {
      id:               106
      pid:              300
      type:             label
      text:             "Friendly Fire:"
      cells:            (1,7,1,1)
   }

   ctrl: {
      id:               107
      pid:              300
      type:             label
      text:             "Reference Grid:"
      cells:            (1,8,1,1)
   }

   ctrl: {
      id:               108
      pid:              300
      type:             label
      text:             "Gunsight:"
      cells:            (1,9,1,1)
   }

   ctrl: {
      id:               500
      pid:              300
      type:             label
      cells:            (4,1,2,8)
   }

   defctrl: { 
      cell_insets:      (0,0,0,10)
      transparent:      false 
   }

   ctrl: {
      id:               201
      pid:              300
      type:             combo
      cells:            (2,1,1,1)

      item:             Standard
      item:             Relaxed
      item:             Arcade

      alt: "Select the flight model to be used by all ships and fighters in the game.\n\n"
           "* The Standard model uses zero-drag Newtonian physics and is similar to Terminus or IWar.  "
           "This model also uses realistic aerodynamics when flying in a planetary atmosphere.\n\n"
           "* The Relaxed model uses Newtonian physics, with added drag to make control easier.  "
           "This model is similar to that of EVE or Jumpgate.\n\n"
           "* The Arcade model keeps your ship flying in the direction it is pointing.  "
           "This model is similar to that of the Wing Commander games."
   }

   ctrl: {
      id:               211
      pid:              300
      type:             combo
      cells:            (2,2,1,1)

      item:             Disabled
      item:             Enabled

      alt: "Choose whether to begin campaign missions on the flight deck or in space.\n\n"
           "* If Flying Start is disabled, you will begin all fighter-based missions in the dynamic campaign on the flight deck or runway, ready to launch.\n\n"
           "* If Flying Start is enabled, you will begin fighter-based missions in the dynamic campaign already in flight.  This setting will get you into the action faster by skipping the launch procedures.\n\n"
   }

   ctrl: {
      id:               202
      pid:              300
      type:             combo
      cells:            (2,3,1,1)

      item:             Standard
      item:             Easier

      alt: "Select the landing model that your fighter will use in the game.\n\n"
           "* The Standard model takes landing velocity into account and requires a softer touch to keep from damaging your ship.\n\n"
           "* The Easier model makes it easier to land your bird under adverse conditions."
   }

   ctrl: {
      id:               203
      pid:              300
      type:             combo
      cells:            (2,4,1,1)

      item:             Ace
      item:             Veteran
      item:             Rookie

      alt: "Select the AI Difficulty level that you wish to face in the game.\n\n"
           "* The Ace level is the hardest, with AI pilots that are good shots and very aggressive.\n\n"
           "* The Veteran level is easier.  Enemy pilots are still somewhat aggressive, but are less skilled at defense.\n\n"
           "* The Rookie level is very easy.  Enemy pilots have poor threat reaction, rarely use missiles, and fly more slowly."
   }

   ctrl: {
      id:               204
      pid:              300
      type:             combo
      cells:            (2,5,1,1)

      item:             Standard
      item:             Simplified

      alt: "Select the HUD that your ship will use in the game.\n\n"
           "* The Standard HUD provides more information but is more complex to read.\n\n"
           "* The Simplified HUD displays only the most important information and is easier to read.\n\n"
           "For best results, use the Standard HUD with the Standard flight model, and the Simplified HUD with the Arcade flight model."
   }

   ctrl: {
      id:               205
      pid:              300
      type:             combo
      cells:            (2,6,1,1)

      item:             Green
      item:             Blue
      item:             Orange
      item:             Black

      alt: "Select the default HUD color that your ship will use in the game.  "
           "You can always switch between HUD colors during play by pressing Shift+H.\n\n"
           "Tip: Switch to the black HUD color during the game when flying atmospheric missions under daylight conditions."
   }

   ctrl: {
      id:               206
      pid:              300
      type:             combo
      cells:            (2,7,1,1)

      item:             "None"
      item:             "25% Damage"
      item:             "50% Damage"
      item:             "75% Damage"
      item:             "Full Damage"

      alt: "Select the amount of damage caused by friendly fire incidents.  "
           "Full damage means that friendly fire is just as deadly as fire targeted at enemies.  "
           "None means that weapons fire will pass right through friendly ships.\n\n"
           "Tip:  You can use this setting to make cooperative network play safer and easier."
   }

   ctrl: {
      id:               207
      pid:              300
      type:             combo
      cells:            (2,8,1,1)

      item:             Disabled
      item:             Enabled

      alt: "Choose whether to enable or disable the reference grid displayed "
           "in the 3D tactical viewer."
   }

   ctrl: {
      id:               208
      pid:              300
      type:             combo
      cells:            (2,9,1,1)

      item:             "Standard LCOS",
      item:             "Lead Indicator",

      alt: "Select the type of gunsight to use in the game.\n\n"
           "* The Standard LCOS pipper is similar to a modern jet fighter.  Place the pipper over the target and pull the trigger.\n\n"
           "* The Lead Indicator gunsight places a lead diamond in front of the target.  Line up the gun crosshairs and the lead diamond to ensure a gun hit.\n\n"
           "If you prefer to use 'Virtual Joystick' mouse control, you will probably have more success with the Lead Indicator gunsight."
   }

   // buttons:

   defctrl: {
      align:            left,
      font:             Limerick12,
      fore_color:       (0,0,0),
      standard_image:   Button17_0,
      activated_image:  Button17_1,
      transition_image: Button17_2,
      bevel_width:      6,
      margins:          (3,18,0,0),
      cell_insets:      (0,10,0,26)
      transparent:      false
   }

   ctrl: {
      id:            1,
      type:          button,
      text:          "Apply",
      cells:         (3,5,1,1)
   }

   ctrl: {
      id:            2,
      type:          button,
      text:          "Cancel",
      cells:         (4,5,1,1),
   }
}
