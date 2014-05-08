// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/resource_provider.h"

#include <algorithm>
#include <limits>

#include "base/containers/hash_tables.h"
#include "base/debug/alias.h"
#include "base/stl_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "cc/output/gl_renderer.h"  // For the GLC() macro.
#include "cc/resources/platform_color.h"
#include "cc/resources/returned_resource.h"
#include "cc/resources/transferable_resource.h"
#include "cc/scheduler/texture_uploader.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "third_party/WebKit/public/platform/WebGraphicsContext3D.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/khronos/GLES2/gl2ext.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/vector2d.h"

using WebKit::WebGraphicsContext3D;

namespace cc {

namespace {

// Measured in seconds.
const double kSoftwareUploadTickRate = 0.000250;
const double kTextureUploadTickRate = 0.004;

GLenum TextureToStorageFormat(GLenum texture_format) {
  GLenum storage_format = GL_RGBA8_OES;
  switch (texture_format) {
    case GL_RGBA:
      break;
    case GL_BGRA_EXT:
      storage_format = GL_BGRA8_EXT;
      break;
    default:
      NOTREACHED();
      break;
  }

  return storage_format;
}

bool IsTextureFormatSupportedForStorage(GLenum format) {
  return (format == GL_RGBA || format == GL_BGRA_EXT);
}

class ScopedSetActiveTexture {
 public:
  ScopedSetActiveTexture(WebGraphicsContext3D* context3d, GLenum unit)
      : context3d_(context3d), unit_(unit) {
    DCHECK_EQ(GL_TEXTURE0, ResourceProvider::GetActiveTextureUnit(context3d_));

    if (unit_ != GL_TEXTURE0)
      GLC(context3d_, context3d_->activeTexture(unit_));
  }

  ~ScopedSetActiveTexture() {
    // Active unit being GL_TEXTURE0 is effectively the ground state.
    if (unit_ != GL_TEXTURE0)
      GLC(context3d_, context3d_->activeTexture(GL_TEXTURE0));
  }

