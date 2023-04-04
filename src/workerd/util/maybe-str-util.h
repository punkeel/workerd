// Copyright (c) 2017-2023 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#pragma once

#include <kj/common.h>
#include <kj/string.h>

namespace workerd {

inline kj::Maybe<kj::String> maybeDeepCopy(kj::Maybe<kj::StringPtr> str) {
  return str.map([](kj::StringPtr s) { return kj::str(s); });
}

}
