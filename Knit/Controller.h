//
//  Controller.h
//  ERManager
//
//  Created by Daniel Riley on 7/18/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//



#import <Cocoa/Cocoa.h>

#include "KnitSerial.h"
#include "KnitController.h"
#include "KnitProtocol.h"

@class Controller;
typedef Controller* ControllerRef;



@interface Controller : NSObject {
	IBOutlet NSMenu *serialMenu;
  IBOutlet NSMenu *protocolMenu;
	IBOutlet NSWindow *serialWindow;
	IBOutlet NSTextView *serialTextView;
  
  IBOutlet NSImageView* imageView;
  
  IBOutlet NSColorWell* color1;
  IBOutlet NSColorWell* color2;
  IBOutlet NSColorWell* color3;
  IBOutlet NSColorWell* color4;
  IBOutlet NSColorWell* color5;
  IBOutlet NSColorWell* color6;
  
  
  IBOutlet NSTextField* color_label_1;
  IBOutlet NSTextField* color_label_2;
  IBOutlet NSTextField* color_label_3;
  IBOutlet NSTextField* color_label_4;
  IBOutlet NSTextField* color_label_5;
  IBOutlet NSTextField* color_label_6;
  
  IBOutlet NSTextField* numColors;
  
  NSArray* colors;
  
  IBOutlet NSTextField* colorField;
  IBOutlet NSTextField* rowField;
  IBOutlet NSTextField* totalRowsField;

  int selectedPortIndex;
  int selectedProtocolIndex;

  KnitController* _knitCtrl;
  
@private
  boost::signals2::scoped_connection _rowChangedConnection;
  boost::signals2::scoped_connection _colorChangedConnection;
}

-(void)awakeFromNib;
// Updates the port menu
-(IBAction)refreshPorts:(id)sender;
// Selects a port from the menu
-(IBAction)selectPort:(id)sender;
-(IBAction)selectProtocol:(id)sender;
-(IBAction)restart:(id)sender;
-(IBAction)set:(id)sender;
-(IBAction)load:(id)sender;
-(IBAction)upload:(id)sender;
-(IBAction)uploadBlank:(id)sender;
-(void)updateImage;
-(IBAction)colorChanged:(id)sender;
-(IBAction)numColorsChanged:(id)sender;
-(IBAction)ColorLabelChanged:(id)sender;

@end