 private:
  WebGraphicsContext3D* context3d_;
  GLenum unit_;
};

}  // namespace

ResourceProvider::Resource::Resource()
    : gl_id(0),
      gl_pixel_buffer_id(0),
      gl_upload_query_id(0),
      pixels(NULL),
      pixel_buffer(NULL),
      lock_for_read_count(0),
      imported_count(0),
      exported_count(0),
      locked_for_write(false),
      external(false),
      marked_for_deletion(false),
      pending_set_pixels(false),
      set_pixels_completion_forced(false),
      allocated(false),
      enable_read_lock_fences(false),
      read_lock_fence(NULL),
      size(),
      format(0),
      filter(0),
      image_id(0),
      texture_pool(0),
      wrap_mode(0),
      hint(TextureUsageAny),
      type(static_cast<ResourceType>(0)) {}

ResourceProvider::Resource::~Resource() {}

ResourceProvider::Resource::Resource(
    unsigned texture_id,
    gfx::Size size,
    GLenum format,
    GLenum filter,
    GLenum texture_pool,
    GLint wrap_mode,
    TextureUsageHint hint)
    : gl_id(texture_id),
      gl_pixel_buffer_id(0),
      gl_upload_query_id(0),
      pixels(NULL),
      pixel_buffer(NULL),
      lock_for_read_count(0),
      imported_count(0),
      exported_count(0),
      locked_for_write(false),
      external(false),
      marked_for_deletion(false),
      pending_set_pixels(false),
      set_pixels_completion_forced(false),
      allocated(false),
      enable_read_lock_fences(false),
      read_lock_fence(NULL),
      size(size),
      format(format),
      filter(filter),
      image_id(0),
      texture_pool(texture_pool),
      wrap_mode(wrap_mode),
      hint(hint),
      type(GLTexture) {
  DCHECK(wrap_mode == GL_CLAMP_TO_EDGE || wrap_mode == GL_REPEAT);
}

ResourceProvider::Resource::Resource(
    uint8_t* pixels,
    gfx::Size size,
    GLenum format,
    GLenum filter,
    GLint wrap_mode)
    : gl_id(0),
      gl_pixel_buffer_id(0),
      gl_upload_query_id(0),
      pixels(pixels),
      pixel_buffer(NULL),
      lock_for_read_count(0),
      imported_count(0),
      exported_count(0),
      locked_for_write(false),
      external(false),
      marked_for_deletion(false),
      pending_set_pixels(false),
      set_pixels_completion_forced(false),
      allocated(false),
      enable_read_lock_fences(false),
      read_lock_fence(NULL),
      size(size),
      format(format),
      filter(filter),
      image_id(0),
      texture_pool(0),
      wrap_mode(wrap_mode),
      hint(TextureUsageAny),
      type(Bitmap) {
  DCHECK(wrap_mode == GL_CLAMP_TO_EDGE || wrap_mode == GL_REPEAT);
}

ResourceProvider::Child::Child() {}

ResourceProvider::Child::~Child() {}

scoped_ptr<ResourceProvider> ResourceProvider::Create(
    OutputSurface* output_surface,
    int highp_threshold_min) {
  scoped_ptr<ResourceProvider> resource_provider(
      new ResourceProvider(output_surface, highp_threshold_min));

  bool success = false;
  if (resource_provider->Context3d()) {
    success = resource_provider->InitializeGL();
  } else {
    resource_provider->InitializeSoftware();
    success = true;
  }

  if (!success)
    return scoped_ptr<ResourceProvider>();

  DCHECK_NE(InvalidType, resource_provider->default_resource_type());
  return resource_provider.Pass();
}

ResourceProvider::~ResourceProvider() {
  while (!resources_.empty())
    DeleteResourceInternal(resources_.begin(), ForShutdown);

  CleanUpGLIfNeeded();
}

bool ResourceProvider::InUseByConsumer(ResourceId id) {
  Resource* resource = GetResource(id);
  return resource->lock_for_read_count > 0 || resource->exported_count > 0;
}

ResourceProvider::ResourceId ResourceProvider::CreateResource(
    gfx::Size size, GLenum format, GLint wrap_mode, TextureUsageHint hint) {
  DCHECK(!size.IsEmpty());
  switch (default_resource_type_) {
    case GLTexture:
      return CreateGLTexture(size, format, GL_TEXTURE_POOL_UNMANAGED_CHROMIUM,
                             wrap_mode, hint);
    case Bitmap:
      // The only wrap_mode currently implemented in software mode is
      // GL_CLAMP_TO_EDGE.
      // http://crbug.com/284796
      DCHECK(format == GL_RGBA);
      return CreateBitmap(size);
    case InvalidType:
      break;
  }

  LOG(FATAL) << "Invalid default resource type.";
  return 0;
}

ResourceProvider::ResourceId ResourceProvider::CreateManagedResource(
    gfx::Size size, GLenum format, GLint wrap_mode, TextureUsageHint hint) {
  DCHECK(!size.IsEmpty());
  switch (default_resource_type_) {
    case GLTexture:
      return CreateGLTexture(size, format, GL_TEXTURE_POOL_MANAGED_CHROMIUM,
                             wrap_mode, hint);
    case Bitmap:
      DCHECK(format == GL_RGBA);
      return CreateBitmap(size);
    case InvalidType:
      break;
  }

  LOG(FATAL) << "Invalid default resource type.";
  return 0;
}

ResourceProvider::ResourceId ResourceProvider::CreateGLTexture(
    gfx::Size size,
    GLenum format,
    GLenum texture_pool,
    GLint wrap_mode,
    TextureUsageHint hint) {
  DCHECK_LE(size.width(), max_texture_size_);
  DCHECK_LE(size.height(), max_texture_size_);
  DCHECK(thread_checker_.CalledOnValidThread());

  ResourceId id = next_id_++;
  Resource resource(0, size, format, GL_LINEAR, texture_pool, wrap_mode, hint);
  resource.allocated = false;
  resources_[id] = resource;
  return id;
}

ResourceProvider::ResourceId ResourceProvider::CreateBitmap(gfx::Size size) {
  DCHECK(thread_checker_.CalledOnValidThread());

  uint8_t* pixels = new uint8_t[4 * size.GetArea()];

  ResourceId id = next_id_++;
  Resource resource(pixels, size, GL_RGBA, GL_LINEAR, GL_CLAMP_TO_EDGE);
  resource.allocated = true;
  resources_[id] = resource;
  return id;
}

ResourceProvider::ResourceId
ResourceProvider::CreateResourceFromExternalTexture(
    unsigned texture_target,
    unsigned texture_id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  WebGraphicsContext3D* context3d = Context3d();
  DCHECK(context3d);
  GLC(context3d, context3d->bindTexture(texture_target, texture_id));
  GLC(context3d, context3d->texParameteri(
      texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GLC(context3d, context3d->texParameteri(
      texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GLC(context3d, context3d->texParameteri(
      texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GLC(context3d, context3d->texParameteri(
      texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  ResourceId id = next_id_++;
  Resource resource(texture_id, gfx::Size(), 0, GL_LINEAR, 0, GL_CLAMP_TO_EDGE,
                    TextureUsageAny);
  resource.external = true;
  resource.allocated = true;
  resources_[id] = resource;
  return id;
}

ResourceProvider::ResourceId ResourceProvider::CreateResourceFromTextureMailbox(
    const TextureMailbox& mailbox) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Just store the information. Mailbox will be consumed in LockForRead().
  ResourceId id = next_id_++;
  DCHECK(mailbox.IsValid());
  Resource& resource = resources_[id];
  if (mailbox.IsTexture()) {
    resource = Resource(0, gfx::Size(), 0, GL_LINEAR, 0, GL_CLAMP_TO_EDGE,
                        TextureUsageAny);
  } else {
    DCHECK(mailbox.IsSharedMemory());
    base::SharedMemory* shared_memory = mailbox.shared_memory();
    DCHECK(shared_memory->memory());
    uint8_t* pixels = reinterpret_cast<uint8_t*>(shared_memory->memory());
    resource = Resource(pixels, mailbox.shared_memory_size(),
                        GL_RGBA, GL_LINEAR, GL_CLAMP_TO_EDGE);
  }
  resource.external = true;
  resource.allocated = true;
  resource.mailbox = mailbox;
  return id;
}

void ResourceProvider::DeleteResource(ResourceId id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ResourceMap::iterator it = resources_.find(id);
  CHECK(it != resources_.end());
  Resource* resource = &it->second;
  DCHECK(!resource->lock_for_read_count);
  DCHECK(!resource->marked_for_deletion);
  DCHECK_EQ(resource->imported_count, 0);
  DCHECK(resource->pending_set_pixels || !resource->locked_for_write);

  if (resource->exported_count > 0) {
    resource->marked_for_deletion = true;
    return;
  } else {
    DeleteResourceInternal(it, Normal);
  }
}

void ResourceProvider::DeleteResourceInternal(ResourceMap::iterator it,
                                              DeleteStyle style) {
  Resource* resource = &it->second;
  bool lost_resource = lost_output_surface_;

  DCHECK(resource->exported_count == 0 || style != Normal);
  if (style == ForShutdown && resource->exported_count > 0)
    lost_resource = true;

  if (resource->image_id) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    GLC(context3d, context3d->destroyImageCHROMIUM(resource->image_id));
  }

  if (resource->gl_id && !resource->external) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    GLC(context3d, context3d->deleteTexture(resource->gl_id));
  }
  if (resource->gl_upload_query_id) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    GLC(context3d, context3d->deleteQueryEXT(resource->gl_upload_query_id));
  }
  if (resource->gl_pixel_buffer_id) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    GLC(context3d, context3d->deleteBuffer(resource->gl_pixel_buffer_id));
  }
  if (resource->mailbox.IsValid() && resource->external) {
    unsigned sync_point = resource->mailbox.sync_point();
    if (resource->mailbox.IsTexture()) {
      WebGraphicsContext3D* context3d = Context3d();
      DCHECK(context3d);
      if (resource->gl_id)
        GLC(context3d, context3d->deleteTexture(resource->gl_id));
      if (!lost_resource && resource->gl_id)
        sync_point = context3d->insertSyncPoint();
    } else {
      DCHECK(resource->mailbox.IsSharedMemory());
      base::SharedMemory* shared_memory = resource->mailbox.shared_memory();
      if (resource->pixels && shared_memory) {
        DCHECK(shared_memory->memory() == resource->pixels);
        resource->pixels = NULL;
      }
    }
    resource->mailbox.RunReleaseCallback(sync_point, lost_resource);
  }
  if (resource->pixels)
    delete[] resource->pixels;
  if (resource->pixel_buffer)
    delete[] resource->pixel_buffer;

  resources_.erase(it);
}

ResourceProvider::ResourceType ResourceProvider::GetResourceType(
    ResourceId id) {
  return GetResource(id)->type;
}

void ResourceProvider::SetPixels(ResourceId id,
                                 const uint8_t* image,
                                 gfx::Rect image_rect,
                                 gfx::Rect source_rect,
                                 gfx::Vector2d dest_offset) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->locked_for_write);
  DCHECK(!resource->lock_for_read_count);
  DCHECK(!resource->external);
  DCHECK_EQ(resource->exported_count, 0);
  DCHECK(ReadLockFenceHasPassed(resource));
  LazyAllocate(resource);

  if (resource->gl_id) {
    DCHECK(!resource->pending_set_pixels);
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    DCHECK(texture_uploader_.get());
    context3d->bindTexture(GL_TEXTURE_2D, resource->gl_id);
    texture_uploader_->Upload(image,
                              image_rect,
                              source_rect,
                              dest_offset,
                              resource->format,
                              resource->size);
  }

  if (resource->pixels) {
    DCHECK(resource->allocated);
    DCHECK(resource->format == GL_RGBA);
    SkBitmap src_full;
    src_full.setConfig(
        SkBitmap::kARGB_8888_Config, image_rect.width(), image_rect.height());
    src_full.setPixels(const_cast<uint8_t*>(image));
    SkBitmap src_subset;
    SkIRect sk_source_rect = SkIRect::MakeXYWH(source_rect.x(),
                                               source_rect.y(),
                                               source_rect.width(),
                                               source_rect.height());
    sk_source_rect.offset(-image_rect.x(), -image_rect.y());
    src_full.extractSubset(&src_subset, sk_source_rect);

    ScopedWriteLockSoftware lock(this, id);
    SkCanvas* dest = lock.sk_canvas();
    dest->writePixels(src_subset, dest_offset.x(), dest_offset.y());
  }
}

size_t ResourceProvider::NumBlockingUploads() {
  if (!texture_uploader_)
    return 0;

  return texture_uploader_->NumBlockingUploads();
}

void ResourceProvider::MarkPendingUploadsAsNonBlocking() {
  if (!texture_uploader_)
    return;

  texture_uploader_->MarkPendingUploadsAsNonBlocking();
}

double ResourceProvider::EstimatedUploadsPerSecond() {
  if (!texture_uploader_)
    return 0.0;

  return texture_uploader_->EstimatedTexturesPerSecond();
}

void ResourceProvider::FlushUploads() {
  if (!texture_uploader_)
    return;

  texture_uploader_->Flush();
}

void ResourceProvider::ReleaseCachedData() {
  if (!texture_uploader_)
    return;

  texture_uploader_->ReleaseCachedQueries();
}

base::TimeDelta ResourceProvider::TextureUpdateTickRate() {
  // Software resource uploads happen on impl thread, so don't bother batching
  // them up and trying to wait for them to complete.
  double rate =
      texture_uploader_ ? kTextureUploadTickRate : kSoftwareUploadTickRate;
  return base::TimeDelta::FromMicroseconds(base::Time::kMicrosecondsPerSecond *
                                           rate);
}

void ResourceProvider::Flush() {
  DCHECK(thread_checker_.CalledOnValidThread());
  WebGraphicsContext3D* context3d = Context3d();
  if (context3d)
    context3d->flush();
}

void ResourceProvider::Finish() {
  DCHECK(thread_checker_.CalledOnValidThread());
  WebGraphicsContext3D* context3d = Context3d();
  if (context3d)
    context3d->finish();
}

bool ResourceProvider::ShallowFlushIfSupported() {
  DCHECK(thread_checker_.CalledOnValidThread());
  WebGraphicsContext3D* context3d = Context3d();
  if (!context3d || !use_shallow_flush_)
    return false;

  context3d->shallowFlushCHROMIUM();
  return true;
}

ResourceProvider::Resource* ResourceProvider::GetResource(ResourceId id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ResourceMap::iterator it = resources_.find(id);
  CHECK(it != resources_.end());
  return &it->second;
}

const ResourceProvider::Resource* ResourceProvider::LockForRead(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->locked_for_write ||
         resource->set_pixels_completion_forced) <<
      "locked for write: " << resource->locked_for_write <<
      " pixels completion forced: " << resource->set_pixels_completion_forced;
  DCHECK_EQ(resource->exported_count, 0);
  // Uninitialized! Call SetPixels or LockForWrite first.
  DCHECK(resource->allocated);

  LazyCreate(resource);

  if (resource->external) {
    if (!resource->gl_id && resource->mailbox.IsTexture()) {
      WebGraphicsContext3D* context3d = Context3d();
      DCHECK(context3d);
      if (resource->mailbox.sync_point()) {
        GLC(context3d,
            context3d->waitSyncPoint(resource->mailbox.sync_point()));
        resource->mailbox.ResetSyncPoint();
      }
      resource->gl_id = context3d->createTexture();
      GLC(context3d, context3d->bindTexture(
          resource->mailbox.target(), resource->gl_id));
      GLC(context3d, context3d->consumeTextureCHROMIUM(
          resource->mailbox.target(), resource->mailbox.data()));
    }
  }

  resource->lock_for_read_count++;
  if (resource->enable_read_lock_fences)
    resource->read_lock_fence = current_read_lock_fence_;

  return resource;
}

void ResourceProvider::UnlockForRead(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK_GT(resource->lock_for_read_count, 0);
  DCHECK_EQ(resource->exported_count, 0);
  resource->lock_for_read_count--;
}

const ResourceProvider::Resource* ResourceProvider::LockForWrite(
    ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->locked_for_write);
  DCHECK(!resource->lock_for_read_count);
  DCHECK_EQ(resource->exported_count, 0);
  DCHECK(!resource->external);
  DCHECK(ReadLockFenceHasPassed(resource));
  LazyAllocate(resource);

