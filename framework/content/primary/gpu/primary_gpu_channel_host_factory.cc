// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/primary/gpu/primary_gpu_channel_host_factory.h"

#include "base/bind.h"
#include "base/threading/thread_restrictions.h"
#include "content/primary/gpu/gpu_data_manager_impl.h"
#include "content/primary/gpu/gpu_process_host.h"
#include "content/primary/gpu/gpu_surface_tracker.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/child_process_host_impl.h"
#include "content/public/primary/primary_thread.h"
#include "content/public/common/content_client.h"
#include "ipc/ipc_forwarding_message_filter.h"

namespace content {

PrimaryGpuChannelHostFactory* PrimaryGpuChannelHostFactory::instance_ = NULL;

PrimaryGpuChannelHostFactory::CreateRequest::CreateRequest()
    : event(false, false),
      gpu_host_id(0),
      route_id(MSG_ROUTING_NONE) {
}

PrimaryGpuChannelHostFactory::CreateRequest::~CreateRequest() {
}

PrimaryGpuChannelHostFactory::EstablishRequest::EstablishRequest(
    CauseForGpuLaunch cause)
    : event(false, false),
      cause_for_gpu_launch(cause),
      gpu_host_id(0),
      reused_gpu_process(true) {
}

PrimaryGpuChannelHostFactory::EstablishRequest::~EstablishRequest() {
}

void PrimaryGpuChannelHostFactory::Initialize() {
  instance_ = new PrimaryGpuChannelHostFactory();
}

void PrimaryGpuChannelHostFactory::Terminate() {
  delete instance_;
  instance_ = NULL;
}

PrimaryGpuChannelHostFactory::PrimaryGpuChannelHostFactory()
    : gpu_client_id_(ChildProcessHostImpl::GenerateChildProcessUniqueId()),
      shutdown_event_(new base::WaitableEvent(true, false)),
      gpu_host_id_(0) {
}

PrimaryGpuChannelHostFactory::~PrimaryGpuChannelHostFactory() {
  shutdown_event_->Signal();
}

bool PrimaryGpuChannelHostFactory::IsMainThread() {
  return PrimaryThread::CurrentlyOn(PrimaryThread::UI);
}

base::MessageLoop* PrimaryGpuChannelHostFactory::GetMainLoop() {
  return PrimaryThread::UnsafeGetMessageLoopForThread(PrimaryThread::UI);
}

scoped_refptr<base::MessageLoopProxy>
PrimaryGpuChannelHostFactory::GetIOLoopProxy() {
  return PrimaryThread::GetMessageLoopProxyForThread(PrimaryThread::IO);
}

base::WaitableEvent* PrimaryGpuChannelHostFactory::GetShutDownEvent() {
  return shutdown_event_.get();
}

scoped_ptr<base::SharedMemory>
PrimaryGpuChannelHostFactory::AllocateSharedMemory(size_t size) {
  scoped_ptr<base::SharedMemory> shm(new base::SharedMemory());
  if (!shm->CreateAnonymous(size))
    return scoped_ptr<base::SharedMemory>();
  return shm.Pass();
}

void PrimaryGpuChannelHostFactory::CreateViewCommandBufferOnIO(
    CreateRequest* request,
    int32 surface_id,
    const GPUCreateCommandBufferConfig& init_params) {
  GpuProcessHost* host = GpuProcessHost::FromID(gpu_host_id_);
  if (!host) {
    request->event.Signal();
    return;
  }

  gfx::GLSurfaceHandle surface =
      GpuSurfaceTracker::Get()->GetSurfaceHandle(surface_id);

  host->CreateViewCommandBuffer(
      surface,
      surface_id,
      gpu_client_id_,
      init_params,
      base::Bind(&PrimaryGpuChannelHostFactory::CommandBufferCreatedOnIO,
                 request));
}

// static
void PrimaryGpuChannelHostFactory::CommandBufferCreatedOnIO(
    CreateRequest* request, int32 route_id) {
  request->route_id = route_id;
  request->event.Signal();
}

int32 PrimaryGpuChannelHostFactory::CreateViewCommandBuffer(
      int32 surface_id,
      const GPUCreateCommandBufferConfig& init_params) {
  CreateRequest request;
  GetIOLoopProxy()->PostTask(FROM_HERE, base::Bind(
        &PrimaryGpuChannelHostFactory::CreateViewCommandBufferOnIO,
        base::Unretained(this),
        &request,
        surface_id,
        init_params));
  // We're blocking the UI thread, which is generally undesirable.
  // In this case we need to wait for this before we can show any UI /anyway/,
  // so it won't cause additional jank.
  // TODO(piman): Make this asynchronous (http://crbug.com/125248).
  base::ThreadRestrictions::ScopedAllowWait allow_wait;
  request.event.Wait();
  return request.route_id;
}

void PrimaryGpuChannelHostFactory::CreateImageOnIO(
    gfx::PluginWindowHandle window,
    int32 image_id,
    const CreateImageCallback& callback) {
  GpuProcessHost* host = GpuProcessHost::FromID(gpu_host_id_);
  if (!host) {
    ImageCreatedOnIO(callback, gfx::Size());
    return;
  }

  host->CreateImage(
      window,
      gpu_client_id_,
      image_id,
      base::Bind(&PrimaryGpuChannelHostFactory::ImageCreatedOnIO, callback));
}

// static
void PrimaryGpuChannelHostFactory::ImageCreatedOnIO(
    const CreateImageCallback& callback, const gfx::Size size) {
  PrimaryThread::PostTask(
      PrimaryThread::UI,
      FROM_HERE,
      base::Bind(&PrimaryGpuChannelHostFactory::OnImageCreated,
                 callback, size));
}

// static
void PrimaryGpuChannelHostFactory::OnImageCreated(
    const CreateImageCallback& callback, const gfx::Size size) {
  callback.Run(size);
}

void PrimaryGpuChannelHostFactory::CreateImage(
    gfx::PluginWindowHandle window,
    int32 image_id,
    const CreateImageCallback& callback) {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::UI));
  GetIOLoopProxy()->PostTask(FROM_HERE, base::Bind(
        &PrimaryGpuChannelHostFactory::CreateImageOnIO,
        base::Unretained(this),
        window,
        image_id,
        callback));
}

