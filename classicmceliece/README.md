= ISARA Radiate(TM) Crypto Suite 2.0 ClassicMcEliece KEM
Samples =

## Introduction Key Encapsulation Mechanisms (KEM)

In general, all KEM schemes follow a similar pattern.

1.  The receiver generates a public and private key pair.
2.  The receiver publishes the public key but keeps the private key secret.
3.  A sender obtains the public key.
4.  That sender uses encapsulate to obtain a ciphertext and shared key.
5.  That sender sends the ciphertext to the receiver.
6.  The receiver obtains the ciphertext.
7.  The receiver uses decapsulate on the ciphertext to obtain the shared key.

The shared key can now be used for symmetric encryption. Note that the key has
already been passed through a Key Derivation Function (KDF) so it is not
necessary to do so again.

Generally speaking, public/private keys only need to be generated once and can
be re-used to create multiple shared secrets. The ClassicMcEliece KEM follows
this pattern.

## Getting Started

We have created 3 small sample applications that demonstrate how to use the
toolkit's ClassicMcEliece KEM implementation:

* `classicmceliece_generate_keys` takes care of step 1.
* `classicmceliece_encapsulate` takes care of step 4.
* `classicmceliece_decapsulate` takes care of step 7.

Build the sample application in a `build` directory:

```
$ mkdir build
$ cd build
$ cmake -DIQR_TOOLKIT_ROOT=/path/to/toolkitroot/ ..
$ make
```

Execute the samples with no arguments to use the default parameters, or use
`--help` to list the available options.

## Further Reading

* See `iqr_classicmceliece.h` in the toolkit's `include` directory.
* <a href="https://csrc.nist.gov/projects/post-quantum-cryptography/round-1-submissions">
  <em>Classic McEliece/em></a>.

## License

See the `LICENSE` file for details:

> Copyright (C) 2018-2019, ISARA Corporation
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

### Trademarks

ISARA Radiate(TM) is a trademark of ISARA Corporation.