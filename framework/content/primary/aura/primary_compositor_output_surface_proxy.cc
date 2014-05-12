// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/primary/aura/primary_compositor_output_surface_proxy.h"

#include "base/bind.h"
#include "content/primary/aura/primary_compositor_output_surface.h"
#include "content/primary/gpu/primary_gpu_channel_host_factory.h"
#include "content/common/gpu/gpu_messages.h"

namespace content {

PrimaryCompositorOutputSurfaceProxy::PrimaryCompositorOutputSurfaceProxy(
    IDMap<PrimaryCompositorOutputSurface>* surface_map)
    : surface_map_(surface_map),
      connected_to_gpu_process_host_id_(0) {}

PrimaryCompositorOutputSurfaceProxy::~PrimaryCompositorOutputSurfaceProxy() {}

void PrimaryCompositorOutputSurfaceProxy::ConnectToGpuProcessHost(
    base::SingleThreadTaskRunner* compositor_thread_task_runner) {
  PrimaryGpuChannelHostFactory* factory =
      PrimaryGpuChannelHostFactory::instance();

  int gpu_process_host_id = factory->GpuProcessHostId();
  if (connected_to_gpu_process_host_id_ == gpu_process_host_id)
    return;

  const uint32 kMessagesToFilter[] = { GpuHostMsg_UpdateVSyncParameters::ID };
  factory->SetHandlerForControlMessages(
      kMessagesToFilter,
      arraysize(kMessagesToFilter),
      base::Bind(&PrimaryCompositorOutputSurfaceProxy::
                     OnMessageReceivedOnCompositorThread,
                 this),
      compositor_thread_task_runner);
  connected_to_gpu_process_host_id_ = gpu_process_host_id;
}

void PrimaryCompositorOutputSurfaceProxy::OnMessageReceivedOnCompositorThread(
    const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(PrimaryCompositorOutputSurfaceProxy, message)
      IPC_MESSAGE_HANDLER(GpuHostMsg_UpdateVSyncParameters,
                          OnUpdateVSyncParametersOnCompositorThread);
  IPC_END_MESSAGE_MAP()
}

void
PrimaryCompositorOutputSurfaceProxy::OnUpdateVSyncParametersOnCompositorThread(
    int surface_id,
    base::TimeTicks timebase,
    base::TimeDelta interval) {
  PrimaryCompositorOutputSurface* surface = surface_map_->Lookup(surface_id);
  if (surface)
    surface->OnUpdateVSyncParameters(timebase, interval);
}
}  // namespace content
