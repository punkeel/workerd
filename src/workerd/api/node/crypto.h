#pragma once

#include <workerd/jsg/jsg.h>
#include <workerd/api/crypto.h>

namespace workerd::api::node {

class CryptoImpl final: public jsg::Object {
public:

  struct ExportOptions {
    jsg::Optional<kj::String> type;
    jsg::Optional<kj::String> format;
    jsg::Optional<kj::String> cipher;
    jsg::Optional<kj::OneOf<kj::String, kj::Array<kj::byte>>> passphrase;
    JSG_STRUCT(type, format, cipher, passphrase);
  };

  struct AsymmetricKeyDetails {
    jsg::Optional<uint32_t> modulusLength;
    jsg::Optional<uint64_t> publicExponent;
    jsg::Optional<kj::String> hashAlgorithm;
    jsg::Optional<kj::String> mgf1HashAlgorithm;
    jsg::Optional<uint32_t> saltLength;
    jsg::Optional<uint32_t> divisorLength;
    jsg::Optional<kj::String> namedCurve;
    JSG_STRUCT(modulusLength,
               publicExponent,
               hashAlgorithm,
               mgf1HashAlgorithm,
               saltLength,
               divisorLength,
               namedCurve);
  };

  struct GenerateKeyOptions {
    uint64_t length;
    JSG_STRUCT(length);
  };

  struct GenerateKeyPairOptions {
    jsg::Optional<uint32_t> modulusLength;
    jsg::Optional<uint64_t> publicExponent;
    jsg::Optional<kj::String> hashAlgorithm;
    jsg::Optional<kj::String> mgf1HashAlgorithm;
    jsg::Optional<uint32_t> saltLength;
    jsg::Optional<uint32_t> divisorLength;
    jsg::Optional<kj::String> namedCurve;
    jsg::Optional<kj::Array<kj::byte>> prime;
    jsg::Optional<uint32_t> primeLength;
    jsg::Optional<uint32_t> generator;
    jsg::Optional<kj::String> groupName;
    jsg::Optional<kj::String> paramEncoding; // one of either 'named' or 'explicit'
    jsg::Optional<ExportOptions> publicKeyEncoding;
    jsg::Optional<ExportOptions> privateKeyEncoding;

    JSG_STRUCT(modulusLength,
               publicExponent,
               hashAlgorithm,
               mgf1HashAlgorithm,
               saltLength,
               divisorLength,
               namedCurve,
               prime,
               primeLength,
               generator,
               groupName,
               paramEncoding,
               publicKeyEncoding,
               privateKeyEncoding);
  };

  struct CreatePrivateKeyOptions {
    v8::Local<v8::Value> key;
    jsg::Optional<kj::String> format; //
    // one of either 'pem', 'der', 'jwk'
    jsg::Optional<kj::String> type;
    // one of either 'pkcs1', 'pkcs8', 'sec1'
    jsg::Optional<kj::OneOf<kj::String, kj::Array<kj::byte>>> passphrase;
    jsg::Optional<kj::String> encoding;
    JSG_STRUCT(key, format, type, passphrase, encoding);
  };

  struct CreatePublicKeyOptions {
    v8::Local<v8::Value> key;
    jsg::Optional<kj::String> format;
    // one of either 'pem', 'der', 'jwk'
    jsg::Optional<kj::String> type;
    // one of either 'pkcs1', 'spki'
    jsg::Optional<kj::OneOf<kj::String, kj::Array<kj::byte>>> passphrase;
    jsg::Optional<kj::String> encoding;
    JSG_STRUCT(key, format, type, passphrase, encoding);
  };

  v8::Local<v8::Value> exportKey(
      jsg::Lock& js,
      jsg::Ref<CryptoKey> key,
      jsg::Optional<ExportOptions> options);

  bool equals(jsg::Lock& js, jsg::Ref<CryptoKey> key, jsg::Ref<CryptoKey> otherKey);

  AsymmetricKeyDetails getAsymmetricKeyDetail(jsg::Lock& js, jsg::Ref<CryptoKey> key);

  kj::String getAsymmetricKeyType(jsg::Lock& js, jsg::Ref<CryptoKey> key);

  jsg::Ref<CryptoKey> generateKey(jsg::Lock& js, kj::String type, GenerateKeyOptions options);
  CryptoKeyPair generateKeyPair(jsg::Lock& js, kj::String type, GenerateKeyPairOptions options);

  jsg::Ref<CryptoKey> createSecretKey(jsg::Lock& js,
                                      kj::OneOf<kj::String, kj::Array<kj::byte>>,
                                      jsg::Optional<kj::String> encoding);

  jsg::Ref<CryptoKey> createPrivateKey(jsg::Lock& js,
                                       kj::OneOf<
                                         CreatePrivateKeyOptions,
                                         kj::String,
                                         kj::Array<kj::byte>> options);

  jsg::Ref<CryptoKey> createPublicKey(jsg::Lock& js,
                                      kj::OneOf<
                                      CreatePublicKeyOptions,
                                      kj::String,
                                      kj::Array<kj::byte>> options);

  JSG_RESOURCE_TYPE(CryptoImpl) {
    JSG_METHOD(equals);
    JSG_METHOD(exportKey);
    JSG_METHOD(getAsymmetricKeyDetail);
    JSG_METHOD(getAsymmetricKeyType);
    JSG_METHOD(generateKey);
    JSG_METHOD(generateKeyPair);
    JSG_METHOD(createSecretKey);
    JSG_METHOD(createPublicKey);
  }
};

#define EW_NODE_CRYPTO_ISOLATE_TYPES                \
    api::node::CryptoImpl,                          \
    api::node::CryptoImpl::ExportOptions,           \
    api::node::CryptoImpl::AsymmetricKeyDetails,    \
    api::node::CryptoImpl::GenerateKeyOptions,      \
    api::node::CryptoImpl::GenerateKeyPairOptions,  \
    api::node::CryptoImpl::CreatePrivateKeyOptions, \
    api::node::CryptoImpl::CreatePublicKeyOptions
}  // namespace workerd::api::node