  resource->locked_for_write = true;
  return resource;
}

bool ResourceProvider::CanLockForWrite(ResourceId id) {
  Resource* resource = GetResource(id);
  return !resource->locked_for_write &&
      !resource->lock_for_read_count &&
      !resource->exported_count &&
      !resource->external &&
      ReadLockFenceHasPassed(resource);
}

void ResourceProvider::UnlockForWrite(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(resource->locked_for_write);
  DCHECK_EQ(resource->exported_count, 0);
  DCHECK(!resource->external);
  resource->locked_for_write = false;
}

ResourceProvider::ScopedReadLockGL::ScopedReadLockGL(
    ResourceProvider* resource_provider,
    ResourceProvider::ResourceId resource_id)
    : resource_provider_(resource_provider),
      resource_id_(resource_id),
      texture_id_(resource_provider->LockForRead(resource_id)->gl_id) {
  DCHECK(texture_id_);
}

ResourceProvider::ScopedReadLockGL::~ScopedReadLockGL() {
  resource_provider_->UnlockForRead(resource_id_);
}

ResourceProvider::ScopedSamplerGL::ScopedSamplerGL(
    ResourceProvider* resource_provider,
    ResourceProvider::ResourceId resource_id,
    GLenum target,
    GLenum filter)
    : ScopedReadLockGL(resource_provider, resource_id),
      target_(target),
      unit_(GL_TEXTURE0) {
  resource_provider->BindForSampling(resource_id, target, unit_, filter);
}

