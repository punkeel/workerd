// Copyright (c) 2017-2023 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#include "io-context.h"
#include <workerd/io/hibernation-manager.h>

namespace workerd {

HibernationManagerImpl::~HibernationManagerImpl() {
  // Note that the HibernatableWebSocket destructor handles removing any references to itself in
  // `tagToWs`, and even removes the hashmap entry if there are no more entries in the bucket.
  allWs.clear();
  KJ_ASSERT(tagToWs.size() == 0, "tagToWs hashmap wasn't cleared.");
  KJ_ASSERT(allWs.size() == 0 && allWs.empty(), "allWs collection wasn't cleared.");
}

void HibernationManagerImpl::acceptWebSocket(
    jsg::Ref<api::WebSocket> ws,
    kj::ArrayPtr<kj::String> tags) {
  // First, we create the HibernatableWebSocket and add it to the collection where it'll stay
  // until it's destroyed.

  // TODO(now): Need to make sure this error makes its way up to the worker script.
  KJ_REQUIRE(allWs.size() < 1024,
      "only 1024 websockets can be accepted on a single Durable Object instance");

  auto hib = kj::heap<HibernatableWebSocket>(kj::mv(ws), tags, *this);
  HibernatableWebSocket& refToHibernatable = *hib.get();
  allWs.push_front(kj::mv(hib));
  refToHibernatable.node = allWs.begin();

  // If the `tags` array is empty (i.e. user did not provide a tag), we skip the population of the
  // `tagToWs` HashMap below and go straight to initiating the readLoop.

  // It is the caller's responsibility to ensure all elements of `tags` are unique.
  // TODO(cleanup): Maybe we could enforce uniqueness by using an immutable type that
  // can only be constructed if the elements in the collection are distinct, ex. "DistinctArray".
  //
  // We need to add the HibernatableWebSocket to each bucket in `tagToWs` corresponding to its tags.
  //  1. Create the entry if it doesn't exist
  //  2. Fill the TagListItem in the HibernatableWebSocket's tagItems array
  size_t position = 0;
  for (auto tag = tags.begin(); tag < tags.end(); tag++, position++) {
    auto& tagCollection = tagToWs.findOrCreate(*tag, [this, &tag]() {
      // TODO(now): Need to make sure this error makes its way up to the worker script.
      KJ_REQUIRE(tagToWs.size() <= 4096, "too many Hibernatable WebSocket tags provided");

      auto item = kj::heap<TagCollection>(
          kj::mv(*tag), kj::heap<kj::List<TagListItem, &TagListItem::link>>());
      return decltype(tagToWs)::Entry {
          item->tag,
          kj::mv(item)
      };
    });
    // This TagListItem sits in the HibernatableWebSocket's tagItems array.
    auto& tagListItem = refToHibernatable.tagItems[position];
    tagListItem.hibWS = refToHibernatable;
    tagListItem.tag = tagCollection->tag.asPtr();

    auto& list = tagCollection->list;
    list->add(tagListItem);
    // We also give the TagListItem a reference to the list it was added to so the
    // HibernatableWebSocket can quickly remove itself from the list without doing a lookup
    // in `tagToWs`.
    tagListItem.list = *list.get();
  }

  // Finally, we initiate the readloop for this HibernatableWebSocket.
  kj::Promise<kj::Maybe<kj::Exception>> readLoopPromise = kj::evalNow([&] {
    return readLoop(refToHibernatable);
  }).then([]() -> kj::Maybe<kj::Exception> { return nullptr; },
          [](kj::Exception&& e) -> kj::Maybe<kj::Exception> { return kj::mv(e); });

  // Give the task to the HibernationManager so it lives long.
  readLoopTasks.add(readLoopPromise.then(
      [&refToHibernatable, this](kj::Maybe<kj::Exception>&& maybeError) {

    KJ_IF_MAYBE(error, maybeError) {
      webSocketForEventHandler = refToHibernatable;
      if (!refToHibernatable.hasDispatchedClose &&
          (error->getType() == kj::Exception::Type::DISCONNECTED)) {
        // If premature disconnect/cancel, dispatch a close event if we haven't already.
        auto params = api::HibernatableSocketParams(
            kj::str("WebSocket disconnected without sending Close frame."),
            1006,
            false);
        // TODO(now): Actually dispatch the event once we have loopback.
        // TODO(now): Call `closeIncoming()` AND `tryReleaseNative()` after the event runs
        // (in a continuation).
      } else {
        // Otherwise, we need to dispatch an error event!
        auto params = api::HibernatableSocketParams(kj::mv(*error));

        // TODO(now): Actually dispatch the event once we have loopback.
        // TODO(now): Call `closeIncoming()` after the event runs (in a continuation).
      }
    }
    return dropHibernatableWebSocket(refToHibernatable);
  }));
}

kj::Vector<jsg::Ref<api::WebSocket>> HibernationManagerImpl::getWebSockets(
    jsg::Lock& js,
    kj::Maybe<kj::StringPtr> maybeTag) {
  kj::Vector<jsg::Ref<api::WebSocket>> matches;
  KJ_IF_MAYBE(tag, maybeTag) {
    KJ_IF_MAYBE(item, tagToWs.find(*tag)) {
      auto& list = *((*item)->list);
      for (auto& entry: list) {
        auto& hibWS = KJ_REQUIRE_NONNULL(entry.hibWS);
        // If the websocket is hibernating, we have to create an api::WebSocket
        // and add it to the HibernatableWebSocket.
        if (hibWS.activeWebSocket == nullptr) {
          hibWS.unhibernate(js);
        }
        // Now that we know the websocket is "awake", we simply add it to the vector.
        KJ_IF_MAYBE(awake, hibWS.activeWebSocket) {
          matches.add(awake->addRef());
        }
      }
    }
  } else {
    // Add all websockets!
    for (auto& hibWS : allWs) {
      if (hibWS->activeWebSocket == nullptr) {
        hibWS->unhibernate(js);
      }
      // Now that we know the websocket is "awake", we simply add it to the vector.
      KJ_IF_MAYBE(awake, hibWS->activeWebSocket) {
        matches.add(awake->addRef());
      }
    }
  }
  return kj::mv(matches);
}

kj::Array<kj::byte> HibernationManagerImpl::serializeV8Value(
    v8::Local<v8::Value> value,
    v8::Isolate* isolate) {
  // TODO(now): Do we want omitHeader == false?
  jsg::Serializer serializer(isolate, jsg::Serializer::Options {
    .version = 15,
    .omitHeader = false,
  });
  serializer.write(value);
  auto released = serializer.release();
  return kj::mv(released.data);
}

v8::Local<v8::Value> HibernationManagerImpl::deserializeV8Value(
    kj::Array<kj::byte> buf,
    v8::Isolate* isolate) {
  // TODO(now): Do we want readHeader == true?
  jsg::Deserializer deserializer(isolate, buf.asPtr(), nullptr, nullptr, jsg::Deserializer::Options {
    .version = 15,
    .readHeader = true,
  });

  return deserializer.readValue();
}

void HibernationManagerImpl::hibernateWebSockets(
    jsg::Lock& js,
    v8::Isolate* isolate,
    IoContext& context) {

  for (auto& ws : allWs) {
    KJ_IF_MAYBE(active, ws->activeWebSocket) {
      // We need to serialize the attachment before hibernating.
      KJ_IF_MAYBE(attachment, active->get()->getAttachment()) {
        ws->attachment = serializeV8Value(*attachment, isolate);
      }
      // Note that we move these properties from api::WebSocket to the HibernatableWebSocket.
      auto p = active->get()->buildPackageForHibernation();
      ws->url = kj::mv(p.url);
      ws->protocol = kj::mv(p.protocol);
      ws->extensions = kj::mv(p.extensions);
    }
    ws->activeWebSocket = nullptr;
  }
}

void HibernationManagerImpl::dropHibernatableWebSocket(HibernatableWebSocket& hib) {
  removeFromAllWs(hib);
}

inline void HibernationManagerImpl::removeFromAllWs(HibernatableWebSocket& hib) {
  auto& node = KJ_REQUIRE_NONNULL(hib.node);
  allWs.erase(node);
}

kj::Promise<void> HibernationManagerImpl::readLoop(HibernatableWebSocket& hib) {
  // Like the api::WebSocket readLoop(), but we dispatch different types of events.
  auto& ws = *hib.ws;
  return ws.receive()
      .then([this, &hib] (kj::WebSocket::Message&& message) mutable -> kj::Promise<void> {
    // Note that errors are handled by the callee of `readLoop`, since we throw from `receive()`.
    webSocketForEventHandler = hib;

    // Build the event params depending on what type of message we got.
    kj::Maybe<api::HibernatableSocketParams> maybeParams;
    KJ_SWITCH_ONEOF(message) {
      KJ_CASE_ONEOF(text, kj::String) {
        maybeParams.emplace(kj::mv(text));
      }
      KJ_CASE_ONEOF(data, kj::Array<kj::byte>) {
        maybeParams.emplace(kj::mv(data));
      }
      KJ_CASE_ONEOF(close, kj::WebSocket::Close) {
        maybeParams.emplace(kj::mv(close.reason), close.code, true);
        // We will dispatch the close event, so let's mark our websocket as having done so to
        // prevent a situation where we dispatch it twice.
        hib.hasDispatchedClose = true;
        // TODO(now): We can't do this here, we (might) have to do it in the custom event
        // (close event), but after dispatching the event we need to call `tryReleaseNative()`.
      }
    }

    // TODO(now): Dispatch the event once we have loopback.

    auto params = kj::mv(KJ_REQUIRE_NONNULL(maybeParams));
    if (params.isCloseEvent()) {
      return kj::READY_NOW;
    }
    return readLoop(hib);
  });
}

}; // namespace workerd
