// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_CONTROLS_HYPERLINK_BUTTON_CELL_H_
#define UI_BASE_COCOA_CONTROLS_HYPERLINK_BUTTON_CELL_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "ui/base/ui_export.h"

// A HyperlinkButtonCell is used to create an NSButton that looks and acts
// like a hyperlink. The default styling is to look like blue, underlined text
// and to have the pointingHand cursor on mouse over.
//
// To use in Interface Builder:
//  1. Drag out an NSButton.
//  2. Double click on the button so you have the cell component selected.
//  3. In the Identity panel of the inspector, set the custom class to this.
//  4. In the Attributes panel, change the Bezel to Square.
//  5. In the Size panel, set the Height to 16.
//
// Use this if all of your text is a link. If you need text that contains
// embedded links but also regular text, use HyperlinkTextView.
UI_EXPORT
@interface HyperlinkButtonCell : NSButtonCell {
  base::scoped_nsobject<NSColor> textColor_;
  BOOL shouldUnderline_;
  BOOL underlineOnHover_;
  BOOL mouseIsInside_;
}
@property(nonatomic, retain) NSColor* textColor;
@property(nonatomic, assign) BOOL underlineOnHover;
@property(nonatomic, assign) BOOL shouldUnderline;

+ (NSColor*)defaultTextColor;

// Helper function to create a button with HyperLinkButtonCell as its cell.
+ (NSButton*)buttonWithString:(NSString*)string;

@end

@interface HyperlinkButtonCell (ExposedForTesting)
- (NSDictionary*)linkAttributes;
@end

#endif  // UI_BASE_COCOA_CONTROLS_HYPERLINK_BUTTON_CELL_H_