ResourceProvider::ScopedSamplerGL::ScopedSamplerGL(
    ResourceProvider* resource_provider,
    ResourceProvider::ResourceId resource_id,
    GLenum target,
    GLenum unit,
    GLenum filter)
    : ScopedReadLockGL(resource_provider, resource_id),
      target_(target),
      unit_(unit) {
  resource_provider->BindForSampling(resource_id, target, unit, filter);
}

ResourceProvider::ScopedSamplerGL::~ScopedSamplerGL() {
  resource_provider_->UnbindForSampling(resource_id_, target_, unit_);
}

ResourceProvider::ScopedWriteLockGL::ScopedWriteLockGL(
    ResourceProvider* resource_provider,
    ResourceProvider::ResourceId resource_id)
    : resource_provider_(resource_provider),
      resource_id_(resource_id),
      texture_id_(resource_provider->LockForWrite(resource_id)->gl_id) {
  DCHECK(texture_id_);
}

ResourceProvider::ScopedWriteLockGL::~ScopedWriteLockGL() {
  resource_provider_->UnlockForWrite(resource_id_);
}

void ResourceProvider::PopulateSkBitmapWithResource(
    SkBitmap* sk_bitmap, const Resource* resource) {
  DCHECK(resource->pixels);
  DCHECK(resource->format == GL_RGBA);
  sk_bitmap->setConfig(SkBitmap::kARGB_8888_Config,
                       resource->size.width(),
                       resource->size.height());
  sk_bitmap->setPixels(resource->pixels);
}

ResourceProvider::ScopedReadLockSoftware::ScopedReadLockSoftware(
    ResourceProvider* resource_provider,
    ResourceProvider::ResourceId resource_id)
    : resource_provider_(resource_provider),
      resource_id_(resource_id) {
  ResourceProvider::PopulateSkBitmapWithResource(
      &sk_bitmap_, resource_provider->LockForRead(resource_id));
}

ResourceProvider::ScopedReadLockSoftware::~ScopedReadLockSoftware() {
  resource_provider_->UnlockForRead(resource_id_);
}

ResourceProvider::ScopedWriteLockSoftware::ScopedWriteLockSoftware(
    ResourceProvider* resource_provider,
    ResourceProvider::ResourceId resource_id)
    : resource_provider_(resource_provider),
      resource_id_(resource_id) {
  ResourceProvider::PopulateSkBitmapWithResource(
      &sk_bitmap_, resource_provider->LockForWrite(resource_id));
  sk_canvas_.reset(new SkCanvas(sk_bitmap_));
}

ResourceProvider::ScopedWriteLockSoftware::~ScopedWriteLockSoftware() {
  resource_provider_->UnlockForWrite(resource_id_);
}

ResourceProvider::ResourceProvider(OutputSurface* output_surface,
                                   int highp_threshold_min)
    : output_surface_(output_surface),
      lost_output_surface_(false),
      highp_threshold_min_(highp_threshold_min),
      next_id_(1),
      next_child_(1),
      default_resource_type_(InvalidType),
      use_texture_storage_ext_(false),
      use_texture_usage_hint_(false),
      use_shallow_flush_(false),
      max_texture_size_(0),
      best_texture_format_(0) {
  DCHECK(output_surface_->HasClient());
}

void ResourceProvider::InitializeSoftware() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_NE(Bitmap, default_resource_type_);

  CleanUpGLIfNeeded();

  default_resource_type_ = Bitmap;
  max_texture_size_ = INT_MAX / 2;
  best_texture_format_ = GL_RGBA;
}

