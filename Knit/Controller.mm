//
//  Controller.m
//  ERDevice
//
//  Created by Daniel Riley on 4/7/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "Controller.h"
#include "KnitController.h"
#include "d2bd/SerialPort.h"
#include "d2bd/SerialPortManager.h"
#include <string>

@implementation Controller



/**
 * Initialize data structures
 */
-(void)awakeFromNib
{
  __unsafe_unretained Controller* weak_self = self;
  
  selectedPortIndex = 0;
  selectedProtocolIndex = -1;

  _knitCtrl = new KnitController();
  
  _rowChangedConnection = _knitCtrl->ConnectRowChanged(
      [weak_self](unsigned int row) {
        dispatch_async(dispatch_get_main_queue(), ^{
          weak_self->rowField.integerValue = row;
        });
      });
  
  _colorChangedConnection = _knitCtrl->ConnectColorChanged(
      [weak_self](unsigned int color) {
        dispatch_async(dispatch_get_main_queue(), ^{
          weak_self->colorField.integerValue = color + 1;
        });
      });

  colors = [[NSArray alloc] initWithObjects:color1, color2, color3, color4, color5, color6, nil];
	[self refreshPorts:NULL];

	// Clear the serial menu except for the refresh button
	protocolMenu.autoenablesItems = NO;

  while (protocolMenu.numberOfItems > 0) {
    [protocolMenu removeItemAtIndex:protocolMenu.numberOfItems - 1];
  }

  for (unsigned int i = 0; i < kKnitprotocolCount; i++) {
    const char* knit_protocol_name = nullptr;
    
    switch ((KnitProtocolType)i) {
      case kKnitProtocolSK840:
        knit_protocol_name = "SK840";
        break;
      case kKnitProtocolPassap:
        knit_protocol_name = "Passap E6000";
        break;
      case kKnitProtocolPassapDirect:
        knit_protocol_name = "Passap E6000 (direct)";
        break;
      default:
        knit_protocol_name = "Unknown";
        break;
    }
    
    NSString* protocolName = [[NSString alloc] initWithCString:knit_protocol_name encoding:NSUTF8StringEncoding];
    NSMenuItem* protocolMenuItem = [[NSMenuItem alloc] initWithTitle:protocolName action:@selector(selectProtocol:) keyEquivalent:@""];
    
    [protocolMenuItem setTarget:self];
    [protocolMenuItem setEnabled:YES];
    
    [protocolMenu addItem:protocolMenuItem];
    
    [protocolName release];
    [protocolMenuItem release];
  }
}

- (void)dealloc {
  delete _knitCtrl;
  
  [colors release];
  
  [super dealloc];
}

- (IBAction)restart:(id)sender
{
  _knitCtrl->reset(0, 0);
}

- (IBAction)set:(id)sender
{
  int color = (int)colorField.integerValue;
  int row = (int)rowField.integerValue;
  
  color = color > 0 ? color - 1 : 0;
  _knitCtrl->reset(row, color);
}

-(IBAction)selectProtocol:(id)sender
{
  int prevSelectedIndex = selectedProtocolIndex;
  selectedProtocolIndex = (int)[protocolMenu indexOfItem:sender];
  
  if (selectedProtocolIndex != prevSelectedIndex) {
    if (prevSelectedIndex > -1) {
      [[protocolMenu itemAtIndex:prevSelectedIndex] setState:NSOffState];
    }
    
    if (selectedProtocolIndex > -1) {
      [[protocolMenu itemAtIndex:selectedProtocolIndex] setState:NSOnState];
      _knitCtrl->SetProtocol((KnitProtocolType)selectedProtocolIndex);
    }
  }
}

-(IBAction)selectPort:(id)sender
{
	int prevSelectedIndex = selectedPortIndex;
  selectedPortIndex = (int)[serialMenu indexOfItem:sender] - 1;

	// Update the interface
	if (selectedPortIndex != prevSelectedIndex) {
		if (prevSelectedIndex >= 0) {
			[[serialMenu itemAtIndex:prevSelectedIndex + 1] setState:NSOffState];
    }
		
		if (selectedPortIndex >= 0) {
			[[serialMenu itemAtIndex:selectedPortIndex + 1] setState:NSOnState];
      _knitCtrl->SetSerialPortIndex(selectedPortIndex);
    }
	}
}

-(IBAction)refreshPorts:(id)sender
{
	// Fill available port list
	d2bd::SerialPortManager& inst = d2bd::SerialPortManager::instance();
	inst.enumerate();
	
	size_t portCount = inst.getNumPorts();
	
	// Clear the serial menu except for the refresh button
	serialMenu.autoenablesItems = NO;

  while (serialMenu.numberOfItems > 1) {
    [serialMenu removeItemAtIndex:serialMenu.numberOfItems - 1];
  }

	if (portCount >  0) {
		for (unsigned int i = 0; i < portCount; i++) {
      NSString* portName = [[NSString alloc] initWithCString:inst.getPortNameAt(i).c_str() encoding:NSUTF8StringEncoding];
      NSMenuItem* portMenuItem = [[NSMenuItem alloc] initWithTitle:portName action:@selector(selectPort:) keyEquivalent:@""];
			
			[portMenuItem setTarget:self];
			[portMenuItem setEnabled:YES];
			
			[serialMenu addItem:portMenuItem];
      
      [portName release];
      [portMenuItem release];
		}
	}
	else
	{
		[serialMenu addItemWithTitle:@"No Ports Available" action:NULL keyEquivalent:@""];
	}
}

- (IBAction)upload:(id)sender
{
  _knitCtrl->Upload();
}