void PrimaryGpuChannelHostFactory::DeleteImageOnIO(
    int32 image_id, int32 sync_point) {
  GpuProcessHost* host = GpuProcessHost::FromID(gpu_host_id_);
  if (!host) {
    return;
  }

  host->DeleteImage(gpu_client_id_, image_id, sync_point);
}

void PrimaryGpuChannelHostFactory::DeleteImage(
    int32 image_id, int32 sync_point) {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::UI));
  GetIOLoopProxy()->PostTask(FROM_HERE, base::Bind(
        &PrimaryGpuChannelHostFactory::DeleteImageOnIO,
        base::Unretained(this),
        image_id,
        sync_point));
}

void PrimaryGpuChannelHostFactory::EstablishGpuChannelOnIO(
    EstablishRequest* request) {
  GpuProcessHost* host = GpuProcessHost::FromID(gpu_host_id_);
  if (!host) {
    host = GpuProcessHost::Get(GpuProcessHost::GPU_PROCESS_KIND_SANDBOXED,
                               request->cause_for_gpu_launch);
    if (!host) {
      request->event.Signal();
      return;
    }
    gpu_host_id_ = host->host_id();
    request->reused_gpu_process = false;
  } else {
    if (host->host_id() == request->gpu_host_id) {
      // We come here if we retried to establish the channel because of a
      // failure in GpuChannelEstablishedOnIO, but we ended up with the same
      // process ID, meaning the failure was not because of a channel error, but
      // another reason. So fail now.
      request->event.Signal();
      return;
    }
    request->reused_gpu_process = true;
  }
  request->gpu_host_id = gpu_host_id_;

  host->EstablishGpuChannel(
      gpu_client_id_,
      true,
      base::Bind(&PrimaryGpuChannelHostFactory::GpuChannelEstablishedOnIO,
                 base::Unretained(this),
                 request));
}

void PrimaryGpuChannelHostFactory::GpuChannelEstablishedOnIO(
    EstablishRequest* request,
    const IPC::ChannelHandle& channel_handle,
    const gpu::GPUInfo& gpu_info) {
  if (channel_handle.name.empty() && request->reused_gpu_process) {
    // We failed after re-using the GPU process, but it may have died in the
    // mean time. Retry to have a chance to create a fresh GPU process.
    EstablishGpuChannelOnIO(request);
  } else {
    request->channel_handle = channel_handle;
    request->gpu_info = gpu_info;
    request->event.Signal();
  }
}

GpuChannelHost* PrimaryGpuChannelHostFactory::EstablishGpuChannelSync(
    CauseForGpuLaunch cause_for_gpu_launch) {
  if (gpu_channel_.get()) {
    // Recreate the channel if it has been lost.
    if (gpu_channel_->IsLost())
      gpu_channel_ = NULL;
    else
      return gpu_channel_.get();
  }
  // Ensure initialization on the main thread.
  GpuDataManagerImpl::GetInstance();

  EstablishRequest request(cause_for_gpu_launch);
  GetIOLoopProxy()->PostTask(
      FROM_HERE,
      base::Bind(
          &PrimaryGpuChannelHostFactory::EstablishGpuChannelOnIO,
          base::Unretained(this),
          &request));

  {
    // We're blocking the UI thread, which is generally undesirable.
    // In this case we need to wait for this before we can show any UI /anyway/,
    // so it won't cause additional jank.
    // TODO(piman): Make this asynchronous (http://crbug.com/125248).
    base::ThreadRestrictions::ScopedAllowWait allow_wait;
    request.event.Wait();
  }

  if (request.channel_handle.name.empty())
    return NULL;

  GetContentClient()->SetGpuInfo(request.gpu_info);
  gpu_channel_ = GpuChannelHost::Create(
      this, request.gpu_host_id, gpu_client_id_,
      request.gpu_info, request.channel_handle);
  return gpu_channel_.get();
}

// static
void PrimaryGpuChannelHostFactory::AddFilterOnIO(
    int host_id,
    scoped_refptr<IPC::ChannelProxy::MessageFilter> filter) {
  DCHECK(PrimaryThread::CurrentlyOn(PrimaryThread::IO));

  GpuProcessHost* host = GpuProcessHost::FromID(host_id);
  if (host)
    host->AddFilter(filter.get());
}

void PrimaryGpuChannelHostFactory::SetHandlerForControlMessages(
      const uint32* message_ids,
      size_t num_messages,
      const base::Callback<void(const IPC::Message&)>& handler,
      base::TaskRunner* target_task_runner) {
  DCHECK(gpu_host_id_)
      << "Do not call"
      << " PrimaryGpuChannelHostFactory::SetHandlerForControlMessages()"
      << " until the GpuProcessHost has been set up.";

  scoped_refptr<IPC::ForwardingMessageFilter> filter =
      new IPC::ForwardingMessageFilter(message_ids,
                                       num_messages,
                                       target_task_runner);
  filter->AddRoute(MSG_ROUTING_CONTROL, handler);

  GetIOLoopProxy()->PostTask(
      FROM_HERE,
      base::Bind(&PrimaryGpuChannelHostFactory::AddFilterOnIO,
                 gpu_host_id_,
                 filter));
}

}  // namespace content