bool ResourceProvider::InitializeGL() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!texture_uploader_);
  DCHECK_NE(GLTexture, default_resource_type_);

  WebGraphicsContext3D* context3d = Context3d();
  DCHECK(context3d);

  if (!context3d->makeContextCurrent())
    return false;

  default_resource_type_ = GLTexture;

  const ContextProvider::Capabilities& caps =
      output_surface_->context_provider()->ContextCapabilities();

  bool use_map_sub = caps.map_sub;
  bool use_bgra = caps.texture_format_bgra8888;
  use_texture_storage_ext_ = caps.texture_storage;
  use_shallow_flush_ = caps.shallow_flush;
  use_texture_usage_hint_ = caps.texture_usage;

  texture_uploader_ =
      TextureUploader::Create(context3d, use_map_sub, use_shallow_flush_);
  GLC(context3d, context3d->getIntegerv(GL_MAX_TEXTURE_SIZE,
                                        &max_texture_size_));
  best_texture_format_ = PlatformColor::BestTextureFormat(use_bgra);

  return true;
}

void ResourceProvider::CleanUpGLIfNeeded() {
  WebGraphicsContext3D* context3d = Context3d();
  if (default_resource_type_ != GLTexture) {
    // We are not in GL mode, but double check before returning.
    DCHECK(!context3d);
    DCHECK(!texture_uploader_);
    return;
  }

  DCHECK(context3d);
  context3d->makeContextCurrent();
  texture_uploader_.reset();
  Finish();
}

int ResourceProvider::CreateChild() {
  DCHECK(thread_checker_.CalledOnValidThread());
  Child child_info;
  int child = next_child_++;
  children_[child] = child_info;
  return child;
}

void ResourceProvider::DestroyChild(int child_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ChildMap::iterator it = children_.find(child_id);
  DCHECK(it != children_.end());
  Child& child = it->second;
  for (ResourceIdMap::iterator child_it = child.child_to_parent_map.begin();
       child_it != child.child_to_parent_map.end();
       ++child_it) {
    ResourceId id = child_it->second;
    // We're abandoning this resource, it will not get recycled.
    // crbug.com/224062
    ResourceMap::iterator resource_it = resources_.find(id);
    CHECK(resource_it != resources_.end());
    resource_it->second.imported_count = 0;
    DeleteResource(id);
  }
  children_.erase(it);
}

const ResourceProvider::ResourceIdMap& ResourceProvider::GetChildToParentMap(
    int child) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  ChildMap::const_iterator it = children_.find(child);
  DCHECK(it != children_.end());
  return it->second.child_to_parent_map;
}

void ResourceProvider::PrepareSendToParent(const ResourceIdArray& resources,
                                           TransferableResourceArray* list) {
  DCHECK(thread_checker_.CalledOnValidThread());
  WebGraphicsContext3D* context3d = Context3d();
  if (!context3d || !context3d->makeContextCurrent()) {
    // TODO(skaslev): Implement this path for software compositing.
    return;
  }
  bool need_sync_point = false;
  for (ResourceIdArray::const_iterator it = resources.begin();
       it != resources.end();
       ++it) {
    TransferableResource resource;
    TransferResource(context3d, *it, &resource);
    if (!resource.sync_point)
      need_sync_point = true;
    ++resources_.find(*it)->second.exported_count;
    list->push_back(resource);
  }
  if (need_sync_point) {
    unsigned int sync_point = context3d->insertSyncPoint();
    for (TransferableResourceArray::iterator it = list->begin();
         it != list->end();
         ++it) {
      if (!it->sync_point)
        it->sync_point = sync_point;
    }
  }
}

void ResourceProvider::PrepareSendReturnsToChild(
    int child,
    const ResourceIdArray& resources,
    ReturnedResourceArray* list) {
  DCHECK(thread_checker_.CalledOnValidThread());
  WebGraphicsContext3D* context3d = Context3d();
  if (!context3d || !context3d->makeContextCurrent()) {
    // TODO(skaslev): Implement this path for software compositing.
    return;
  }
  Child& child_info = children_.find(child)->second;
  bool need_sync_point = false;
  for (ResourceIdArray::const_iterator it = resources.begin();
       it != resources.end(); ++it) {
    Resource* resource = GetResource(*it);
    DCHECK(!resource->locked_for_write);
    DCHECK(!resource->lock_for_read_count);
    DCHECK(child_info.parent_to_child_map.find(*it) !=
           child_info.parent_to_child_map.end());

    ReturnedResource returned;
    returned.id = child_info.parent_to_child_map[*it];
    returned.filter = resource->filter;
    returned.sync_point = resource->mailbox.sync_point();
    if (!returned.sync_point)
      need_sync_point = true;
    returned.count = resource->imported_count;
    list->push_back(returned);

    child_info.parent_to_child_map.erase(*it);
    child_info.child_to_parent_map.erase(returned.id);
    resources_[*it].imported_count = 0;
    DeleteResource(*it);
  }
  if (need_sync_point) {
    unsigned int sync_point = context3d->insertSyncPoint();
    for (ReturnedResourceArray::iterator it = list->begin();
         it != list->end(); ++it) {
      if (!it->sync_point)
        it->sync_point = sync_point;
    }
  }
}

