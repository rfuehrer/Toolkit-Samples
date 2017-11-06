# ISARA Radiate Security Solution Suite 1.3 Dilithium Samples
ISARA Corporation <info@isara.com>
v1.3 2017: Copyright (C) 2017 ISARA Corporation, All Rights Reserved.

## Introduction to Signature Schemes

In general, all signature schemes follow a similar pattern.  There is one
signer and any number of verifiers.

1.  The signer generates a public and private key pair.
2.  The signer publishes the public key but keeps the private key secret.
3.  The signer uses the private key to sign the digest of a message.
4.  The signer publishes the message and signature.
5.  A verifier obtains the public key, the message and the signature.
6.  A verifier reproduces the digest of the message and verifies it
    against the signature.

With the Dilithium signature scheme, keys only need to be generated once, then
an unlimited number of messages can be signed, and each message and signature
can be verified an unlimited number of times.

## Sample Applications

We have created 3 small sample applications that demonstrate how to use the
toolkit's Dilithium implementation:

* `dilithium_generate_keys` takes care of step 1.
* `dilithium_sign` takes care of step 3.
* `dilithium_verify` takes care of step 6.

The rest of the steps are left up to the underlying system.

As per [CRYSTALS -- Dilithium:](https://eprint.iacr.org/2017/633.pdf)
Digital Signatures from Module Lattices, the toolkit's Dilithium implementation
provides both 128-bit and 160-bit quantum security.

Here is the simplest way to use the samples:

Create a digital message and save it to a file called `message.dat`. For
example, on a lark, we used Project Gutenberg's Alice's Adventures in
Wonderland by Lewis Carroll. (It can be freely obtained from
[Project Gutenberg](http://www.gutenberg.org/ebooks/11.txt.utf-8).)
We downloaded the plaintext version and saved it as `message.dat` in the same
directory that contained the compiled executables of the samples.

Build the sample application:

```
$ cmake -DIQR_TOOLKIT_ROOT=/path/to/toolkitroot/ .
$ make
```

Execute `dilithium_generate_keys` using default parameters.

Execution and expected outputs:

```
$ ./dilithium_generate_keys
Running dilithium/generate_keys/dilithium_generate_keys with the following
parameters...
    security level: 128
    public key file: pub.key
    private key file: priv.key

Keys have been generated.
Public Key has been exported.
Private Key has been exported.
Successfully saved pub.key (1472 bytes)
Successfully saved priv.key (15816 bytes)
Public and private keys have been saved to disk.
```

Execute `dilithium_sign` using default parameters.

Execution and expected output:

```
$ ./dilithium_sign
Running dilithium/sign/dilithium_sign with the following parameters...
    security level: 128
    signature file: sig.dat
    private key file: priv.key
    message data file: message.dat

Successfully loaded priv.key (15816 bytes)
Private key has been imported.
Successfully loaded message.dat (35907 bytes)
Signature has been created.
Successfully saved sig.dat (2700 bytes)
Signature has been saved to disk.
```

Execute `dilithium_verify` using default parameters.

Execution and expected output:

```
$ ./dilithium_verify
Running dilithium/verify/dilithium_verify with the following parameters...
    security level: 128
    signature file: sig.dat
    public key file: pub.key
    message data file: message.dat

Successfully loaded pub.key (1472 bytes)
Successfully loaded message.dat (35907 bytes)
Successfully loaded sig.dat (2700 bytes)
Public key has been loaded successfully!
Dilithium verified the signature successfully!
```

## Sample Applications Usage Details

### dilithium_generate_key

Generates a new private key and public key and saves them to two separate
files.

Command line format:

```
dilithium_generate_keys [--security 128|160] [--pub <filename>]
[--priv <filename>]
```

Command line defaults:

```
--security 128
--pub pub.key
--priv priv.key
```

Command line parameter descriptions:

```
[--security 128|160]
The desired security level.

[--pub <filename>]
<filename> is the name of the file where the public key is to be saved.

[--priv <filename>]
<filename> is the name of the file where the private key is to be saved.
```

### dilithium_sign

Creates the digest of a message, signs the digest and saves the signature to a
file.

Command line format:

```
dilithium_sign [--security 128|160] [--sig filename] [--priv <filename>]
  [--message <filename>]
```

Command line defaults:

```
--security 128
--sig sig.dat
--priv priv.key
--message message.dat
```

Command line parameter descriptions:

```
[--security 128|160]
The desired security level.

[--sig <filename>]
<filename> is the name of the file where the signature is to be saved.

[--priv <filename>]
<filename> is the name of the file where the private key is stored.

[--message <filename>]
<filename> is the name of the file where the message is stored.
```

### dilithium_verify

Creates the digest of a message and verifies the signature against the digest.

Command line format:

```
dilithium_verify [--security 128|160] [--sig <filename>] [--pub <filename>]
  [--message <filename>]
```

Command line defaults:

```
--security 128
--sig sig.dat
--pub pub.key
--message message.dat
```

Command line parameter descriptions:

```
[--security 128|160]
The desired security level.

[--sig <filename>]
<filename> is the name of the file where the signature is stored.

[--pub <filename>]
<filename> is the name of the file where the public key is stored.

[--message <filename>]
<filename> is the name of the file where the message is stored.
```

## Further Reading

* See `iqr_dilithium.h` in the toolkit's `include` directory.

## License

See the `LICENSE` file for details:

> Copyright 2017 ISARA Corporation
> 
> Licensed under the Apache License, Version 2.0 (the "License");
> you may not use this file except in compliance with the License.
> You may obtain a copy of the License at
> 
> http://www.apache.org/licenses/LICENSE-2.0
> 
> Unless required by applicable law or agreed to in writing, software
> distributed under the License is distributed on an "AS IS" BASIS,
> WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
> See the License for the specific language governing permissions and
> limitations under the License.