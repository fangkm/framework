/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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
#include "V8PromiseResolver.h"

#include "bindings/v8/V8Binding.h"
#include "bindings/v8/custom/V8PromiseCustom.h"
#include <v8.h>

namespace WebCore {

void V8PromiseResolver::fulfillMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Local<v8::Object> resolver = args.This();
    if (V8PromiseCustom::isInternalDetached(resolver))
        return;
    v8::Local<v8::Object> internal = V8PromiseCustom::getInternal(resolver);
    if (V8PromiseCustom::getState(internal) != V8PromiseCustom::Pending)
        return;
    V8PromiseCustom::setState(internal, V8PromiseCustom::PendingWithResolvedFlagSet);

    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Value> result = v8::Undefined();
    if (args.Length() > 0)
        result = args[0];
    V8PromiseCustom::fulfillResolver(resolver, result, V8PromiseCustom::Asynchronous, isolate);
}

void V8PromiseResolver::resolveMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Local<v8::Object> resolver = args.This();
    if (V8PromiseCustom::isInternalDetached(resolver))
        return;
    v8::Local<v8::Object> internal = V8PromiseCustom::getInternal(resolver);
    if (V8PromiseCustom::getState(internal) != V8PromiseCustom::Pending)
        return;
    V8PromiseCustom::setState(internal, V8PromiseCustom::PendingWithResolvedFlagSet);

    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Value> result = v8::Undefined();
    if (args.Length() > 0)
        result = args[0];
    V8PromiseCustom::resolveResolver(resolver, result, V8PromiseCustom::Asynchronous, isolate);
}

void V8PromiseResolver::rejectMethodCustom(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Local<v8::Object> resolver = args.This();
    if (V8PromiseCustom::isInternalDetached(resolver))
        return;
    v8::Local<v8::Object> internal = V8PromiseCustom::getInternal(resolver);
    if (V8PromiseCustom::getState(internal) != V8PromiseCustom::Pending)
        return;
    V8PromiseCustom::setState(internal, V8PromiseCustom::PendingWithResolvedFlagSet);

    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Value> result = v8::Undefined();
    if (args.Length() > 0)
        result = args[0];
    V8PromiseCustom::rejectResolver(resolver, result, V8PromiseCustom::Asynchronous, isolate);
}

} // namespace WebCore
