// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/app_list/cocoa/current_user_menu_item_view.h"

#include "base/mac/scoped_nsobject.h"
#include "grit/ui_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// Padding on the left of the indicator icon.
const CGFloat kMenuLeftMargin = 3;

// Padding on the top and bottom of the menu item.
const CGFloat kMenuTopBottomPadding = 2;

}

@interface CurrentUserMenuItemView ()

// Adds a text label in the custom view in the menu showing the current user.
- (NSTextField*)addLabelWithFrame:(NSPoint)origin
                        labelText:(NSString*)labelText;

@end

@implementation CurrentUserMenuItemView

- (id)initWithCurrentUser:(NSString*)userName
                userEmail:(NSString*)userEmail {
  if ((self = [super initWithFrame:NSZeroRect])) {
    NSImage* userImage = ui::ResourceBundle::GetSharedInstance().
        GetNativeImageNamed(IDR_APP_LIST_USER_INDICATOR).AsNSImage();
    NSRect imageRect = NSMakeRect(kMenuLeftMargin, kMenuTopBottomPadding, 0, 0);
    imageRect.size = [userImage size];
    base::scoped_nsobject<NSImageView> userImageView(
        [[NSImageView alloc] initWithFrame:imageRect]);
    [userImageView setImage:userImage];
    [self addSubview:userImageView];

    NSPoint labelOrigin = NSMakePoint(NSMaxX(imageRect), kMenuTopBottomPadding);
    NSTextField* userField =
        [self addLabelWithFrame:labelOrigin
                      labelText:userName];

    labelOrigin.y = NSMaxY([userField frame]);
    NSTextField* emailField =
        [self addLabelWithFrame:labelOrigin
                      labelText:userEmail];
    [emailField setTextColor:[NSColor disabledControlTextColor]];

    // Size the container view to fit the longest label.
    NSRect labelFrame = [emailField frame];
    if (NSWidth([userField frame]) > NSWidth(labelFrame))
      labelFrame.size.width = NSWidth([userField frame]);
    [self setFrameSize:NSMakeSize(
        NSMaxX(labelFrame) + NSMaxX(imageRect),
        NSMaxY(labelFrame) + kMenuTopBottomPadding)];
  }
  return self;
}

- (NSTextField*)addLabelWithFrame:(NSPoint)origin
                        labelText:(NSString*)labelText {
  NSRect labelFrame = NSZeroRect;
  labelFrame.origin = origin;
  base::scoped_nsobject<NSTextField> label(
      [[NSTextField alloc] initWithFrame:labelFrame]);
  [label setStringValue:labelText];
  [label setEditable:NO];
  [label setBordered:NO];
  [label setDrawsBackground:NO];
  [label setFont:[NSFont menuFontOfSize:0]];
  [label sizeToFit];
  [self addSubview:label];
  return label.autorelease();
}

- (BOOL)isFlipped {
  return YES;
}

@end