- (IBAction)uploadBlank:(id)sender
{
  _knitCtrl->UploadBlank();
}

-(IBAction)load:(id)sender
{
  int result;
    NSArray *fileTypes = [NSArray arrayWithObject:@"gif"];
    NSOpenPanel *oPanel = [NSOpenPanel openPanel];
	
    [oPanel setAllowsMultipleSelection:NO];
	
	result = [oPanel runModalForTypes:fileTypes];
    if (result == NSOKButton) {
        NSArray *filesToOpen = [oPanel filenames];
		
		NSString *aFile = [filesToOpen objectAtIndex:0];
		
		_knitCtrl->loadImage([aFile cStringUsingEncoding:(NSASCIIStringEncoding)]);
  }
  
  // Display the image.
  const d2bd::IndexedImage* image = _knitCtrl->GetImage();
  
  for (int i = 0; i < 6; ++i) {
    NSColorWell* colorWell = [colors objectAtIndex:i];
    if (i < image->GetPalletteSize()) {
      const unsigned char* pal = &image->GetPallette()[i * 3];
      colorWell.color = [NSColor colorWithDeviceRed:pal[0] / 255.0
        green:pal[1] / 255.0 blue:pal[2] / 255.0 alpha:1.0];
      [colorWell deactivate];
      [colorWell setEnabled:YES];
    } else {
      colorWell.color = [NSColor colorWithDeviceRed:0.5 green:0.5 blue:0.5 alpha:1.0];
      [colorWell deactivate];
      [colorWell setEnabled:NO];
    }
  }
  
  totalRowsField.integerValue = image->GetHeight();
  
  [self updateImage];
}

- (IBAction)colorChanged:(id)sender
{
  NSLog(@"%@", sender);
  [self updateImage];
}

- (IBAction)numColorsChanged:(id)sender
{
  
}

- (IBAction)ColorLabelChanged:(id)sender
{
  int int_value = [sender intValue];
  
  if (int_value < 1) int_value = 1;
  if (int_value > 6) int_value = 6;
  
  int default_value = 0;
  NSTextField* other_field = nil;
  NSTextField* modified_field = sender;
  
  switch (int_value) {
    case 1: other_field = color_label_1; break;
    case 2: other_field = color_label_2; break;
    case 3: other_field = color_label_3; break;
    case 4: other_field = color_label_4; break;
    case 5: other_field = color_label_5; break;
    case 6: other_field = color_label_6; break;
  }
  
  if (modified_field == color_label_1) {
    default_value = 1;
  } else if (modified_field == color_label_2) {
    default_value = 2;
  } else if (modified_field == color_label_3) {
    default_value = 3;
  } else if (modified_field == color_label_4) {
    default_value = 4;
  } else if (modified_field == color_label_5) {
    default_value = 5;
  } else if (modified_field == color_label_6) {
    default_value = 6;
  }

  if (other_field != nil && default_value != 0 && other_field != modified_field) {
    _knitCtrl->SwapColors(default_value - 1, int_value - 1);
    // Swap color wells as well
    NSColorWell* modified_well = [colors objectAtIndex: default_value - 1];
    NSColorWell* other_well = [colors objectAtIndex:int_value - 1];
    
    double red = modified_well.color.redComponent;
    double green = modified_well.color.greenComponent;
    double blue = modified_well.color.blueComponent;
    
    modified_well.color = other_well.color;
    other_well.color = [NSColor colorWithDeviceRed:red green:green blue:blue alpha:1.0];

    NSLog(@"Change color %d to color %d", default_value, int_value);
    [self updateImage];
  }
  
  modified_field.intValue = default_value;
}

- (void)updateImage
{
  // Display the image.
  const d2bd::IndexedImage* image = _knitCtrl->GetImage();
  
  unsigned char pal[18];
  for (int i = 0; i < 6; ++i) {
    NSColorWell* colorWell = [colors objectAtIndex:i];
    pal[i * 3] = colorWell.color.redComponent * 255.0;
    pal[i * 3 + 1] = colorWell.color.greenComponent * 255.0;
    pal[i * 3 + 2] = colorWell.color.blueComponent * 255.0;
  }
 
  unsigned char* imageData = (unsigned char*)malloc(image->GetWidth() * image->GetHeight() * 4);
  unsigned char* dest = imageData;
  const unsigned char* src = image->GetData();
  for (int i = 0; i < image->GetWidth() * image->GetHeight(); ++i, src++, dest += 4) {
    if (*src < 6) {
      unsigned char* color = &pal[(*src) * 3];
      dest[0] = color[2];
      dest[1] = color[1];
      dest[2] = color[0];
      dest[3] = 0xFF;
    } else {
      dest[0] = 0x00;
      dest[1] = 0xFF;
      dest[2] = 0xFF;
      dest[3] = 0xFF;
    }
  }
  
  
  //bgra
  CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(imageData, image->GetWidth(),
      image->GetHeight(), 8, 4 * image->GetWidth(), colorspace,
      kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little);
  
  CGImageRef cgImage = CGBitmapContextCreateImage(context);
  
  CGContextRelease(context);
  CGColorSpaceRelease(colorspace);
  
  NSImage* nsImage = [[NSImage alloc] initWithCGImage:cgImage size:
    NSMakeSize(image->GetWidth(), image->GetHeight())];

  imageView.image = nsImage;
  [nsImage release];
  CGImageRelease(cgImage);

  imageView.imageScaling = NSScaleToFit;
  imageView.bounds = NSMakeRect(0, 0,
    image->GetWidth() * 2, image->GetHeight() * 2);
}

@end


