// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SCHEDULER_SCHEDULER_STATE_MACHINE_H_
#define CC_SCHEDULER_SCHEDULER_STATE_MACHINE_H_

#include <string>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "cc/base/cc_export.h"
#include "cc/output/begin_frame_args.h"
#include "cc/scheduler/scheduler_settings.h"

namespace base {
class Value;
}

namespace cc {

// The SchedulerStateMachine decides how to coordinate main thread activites
// like painting/running javascript with rendering and input activities on the
// impl thread.
//
// The state machine tracks internal state but is also influenced by external
// state.  Internal state includes things like whether a frame has been
// requested, while external state includes things like the current time being
// near to the vblank time.
//
// The scheduler seperates "what to do next" from the updating of its internal
// state to make testing cleaner.
class CC_EXPORT SchedulerStateMachine {
 public:
  // settings must be valid for the lifetime of this class.
  explicit SchedulerStateMachine(const SchedulerSettings& settings);

  enum OutputSurfaceState {
    OUTPUT_SURFACE_ACTIVE,
    OUTPUT_SURFACE_LOST,
    OUTPUT_SURFACE_CREATING,
    OUTPUT_SURFACE_WAITING_FOR_FIRST_COMMIT,
    OUTPUT_SURFACE_WAITING_FOR_FIRST_ACTIVATION,
  };
  static const char* OutputSurfaceStateToString(OutputSurfaceState state);

  enum CommitState {
    COMMIT_STATE_IDLE,
    COMMIT_STATE_FRAME_IN_PROGRESS,
    COMMIT_STATE_READY_TO_COMMIT,
    COMMIT_STATE_WAITING_FOR_FIRST_DRAW,
  };
  static const char* CommitStateToString(CommitState state);

  enum TextureState {
    LAYER_TEXTURE_STATE_UNLOCKED,
    LAYER_TEXTURE_STATE_ACQUIRED_BY_MAIN_THREAD,
    LAYER_TEXTURE_STATE_ACQUIRED_BY_IMPL_THREAD,
  };
  static const char* TextureStateToString(TextureState state);

  enum SynchronousReadbackState {
    READBACK_STATE_IDLE,
    READBACK_STATE_NEEDS_BEGIN_FRAME,
    READBACK_STATE_WAITING_FOR_COMMIT,
    READBACK_STATE_WAITING_FOR_ACTIVATION,
    READBACK_STATE_WAITING_FOR_DRAW_AND_READBACK,
    READBACK_STATE_WAITING_FOR_REPLACEMENT_COMMIT,
    READBACK_STATE_WAITING_FOR_REPLACEMENT_ACTIVATION,
  };
  static const char* SynchronousReadbackStateToString(
      SynchronousReadbackState state);

  enum ForcedRedrawOnTimeoutState {
    FORCED_REDRAW_STATE_IDLE,
    FORCED_REDRAW_STATE_WAITING_FOR_COMMIT,
    FORCED_REDRAW_STATE_WAITING_FOR_ACTIVATION,
    FORCED_REDRAW_STATE_WAITING_FOR_DRAW,
  };
  static const char* ForcedRedrawOnTimeoutStateToString(
      ForcedRedrawOnTimeoutState state);

  bool CommitPending() const {
    return commit_state_ == COMMIT_STATE_FRAME_IN_PROGRESS ||
           commit_state_ == COMMIT_STATE_READY_TO_COMMIT;
  }

  bool RedrawPending() const { return needs_redraw_; }

  enum Action {
    ACTION_NONE,
    ACTION_SEND_BEGIN_FRAME_TO_MAIN_THREAD,
    ACTION_COMMIT,
    ACTION_UPDATE_VISIBLE_TILES,
    ACTION_ACTIVATE_PENDING_TREE,
    ACTION_DRAW_AND_SWAP_IF_POSSIBLE,
    ACTION_DRAW_AND_SWAP_FORCED,
    ACTION_DRAW_AND_SWAP_ABORT,
    ACTION_DRAW_AND_READBACK,
    ACTION_BEGIN_OUTPUT_SURFACE_CREATION,
    ACTION_ACQUIRE_LAYER_TEXTURES_FOR_MAIN_THREAD,
  };
  static const char* ActionToString(Action action);

  scoped_ptr<base::Value> AsValue() const;

  Action NextAction() const;
  void UpdateState(Action action);
  void CheckInvariants();

  // Indicates whether the main thread needs a begin frame callback in order to
  // make progress.
  bool BeginFrameNeededToDrawByImplThread() const;
  bool ProactiveBeginFrameWantedByImplThread() const;

  // Indicates that the system has entered and left a BeginFrame callback.
  // The scheduler will not draw more than once in a given BeginFrame
  // callback nor send more than one BeginFrame message.
  void DidEnterBeginFrame(const BeginFrameArgs& args);
  void DidLeaveBeginFrame();
  bool inside_begin_frame() const { return inside_begin_frame_; }

