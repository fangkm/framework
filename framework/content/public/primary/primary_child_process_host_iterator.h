// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_BROWSER_CHILD_PROCESS_HOST_ITERATOR_H_
#define CONTENT_PUBLIC_BROWSER_BROWSER_CHILD_PROCESS_HOST_ITERATOR_H_

#include <list>

#include "content/common/content_export.h"

namespace IPC {
class Message;
}

namespace content {
class PrimaryChildProcessHostDelegate;
class PrimaryChildProcessHostImpl;
struct ChildProcessData;

// This class allows iteration through either all child processes, or ones of a
// specific type, depending on which constructor is used.  Note that this should
// be done from the IO thread and that the iterator should not be kept around as
// it may be invalidated on subsequent event processing in the event loop.
class CONTENT_EXPORT PrimaryChildProcessHostIterator {
 public:
  PrimaryChildProcessHostIterator();
  explicit PrimaryChildProcessHostIterator(int type);

  // These methods work on the current iterator object. Only call them if
  // Done() returns false.
  bool operator++();
  bool Done();
  const ChildProcessData& GetData();
  bool Send(IPC::Message* message);
  PrimaryChildProcessHostDelegate* GetDelegate();

 private:
  bool all_;
  int process_type_;
  std::list<PrimaryChildProcessHostImpl*>::iterator iterator_;
};

// Helper class so that subclasses of PrimaryChildProcessHostDelegate can be
// iterated with no casting needing. Note that because of the components build,
// this class can only be used by BCPHD implementations that live in content,
// otherwise link errors will result.
template <class T>
class CONTENT_EXPORT BrowserChildProcessHostTypeIterator
    : public PrimaryChildProcessHostIterator {
 public:
  explicit BrowserChildProcessHostTypeIterator(int process_type)
      : PrimaryChildProcessHostIterator(process_type) {}
  T* operator->() {
    return static_cast<T*>(GetDelegate());
  }
  T* operator*() {
    return static_cast<T*>(GetDelegate());
  }
};

};  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_BROWSER_CHILD_PROCESS_HOST_ITERATOR_H_