void ResourceProvider::ReceiveFromChild(
    int child, const TransferableResourceArray& resources) {
  DCHECK(thread_checker_.CalledOnValidThread());
  WebGraphicsContext3D* context3d = Context3d();
  if (!context3d || !context3d->makeContextCurrent()) {
    // TODO(skaslev): Implement this path for software compositing.
    return;
  }
  Child& child_info = children_.find(child)->second;
  for (TransferableResourceArray::const_iterator it = resources.begin();
       it != resources.end();
       ++it) {
    ResourceIdMap::iterator resource_in_map_it =
        child_info.child_to_parent_map.find(it->id);
    if (resource_in_map_it != child_info.child_to_parent_map.end()) {
      resources_[resource_in_map_it->second].imported_count++;
      continue;
    }
    unsigned texture_id;
    // NOTE: If the parent is a browser and the child a renderer, the parent
    // is not supposed to have its context wait, because that could induce
    // deadlocks and/or security issues. The caller is responsible for
    // waiting asynchronously, and resetting sync_point before calling this.
    // However if the parent is a renderer (e.g. browser tag), it may be ok
    // (and is simpler) to wait.
    if (it->sync_point)
      GLC(context3d, context3d->waitSyncPoint(it->sync_point));
    GLC(context3d, texture_id = context3d->createTexture());
    GLC(context3d, context3d->bindTexture(GL_TEXTURE_2D, texture_id));
    GLC(context3d, context3d->consumeTextureCHROMIUM(GL_TEXTURE_2D,
                                                     it->mailbox.name));
    ResourceId id = next_id_++;
    Resource resource(
        texture_id, it->size, it->format, it->filter, 0, GL_CLAMP_TO_EDGE,
        TextureUsageAny);
    resource.mailbox.SetName(it->mailbox);
    // Don't allocate a texture for a child.
    resource.allocated = true;
    resource.imported_count = 1;
    resources_[id] = resource;
    child_info.parent_to_child_map[id] = it->id;
    child_info.child_to_parent_map[it->id] = id;
  }
}

void ResourceProvider::ReceiveReturnsFromParent(
    const ReturnedResourceArray& resources) {
  DCHECK(thread_checker_.CalledOnValidThread());
  WebGraphicsContext3D* context3d = Context3d();
  if (!context3d || !context3d->makeContextCurrent()) {
    // TODO(skaslev): Implement this path for software compositing.
    return;
  }
  for (ReturnedResourceArray::const_iterator it = resources.begin();
       it != resources.end();
       ++it) {
    ResourceMap::iterator map_iterator = resources_.find(it->id);
    DCHECK(map_iterator != resources_.end());
    Resource* resource = &map_iterator->second;
    CHECK_GE(resource->exported_count, it->count);
    resource->exported_count -= it->count;
    if (resource->exported_count)
      continue;
    resource->filter = it->filter;
    if (resource->gl_id) {
      if (it->sync_point)
        GLC(context3d, context3d->waitSyncPoint(it->sync_point));
    } else {
      resource->mailbox = TextureMailbox(resource->mailbox.name(),
                                         resource->mailbox.callback(),
                                         it->sync_point);
    }
    if (resource->marked_for_deletion)
      DeleteResourceInternal(map_iterator, Normal);
  }
}

void ResourceProvider::TransferResource(WebGraphicsContext3D* context,
                                        ResourceId id,
                                        TransferableResource* resource) {
  Resource* source = GetResource(id);
  DCHECK(!source->locked_for_write);
  DCHECK(!source->lock_for_read_count);
  DCHECK(!source->external || (source->external && source->mailbox.IsValid()));
  DCHECK(source->allocated);
  resource->id = id;
  resource->format = source->format;
  resource->filter = source->filter;
  resource->size = source->size;

  // TODO(skaslev) Implement this path for shared memory resources.
  DCHECK(!source->mailbox.IsSharedMemory());

  if (!source->mailbox.IsTexture()) {
    // This is a resource allocated by the compositor, we need to produce it.
    // Don't set a sync point, the caller will do it.
    DCHECK(source->gl_id);
    GLC(context, context->bindTexture(GL_TEXTURE_2D, source->gl_id));
    GLC(context, context->genMailboxCHROMIUM(resource->mailbox.name));
    GLC(context, context->produceTextureCHROMIUM(GL_TEXTURE_2D,
                                                 resource->mailbox.name));
    source->mailbox.SetName(resource->mailbox);
  } else {
    // This is either an external resource, or a compositor resource that we
    // already exported. Make sure to forward the sync point that we were given.
    resource->mailbox = source->mailbox.name();
    resource->sync_point = source->mailbox.sync_point();
    source->mailbox.ResetSyncPoint();
  }
}

void ResourceProvider::AcquirePixelBuffer(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->external);
  DCHECK_EQ(resource->exported_count, 0);
  DCHECK(!resource->image_id);

  if (resource->type == GLTexture) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    if (!resource->gl_pixel_buffer_id)
      resource->gl_pixel_buffer_id = context3d->createBuffer();
    context3d->bindBuffer(
        GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
        resource->gl_pixel_buffer_id);
    context3d->bufferData(
        GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
        4 * resource->size.GetArea(),
        NULL,
        GL_DYNAMIC_DRAW);
    context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
  }

  if (resource->pixels) {
    if (resource->pixel_buffer)
      return;

    resource->pixel_buffer = new uint8_t[4 * resource->size.GetArea()];
  }
}

void ResourceProvider::ReleasePixelBuffer(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->external);
  DCHECK_EQ(resource->exported_count, 0);
  DCHECK(!resource->image_id);

  // The pixel buffer can be released while there is a pending "set pixels"
  // if completion has been forced. Any shared memory associated with this
  // pixel buffer will not be freed until the waitAsyncTexImage2DCHROMIUM
  // command has been processed on the service side. It is also safe to
  // reuse any query id associated with this resource before they complete
  // as each new query has a unique submit count.
  if (resource->pending_set_pixels) {
    DCHECK(resource->set_pixels_completion_forced);
    resource->pending_set_pixels = false;
    UnlockForWrite(id);
  }

  if (resource->type == GLTexture) {
    if (!resource->gl_pixel_buffer_id)
      return;
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    context3d->bindBuffer(
        GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
        resource->gl_pixel_buffer_id);
    context3d->bufferData(
        GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
        0,
        NULL,
        GL_DYNAMIC_DRAW);
    context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
  }

  if (resource->pixels) {
    if (!resource->pixel_buffer)
      return;
    delete[] resource->pixel_buffer;
    resource->pixel_buffer = NULL;
  }
}

