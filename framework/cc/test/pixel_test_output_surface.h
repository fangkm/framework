// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_PIXEL_TEST_OUTPUT_SURFACE_H_
#define CC_TEST_PIXEL_TEST_OUTPUT_SURFACE_H_

#include "cc/output/output_surface.h"

namespace cc {

class PixelTestOutputSurface : public OutputSurface {
 public:
  explicit PixelTestOutputSurface(
      scoped_refptr<ContextProvider> context_provider)
      : OutputSurface(context_provider) {}
  explicit PixelTestOutputSurface(
      scoped_ptr<cc::SoftwareOutputDevice> software_device)
      : OutputSurface(software_device.Pass()) {}

  virtual void Reshape(gfx::Size size, float scale_factor) OVERRIDE;

  void set_surface_expansion_size(gfx::Size surface_expansion_size) {
    surface_expansion_size_ = surface_expansion_size;
  }
  void set_viewport_offset(gfx::Vector2d viewport_offset) {
    viewport_offset_ = viewport_offset;
  }
  void set_device_clip(gfx::Rect device_clip) {
    device_clip_ = device_clip;
  }

 private:
  gfx::Size surface_expansion_size_;
  gfx::Vector2d viewport_offset_;
  gfx::Rect device_clip_;
};

}  // namespace cc

#endif  // CC_TEST_PIXEL_TEST_OUTPUT_SURFACE_H_