  // Indicates whether the LayerTreeHostImpl is visible.
  void SetVisible(bool visible);

  // Indicates that a redraw is required, either due to the impl tree changing
  // or the screen being damaged and simply needing redisplay.
  void SetNeedsRedraw();

  // Indicates whether a redraw is required because we are currently rendering
  // with a low resolution or checkerboarded tile.
  void SetSwapUsedIncompleteTile(bool used_incomplete_tile);

  // Indicates whether ACTION_DRAW_AND_SWAP_IF_POSSIBLE drew to the screen or
  // not.
  void DidDrawIfPossibleCompleted(bool success);

  // Indicates that a new commit flow needs to be performed, either to pull
  // updates from the main thread to the impl, or to push deltas from the impl
  // thread to main.
  void SetNeedsCommit();

  // As SetNeedsCommit(), but ensures the begin frame will be sent to the main
  // thread even if we are not visible.  After this call we expect to go through
  // the forced commit flow and then return to waiting for a non-forced
  // begin frame to finish.
  void SetNeedsForcedCommitForReadback();

  // Call this only in response to receiving an
  // ACTION_SEND_BEGIN_FRAME_TO_MAIN_THREAD from NextAction.
  // Indicates that all painting is complete.
  void FinishCommit();

  // Call this only in response to receiving an
  // ACTION_SEND_BEGIN_FRAME_TO_MAIN_THREAD from NextAction if the client
  // rejects the begin frame message.  If did_handle is false, then
  // another commit will be retried soon.
  void BeginFrameAbortedByMainThread(bool did_handle);

  // Request exclusive access to the textures that back single buffered
  // layers on behalf of the main thread. Upon acquisition,
  // ACTION_DRAW_AND_SWAP_IF_POSSIBLE will not draw until the main thread
  // releases the
  // textures to the impl thread by committing the layers.
  void SetMainThreadNeedsLayerTextures();

  // Set that we can create the first OutputSurface and start the scheduler.
  void SetCanStart() { can_start_ = true; }

  // Indicates whether drawing would, at this time, make sense.
  // CanDraw can be used to supress flashes or checkerboarding
  // when such behavior would be undesirable.
  void SetCanDraw(bool can);

  // Indicates that the pending tree is ready for activation.
  void NotifyReadyToActivate();

  bool has_pending_tree() const { return has_pending_tree_; }

  void DidLoseOutputSurface();
  void DidCreateAndInitializeOutputSurface();
  bool HasInitializedOutputSurface() const;

  // True if we need to abort draws to make forward progress.
  bool PendingDrawsShouldBeAborted() const;

 protected:
  // True if we need to force activations to make forward progress.
  bool PendingActivationsShouldBeForced() const;

  bool ShouldBeginOutputSurfaceCreation() const;
  bool ShouldDrawForced() const;
  bool ShouldDraw() const;
  bool ShouldActivatePendingTree() const;
  bool ShouldAcquireLayerTexturesForMainThread() const;
  bool ShouldUpdateVisibleTiles() const;
  bool ShouldSendBeginFrameToMainThread() const;
  bool ShouldCommit() const;

  bool HasDrawnAndSwappedThisFrame() const;
  bool HasActivatedPendingTreeThisFrame() const;
  bool HasUpdatedVisibleTilesThisFrame() const;
  bool HasSentBeginFrameToMainThreadThisFrame() const;

  void UpdateStateOnCommit(bool commit_was_aborted);
  void UpdateStateOnActivation();
  void UpdateStateOnDraw(bool did_swap);

  const SchedulerSettings settings_;

  OutputSurfaceState output_surface_state_;
  CommitState commit_state_;
  TextureState texture_state_;
  ForcedRedrawOnTimeoutState forced_redraw_state_;
  SynchronousReadbackState readback_state_;

  int commit_count_;
  int current_frame_number_;
  int last_frame_number_where_begin_frame_sent_to_main_thread_;
  int last_frame_number_swap_performed_;
  int last_frame_number_where_update_visible_tiles_was_called_;
  int consecutive_failed_draws_;
  bool needs_redraw_;
  bool swap_used_incomplete_tile_;
  bool needs_commit_;
  bool main_thread_needs_layer_textures_;
  bool inside_begin_frame_;
  BeginFrameArgs last_begin_frame_args_;
  bool visible_;
  bool can_start_;
  bool can_draw_;
  bool has_pending_tree_;
  bool pending_tree_is_ready_for_activation_;
  bool active_tree_needs_first_draw_;
  bool draw_if_possible_failed_;
  bool did_create_and_initialize_first_output_surface_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SchedulerStateMachine);
};

}  // namespace cc

#endif  // CC_SCHEDULER_SCHEDULER_STATE_MACHINE_H_
