// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/input/input_param_traits.h"

#include "content/common/content_param_traits.h"
#include "content/common/input/input_event_disposition.h"
#include "content/common/input/ipc_input_event_payload.h"
#include "content/common/input_messages.h"

namespace IPC {
namespace {
template <typename PayloadType>
scoped_ptr<content::InputEvent::Payload> ReadPayload(const Message* m,
                                                     PickleIterator* iter) {
  scoped_ptr<PayloadType> event = PayloadType::Create();
  if (!ReadParam(m, iter, event.get()))
    return scoped_ptr<content::InputEvent::Payload>();
  return event.template PassAs<content::InputEvent::Payload>();
}
}  // namespace

void ParamTraits<content::EventPacket>::Write(Message* m, const param_type& p) {
  WriteParam(m, p.id());
  WriteParam(m, p.events());
}

bool ParamTraits<content::EventPacket>::Read(const Message* m,
                                             PickleIterator* iter,
                                             param_type* p) {
  int64 id;
  content::EventPacket::InputEvents events;
  if (!ReadParam(m, iter, &id) ||
      !ReadParam(m, iter, &events))
    return false;

  p->set_id(id);
  bool events_added_successfully = true;
  for (size_t i = 0; i < events.size(); ++i) {
    // Take ownership of all events.
    scoped_ptr<content::InputEvent> event(events[i]);
    if (!events_added_successfully)
      continue;
    if (!p->Add(event.Pass()))
      events_added_successfully = false;
  }
  events.weak_clear();
  return events_added_successfully;
}

void ParamTraits<content::EventPacket>::Log(const param_type& p,
                                            std::string* l) {
  l->append("EventPacket((");
  LogParam(p.id(), l);
  l->append("), Events(");
  LogParam(p.events(), l);
  l->append("))");
}

void ParamTraits<content::InputEvent>::Write(Message* m, const param_type& p) {
  WriteParam(m, p.id());
  WriteParam(m, !!p.payload());
  if (!p.valid())
    return;

  content::InputEvent::Payload::Type payload_type = p.payload()->GetType();
  WriteParam(m, payload_type);
  switch (payload_type) {
    case content::InputEvent::Payload::IPC_MESSAGE:
      WriteParam(m, *content::IPCInputEventPayload::Cast(p.payload()));
      break;
    default:
      break;
  }
}

bool ParamTraits<content::InputEvent>::Read(const Message* m,
                                            PickleIterator* iter,
                                            param_type* p) {
  int64 id;
  bool has_payload = false;
  content::InputEvent::Payload::Type payload_type;
  if (!ReadParam(m, iter, &id) ||
      !ReadParam(m, iter, &has_payload) ||
      !id ||
      !has_payload ||
      !ReadParam(m, iter, &payload_type))
    return false;

  scoped_ptr<content::InputEvent::Payload> payload;
  switch (payload_type) {
    case content::InputEvent::Payload::IPC_MESSAGE:
      payload = ReadPayload<content::IPCInputEventPayload>(m, iter);
      break;
    default:
      NOTREACHED() << "Invalid InputEvent::Payload type.";
      return false;
  }
  return p->Initialize(id, payload.Pass());
}

void ParamTraits<content::InputEvent>::Log(const param_type& p,
                                           std::string* l) {
  l->append("InputEvent((");
  LogParam(p.id(), l);
  l->append("), Payload (");
  const content::InputEvent::Payload* payload = p.payload();
  if (payload) {
    switch (payload->GetType()) {
      case content::InputEvent::Payload::IPC_MESSAGE:
        LogParam(*content::IPCInputEventPayload::Cast(payload), l);
        break;
      default:
        NOTREACHED() << "Invalid InputEvent::Payload type.";
        l->append("INVALID");
        break;
    }
  } else {
    l->append("NULL");
  }
  l->append("))");
}

}  // namespace IPC