uint8_t* ResourceProvider::MapPixelBuffer(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->external);
  DCHECK_EQ(resource->exported_count, 0);
  DCHECK(!resource->image_id);

  if (resource->type == GLTexture) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    DCHECK(resource->gl_pixel_buffer_id);
    context3d->bindBuffer(
        GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
        resource->gl_pixel_buffer_id);
    uint8_t* image = static_cast<uint8_t*>(
        context3d->mapBufferCHROMIUM(
            GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, GL_WRITE_ONLY));
    context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
    // Buffer is required to be 4-byte aligned.
    CHECK(!(reinterpret_cast<intptr_t>(image) & 3));
    return image;
  }

  if (resource->pixels)
    return resource->pixel_buffer;

  return NULL;
}

void ResourceProvider::UnmapPixelBuffer(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->external);
  DCHECK_EQ(resource->exported_count, 0);
  DCHECK(!resource->image_id);

  if (resource->type == GLTexture) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    DCHECK(resource->gl_pixel_buffer_id);
    context3d->bindBuffer(
        GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
        resource->gl_pixel_buffer_id);
    context3d->unmapBufferCHROMIUM(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM);
    context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
  }
}

void ResourceProvider::BindForSampling(ResourceProvider::ResourceId resource_id,
                                       GLenum target,
                                       GLenum unit,
                                       GLenum filter) {
  DCHECK(thread_checker_.CalledOnValidThread());
  WebGraphicsContext3D* context3d = Context3d();
  ResourceMap::iterator it = resources_.find(resource_id);
  DCHECK(it != resources_.end());
  Resource* resource = &it->second;
  DCHECK(resource->lock_for_read_count);
  DCHECK(!resource->locked_for_write || resource->set_pixels_completion_forced);

  ScopedSetActiveTexture scoped_active_tex(context3d, unit);
  GLC(context3d, context3d->bindTexture(target, resource->gl_id));
  if (filter != resource->filter) {
    GLC(context3d, context3d->texParameteri(target,
                                            GL_TEXTURE_MIN_FILTER,
                                            filter));
    GLC(context3d, context3d->texParameteri(target,
                                            GL_TEXTURE_MAG_FILTER,
                                            filter));
    resource->filter = filter;
  }

  if (resource->image_id)
    context3d->bindTexImage2DCHROMIUM(target, resource->image_id);
}

void ResourceProvider::UnbindForSampling(
    ResourceProvider::ResourceId resource_id, GLenum target, GLenum unit) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ResourceMap::iterator it = resources_.find(resource_id);
  DCHECK(it != resources_.end());
  Resource* resource = &it->second;

  if (!resource->image_id)
    return;

  WebGraphicsContext3D* context3d = Context3d();
  ScopedSetActiveTexture scoped_active_tex(context3d, unit);
  context3d->releaseTexImage2DCHROMIUM(target, resource->image_id);
}

void ResourceProvider::BeginSetPixels(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->pending_set_pixels);

  LazyCreate(resource);
  DCHECK(resource->gl_id || resource->allocated);
  DCHECK(ReadLockFenceHasPassed(resource));
  DCHECK(!resource->image_id);

  bool allocate = !resource->allocated;
  resource->allocated = true;
  LockForWrite(id);

  if (resource->gl_id) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    DCHECK(resource->gl_pixel_buffer_id);
    context3d->bindTexture(GL_TEXTURE_2D, resource->gl_id);
    context3d->bindBuffer(
        GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM,
        resource->gl_pixel_buffer_id);
    if (!resource->gl_upload_query_id)
      resource->gl_upload_query_id = context3d->createQueryEXT();
    context3d->beginQueryEXT(
        GL_ASYNC_PIXEL_UNPACK_COMPLETED_CHROMIUM,
        resource->gl_upload_query_id);
    if (allocate) {
      context3d->asyncTexImage2DCHROMIUM(GL_TEXTURE_2D,
                                         0, /* level */
                                         resource->format,
                                         resource->size.width(),
                                         resource->size.height(),
                                         0, /* border */
                                         resource->format,
                                         GL_UNSIGNED_BYTE,
                                         NULL);
    } else {
      context3d->asyncTexSubImage2DCHROMIUM(GL_TEXTURE_2D,
                                            0, /* level */
                                            0, /* x */
                                            0, /* y */
                                            resource->size.width(),
                                            resource->size.height(),
                                            resource->format,
                                            GL_UNSIGNED_BYTE,
                                            NULL);
    }
    context3d->endQueryEXT(GL_ASYNC_PIXEL_UNPACK_COMPLETED_CHROMIUM);
    context3d->bindBuffer(GL_PIXEL_UNPACK_TRANSFER_BUFFER_CHROMIUM, 0);
  }

  if (resource->pixels) {
    DCHECK(!resource->mailbox.IsValid());
    DCHECK(resource->pixel_buffer);
    DCHECK(resource->format == GL_RGBA);

    std::swap(resource->pixels, resource->pixel_buffer);
    delete[] resource->pixel_buffer;
    resource->pixel_buffer = NULL;
  }

  resource->pending_set_pixels = true;
  resource->set_pixels_completion_forced = false;
}

void ResourceProvider::ForceSetPixelsToComplete(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(resource->locked_for_write);
  DCHECK(resource->pending_set_pixels);
  DCHECK(!resource->set_pixels_completion_forced);

  if (resource->gl_id) {
    WebGraphicsContext3D* context3d = Context3d();
    GLC(context3d, context3d->bindTexture(GL_TEXTURE_2D, resource->gl_id));
    GLC(context3d, context3d->waitAsyncTexImage2DCHROMIUM(GL_TEXTURE_2D));
    GLC(context3d, context3d->bindTexture(GL_TEXTURE_2D, 0));
  }

  resource->set_pixels_completion_forced = true;
}

