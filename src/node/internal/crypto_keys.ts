// Copyright (c) 2017-2022 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0
//
// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

/* todo: the following is adopted code, enabling linting one day */
/* eslint-disable */

import { Buffer } from 'node-internal:internal_buffer';
import * as cryptoImpl from 'node-internal:crypto';
import {
  SecretKeyExportOptions,
  PublicKeyExportOptions,
  PrivateKeyExportOptions,
  ExportOptions,
  AsymmetricKeyDetails,
  AsymmetricKeyType,
  CreatePrivateKeyOptions,
  CreatePublicKeyOptions,
  GenerateKeyOptions,
  GenerateKeyPairOptions,
} from 'node-internal:crypto';
import {
  isUint8Array,
} from 'node-internal:internal_types';

import {
  ERR_INVALID_ARG_TYPE,
} from 'node-internal:internal_errors';

const kHandle = Symbol('kHandle');

// In Node.js, the definition of KeyObject is a bit complicated because
// KeyObject instances in Node.js can be transfered via postMessage() and
// structuredClone(), etc, allowing instances to be shared across multiple
// worker threads. We do not implement that model so we're esssentially
// re-implementing the Node.js API here instead of just taking their code.
// Also in Node.js, CryptoKey is layered on top of KeyObject since KeyObject
// existed first. We're, however, going to layer our KeyObject on top of
// CryptoKey with a few augmentations.

export abstract class KeyObject {
  [kHandle]: CryptoKey;

  constructor() {
    // KeyObjects cannot be created with new ... use one of the
    // create or generate methods, or use from to get from a
    // CryptoKey.
    throw new Error('Illegal constructor');
  }

  static from(key: CryptoKey) : KeyObject {
    if (!(key instanceof CryptoKey)) {
      throw new ERR_INVALID_ARG_TYPE('key', 'CryptoKey', key);
    }
    switch (key.type) {
      case 'secret':
        return Reflect.construct(function(this: SecretKeyObject) {
          this[kHandle] = key;
        }, [], SecretKeyObject);
      case 'private':
        return Reflect.construct(function(this: SecretKeyObject) {
          this[kHandle] = key;
        }, [], PrivateKeyObject);
      case 'public':
        return Reflect.construct(function(this: SecretKeyObject) {
          this[kHandle] = key;
        }, [], PublicKeyObject);
    }
  }

  export(options?: ExportOptions) : string | Buffer | JsonWebKey {
    const ret = cryptoImpl.exportKey(this[kHandle]!, options);
    if (typeof ret === 'string') return ret;
    if (isUint8Array(ret)) return Buffer.from(ret as Uint8Array);
    return ret;
  }

  equals(otherKeyObject: KeyObject) : boolean {
    if (this === otherKeyObject ||
        this[kHandle] === otherKeyObject[kHandle]) return true;
    if (this.type !== otherKeyObject.type) return false;
    if (!(otherKeyObject[kHandle] instanceof CryptoKey)) {
      throw new ERR_INVALID_ARG_TYPE('otherKeyObject', 'KeyObject', otherKeyObject);
    }
    return cryptoImpl.equals(this[kHandle], otherKeyObject[kHandle]);
  }

  abstract get type() : 'secret' | 'public' | 'private';
}

abstract class AsymmetricKeyObject extends KeyObject {
  get asymmetricKeyDetails() : AsymmetricKeyDetails {
    return cryptoImpl.getAsymmetricKeyDetail(this[kHandle]);
  }

  get asymmetricKeyType() : AsymmetricKeyType {
    return cryptoImpl.getAsymmetricKeyType(this[kHandle]);
  }
}

export class PublicKeyObject extends AsymmetricKeyObject {
  override export(options?: PublicKeyExportOptions) : string | Buffer | JsonWebKey {
    return super.export(options);
  }

  get type() : 'public' { return 'public'; }
}

export class PrivateKeyObject extends AsymmetricKeyObject {
  override export(options?: PrivateKeyExportOptions) : string | Buffer | JsonWebKey {
    return super.export(options);
  }

  get type() : 'private' { return 'private'; }
}

export class SecretKeyObject extends KeyObject {
  get symmetricKeySize() : number {
    const inner = this[kHandle]!;
    return (inner.algorithm as any).length | 0;
  }

  override export(options?: SecretKeyExportOptions) : string | Buffer | JsonWebKey {
    return super.export(options);
  }

  get type() : 'secret' {
    return 'secret';
  }
}

export function createSecretKey(key : string | ArrayBuffer | Buffer | ArrayBufferView,
                                encoding?: string) : SecretKeyObject {
  return KeyObject.from(cryptoImpl.createSecretKey(key, encoding)) as SecretKeyObject;
}

export function createPrivateKey(key: CreatePrivateKeyOptions |
                                      string |
                                      ArrayBuffer |
                                      ArrayBufferView) : PrivateKeyObject {
  return KeyObject.from(cryptoImpl.createPrivateKey(key)) as PrivateKeyObject;
}

export function createPublicKey(key: CreatePublicKeyOptions |
                                     string |
                                     ArrayBuffer |
                                     ArrayBufferView)
    : PublicKeyObject {
  return KeyObject.from(cryptoImpl.createPublicKey(key)) as PublicKeyObject;
}

export function generateKey(type: 'hmac' | 'aes',
                            options: GenerateKeyOptions,
                            callback: (err?: any, key?: KeyObject) => void) {
  new Promise<CryptoKey>((resolve, reject) => {
    try {
      resolve(cryptoImpl.generateKey(type, options));
    } catch (err) {
      reject(err);
    }
  }).then((inner: CryptoKey) => {
    callback(null, KeyObject.from(inner));
  }, (err: unknown) => {
    callback(err);
  });
}

export function generateKeySync(type: 'hmac' | 'aes',
                                options: GenerateKeyOptions) {
  return KeyObject.from(cryptoImpl.generateKey(type, options));
}

export function generateKeyPair(
    type : AsymmetricKeyType,
    options: GenerateKeyPairOptions,
    callback: (err?: unknown,
               publicKey?: string | Buffer | PublicKeyObject,
               privateKey?: string | Buffer | PrivateKeyObject) => void) {
  new Promise<CryptoKeyPair>((resolve, reject) => {
    try {
      resolve(cryptoImpl.generateKeyPair(type, options));
    } catch (err) {
      reject(err);
    }
  }).then((pair: CryptoKeyPair) => {
    callback(null,
             KeyObject.from(pair.publicKey) as PublicKeyObject,
             KeyObject.from(pair.privateKey) as PrivateKeyObject);
  }, (err: unknown) => {
    callback(err);
  });
}

export interface KeyPair {
  publicKey: string | Buffer | PublicKeyObject;
  privateKey: string | Buffer | PrivateKeyObject;
}

export function generateKeyPairSync(
    type : AsymmetricKeyType,
    options: GenerateKeyPairOptions) : KeyPair {
  const {
    publicKey,
    privateKey,
  } = cryptoImpl.generateKeyPair(type, options);
  return {
    publicKey: KeyObject.from(publicKey) as PublicKeyObject,
    privateKey: KeyObject.from(privateKey) as PrivateKeyObject,
  };
}
