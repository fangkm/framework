/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/dom/CustomElementCallbackScheduler.h"

#include "core/dom/CustomElementCallbackDispatcher.h"
#include "core/dom/CustomElementCallbackQueue.h"
#include "core/dom/CustomElementLifecycleCallbacks.h"
#include "core/dom/Element.h"

namespace WebCore {

void CustomElementCallbackScheduler::scheduleAttributeChangedCallback(PassRefPtr<CustomElementLifecycleCallbacks> callbacks, PassRefPtr<Element> element, const AtomicString& name, const AtomicString& oldValue, const AtomicString& newValue)
{
    if (!callbacks->hasAttributeChangedCallback())
        return;

    CustomElementCallbackQueue* queue = CustomElementCallbackDispatcher::instance().schedule(element);
    queue->append(CustomElementCallbackInvocation::createAttributeChangedInvocation(callbacks, name, oldValue, newValue));
}

void CustomElementCallbackScheduler::scheduleCreatedCallback(PassRefPtr<CustomElementLifecycleCallbacks> callbacks, PassRefPtr<Element> element)
{
    if (!callbacks->hasCreatedCallback())
        return;

    CustomElementCallbackQueue* queue = CustomElementCallbackDispatcher::instance().scheduleInCurrentElementQueue(element);
    queue->append(CustomElementCallbackInvocation::createInvocation(callbacks, CustomElementLifecycleCallbacks::Created));
}

void CustomElementCallbackScheduler::scheduleEnteredDocumentCallback(PassRefPtr<CustomElementLifecycleCallbacks> callbacks, PassRefPtr<Element> element)
{
    if (!callbacks->hasEnteredDocumentCallback())
        return;

    CustomElementCallbackQueue* queue = CustomElementCallbackDispatcher::instance().schedule(element);
    queue->append(CustomElementCallbackInvocation::createInvocation(callbacks, CustomElementLifecycleCallbacks::EnteredDocument));
}

void CustomElementCallbackScheduler::scheduleLeftDocumentCallback(PassRefPtr<CustomElementLifecycleCallbacks> callbacks, PassRefPtr<Element> element)
{
    if (!callbacks->hasLeftDocumentCallback())
        return;

    CustomElementCallbackQueue* queue = CustomElementCallbackDispatcher::instance().schedule(element);
    queue->append(CustomElementCallbackInvocation::createInvocation(callbacks, CustomElementLifecycleCallbacks::LeftDocument));
}

} // namespace WebCore