bool ResourceProvider::DidSetPixelsComplete(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(resource->locked_for_write);
  DCHECK(resource->pending_set_pixels);

  if (resource->gl_id) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    DCHECK(resource->gl_upload_query_id);
    unsigned complete = 1;
    context3d->getQueryObjectuivEXT(
        resource->gl_upload_query_id,
        GL_QUERY_RESULT_AVAILABLE_EXT,
        &complete);
    if (!complete)
      return false;
  }

  resource->pending_set_pixels = false;
  UnlockForWrite(id);

  return true;
}

void ResourceProvider::CreateForTesting(ResourceId id) {
  LazyCreate(GetResource(id));
}

GLint ResourceProvider::WrapModeForTesting(ResourceId id) {
  Resource* resource = GetResource(id);
  return resource->wrap_mode;
}

void ResourceProvider::LazyCreate(Resource* resource) {
  if (resource->type != GLTexture || resource->gl_id != 0)
    return;

  // Early out for resources that don't require texture creation.
  if (resource->texture_pool == 0)
    return;

  WebGraphicsContext3D* context3d = Context3d();
  DCHECK(context3d);

  // Create and set texture properties. Allocation is delayed until needed.
  GLC(context3d, resource->gl_id = context3d->createTexture());
  GLC(context3d, context3d->bindTexture(GL_TEXTURE_2D, resource->gl_id));
  GLC(context3d, context3d->texParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GLC(context3d, context3d->texParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GLC(context3d, context3d->texParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, resource->wrap_mode));
  GLC(context3d, context3d->texParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, resource->wrap_mode));
  GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D,
                                          GL_TEXTURE_POOL_CHROMIUM,
                                          resource->texture_pool));
  if (use_texture_usage_hint_ && resource->hint == TextureUsageFramebuffer) {
    GLC(context3d, context3d->texParameteri(GL_TEXTURE_2D,
                                            GL_TEXTURE_USAGE_ANGLE,
                                            GL_FRAMEBUFFER_ATTACHMENT_ANGLE));
  }
}

void ResourceProvider::AllocateForTesting(ResourceId id) {
  LazyAllocate(GetResource(id));
}

void ResourceProvider::LazyAllocate(Resource* resource) {
  DCHECK(resource);
  LazyCreate(resource);

  DCHECK(resource->gl_id || resource->allocated);
  if (resource->allocated || !resource->gl_id)
    return;
  resource->allocated = true;
  WebGraphicsContext3D* context3d = Context3d();
  gfx::Size& size = resource->size;
  GLenum format = resource->format;
  GLC(context3d, context3d->bindTexture(GL_TEXTURE_2D, resource->gl_id));
  if (use_texture_storage_ext_ && IsTextureFormatSupportedForStorage(format)) {
    GLenum storage_format = TextureToStorageFormat(format);
    GLC(context3d, context3d->texStorage2DEXT(GL_TEXTURE_2D,
                                              1,
                                              storage_format,
                                              size.width(),
                                              size.height()));
  } else {
    GLC(context3d, context3d->texImage2D(GL_TEXTURE_2D,
                                         0,
                                         format,
                                         size.width(),
                                         size.height(),
                                         0,
                                         format,
                                         GL_UNSIGNED_BYTE,
                                         NULL));
  }
}

void ResourceProvider::EnableReadLockFences(ResourceProvider::ResourceId id,
                                            bool enable) {
  Resource* resource = GetResource(id);
  resource->enable_read_lock_fences = enable;
}

void ResourceProvider::AcquireImage(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->external);
  DCHECK_EQ(resource->exported_count, 0);

  if (resource->type != GLTexture)
    return;

  if (resource->image_id)
    return;

  resource->allocated = true;
  WebGraphicsContext3D* context3d = Context3d();
  DCHECK(context3d);
  DCHECK_EQ(static_cast<GLenum>(GL_RGBA), resource->format);
  resource->image_id = context3d->createImageCHROMIUM(
      resource->size.width(), resource->size.height(), GL_RGBA8_OES);
  DCHECK(resource->image_id);
}

void ResourceProvider::ReleaseImage(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->external);
  DCHECK_EQ(resource->exported_count, 0);

  if (!resource->image_id)
    return;

  WebGraphicsContext3D* context3d = Context3d();
  DCHECK(context3d);
  context3d->destroyImageCHROMIUM(resource->image_id);
  resource->image_id = 0;
  resource->allocated = false;
}

uint8_t* ResourceProvider::MapImage(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(ReadLockFenceHasPassed(resource));
  DCHECK(!resource->external);
  DCHECK_EQ(resource->exported_count, 0);

  if (resource->image_id) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    return static_cast<uint8_t*>(
        context3d->mapImageCHROMIUM(resource->image_id, GL_READ_WRITE));
  }

  if (resource->pixels)
    return resource->pixels;

  return NULL;
}

void ResourceProvider::UnmapImage(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->external);
  DCHECK_EQ(resource->exported_count, 0);

  if (resource->image_id) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    context3d->unmapImageCHROMIUM(resource->image_id);
  }
}

int ResourceProvider::GetImageStride(ResourceId id) {
  Resource* resource = GetResource(id);
  DCHECK(!resource->external);
  DCHECK_EQ(resource->exported_count, 0);

  int stride = 0;

  if (resource->image_id) {
    WebGraphicsContext3D* context3d = Context3d();
    DCHECK(context3d);
    context3d->getImageParameterivCHROMIUM(
        resource->image_id, GL_IMAGE_ROWBYTES_CHROMIUM, &stride);
  }

  return stride;
}

GLint ResourceProvider::GetActiveTextureUnit(WebGraphicsContext3D* context) {
  GLint active_unit = 0;
  context->getIntegerv(GL_ACTIVE_TEXTURE, &active_unit);
  return active_unit;
}

WebKit::WebGraphicsContext3D* ResourceProvider::Context3d() const {
  ContextProvider* context_provider = output_surface_->context_provider();
  return context_provider ? context_provider->Context3d() : NULL;
}

}  // namespace cc
