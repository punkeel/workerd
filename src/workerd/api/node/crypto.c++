#include "crypto.h"
#include <workerd/api/crypto.h>

namespace workerd::api::node {

v8::Local<v8::Value> CryptoImpl::exportKey(
    jsg::Lock& js,
    jsg::Ref<CryptoKey> key,
    jsg::Optional<CryptoImpl::ExportOptions> options) {
  KJ_UNIMPLEMENTED("not implemented yet");
}

bool CryptoImpl::equals(
    jsg::Lock& js,
    jsg::Ref<CryptoKey> key,
    jsg::Ref<CryptoKey> otherKey) {
  KJ_UNIMPLEMENTED("not implemented yet");
}

CryptoImpl::AsymmetricKeyDetails CryptoImpl::getAsymmetricKeyDetail(
    jsg::Lock& js,
    jsg::Ref<CryptoKey> key) {
  KJ_UNIMPLEMENTED("not implemented yet");
}

kj::String CryptoImpl::getAsymmetricKeyType(
    jsg::Lock& js,
    jsg::Ref<CryptoKey> key) {
  KJ_UNIMPLEMENTED("not implemented yet");
}

jsg::Ref<CryptoKey> CryptoImpl::generateKey(
    jsg::Lock& js,
    kj::String type,
    GenerateKeyOptions options) {
  KJ_UNIMPLEMENTED("not implemented yet");
}

CryptoKeyPair CryptoImpl::generateKeyPair(
    jsg::Lock& js,
    kj::String type,
    GenerateKeyPairOptions options) {
  KJ_UNIMPLEMENTED("not implemented yet");
}

jsg::Ref<CryptoKey> CryptoImpl::createSecretKey(
    jsg::Lock& js,
    kj::OneOf<kj::String, kj::Array<kj::byte>>,
    jsg::Optional<kj::String> encoding) {
  KJ_UNIMPLEMENTED("not implemented yet");
}

jsg::Ref<CryptoKey> CryptoImpl::createPrivateKey(
    jsg::Lock& js,
    kj::OneOf<CreatePrivateKeyOptions, kj::String, kj::Array<kj::byte>> options) {
  KJ_UNIMPLEMENTED("not implemented yet");
}

jsg::Ref<CryptoKey> CryptoImpl::createPublicKey(
    jsg::Lock& js,
    kj::OneOf<CreatePublicKeyOptions, kj::String, kj::Array<kj::byte>> options) {
  KJ_UNIMPLEMENTED("not implemented yet");
}

}  // namespace workerd::api::node
