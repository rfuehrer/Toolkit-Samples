/** @file main.c
 *
 * @brief Demonstrate the toolkit's NTRUPrime key encapsulation mechanism.
 *
 * @copyright Copyright (C) 2017-2019, ISARA Corporation
 *
 * @license Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * <a href="http://www.apache.org/licenses/LICENSE-2.0">http://www.apache.org/licenses/LICENSE-2.0</a>
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "iqr_context.h"
#include "iqr_hash.h"
#include "iqr_ntruprime.h"
#include "iqr_retval.h"
#include "iqr_rng.h"
#include "isara_samples.h"

// ---------------------------------------------------------------------------------------------------------------------------------
// Document the command-line arguments.
// ---------------------------------------------------------------------------------------------------------------------------------

static const char *usage_msg =
"ntruprime_generate_keys [--pub <filename>] [--priv <filename>]\n"
"    Default for the sample (when no option is specified):\n"
"        --pub pub.key\n"
"        --priv priv.key\n";

// ---------------------------------------------------------------------------------------------------------------------------------
// This function showcases the generation of NTRUPrime public and
// private keys.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval showcase_ntruprime_key_gen(const iqr_NTRUPrimeParams *params, const iqr_RNG *rng,
    const char *pub_file, const char *priv_file)
{
    iqr_NTRUPrimePublicKey *pub = NULL;
    iqr_NTRUPrimePrivateKey *priv = NULL;

    uint8_t pub_raw[IQR_NTRUPRIME_PUBLIC_KEY_SIZE] = { 0 };
    uint8_t priv_raw[IQR_NTRUPRIME_PRIVATE_KEY_SIZE] = { 0 };

    fprintf(stdout, "Creating NTRUPrime key-pair.\n");

    iqr_retval ret = iqr_NTRUPrimeCreateKeyPair(params, rng, &pub, &priv);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_NTRUPrimeCreateKeyPair(): %s\n", iqr_StrError(ret));
        goto end;
    }
    fprintf(stdout, "NTRUPrime public and private key-pair has been created\n");

    ret = iqr_NTRUPrimeExportPublicKey(pub, pub_raw, sizeof(pub_raw));
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_NTRUPrimeExportPublicKey(): %s\n", iqr_StrError(ret));
        goto end;
    }

    fprintf(stdout, "Public key has been exported.\n");

    ret = iqr_NTRUPrimeExportPrivateKey(priv, priv_raw, sizeof(priv_raw));
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_NTRUPrimeExportPrivateKey(): %s\n", iqr_StrError(ret));
        goto end;
    }

    fprintf(stdout, "Private key has been exported.\n");

    /* And finally, write the public and private key to disk. */
    ret = save_data(pub_file, pub_raw, sizeof(pub_raw));
    if (ret != IQR_OK) {
        goto end;
    }

    ret = save_data(priv_file, priv_raw, sizeof(priv_raw));
    if (ret != IQR_OK) {
        goto end;
    }

    fprintf(stdout, "Public and private keys have been saved to disk.\n");

end:
    /* (Private) Keys are private, sensitive data, be sure to clear memory
     * containing them when you're done.
     */
    secure_memzero(priv_raw, sizeof(priv_raw));

    iqr_NTRUPrimeDestroyPublicKey(&pub);
    iqr_NTRUPrimeDestroyPrivateKey(&priv);

    return ret;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// This function showcases the creation of NTRUPrime parameter structure.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval showcase_ntruprime_params_creation(const iqr_Context *ctx, iqr_NTRUPrimeParams **params)
{
    /* Create NTRUPrime parameters. */
    iqr_retval ret = iqr_NTRUPrimeCreateParams(ctx, params);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_NTRUPrimeCreateParams(): %s\n", iqr_StrError(ret));
        return ret;
    }

    fprintf(stdout, "NTRUPrime parameter structure has been created.\n");

    return IQR_OK;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Initialize the toolkit by creating a context, registering hash
// algorithm, and creating a RNG object.
//
// For NTRUPrime IQR_HASHALGO_SHA2_512 must be registered.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval init_toolkit(iqr_Context **ctx, iqr_RNG **rng)
{
    /* Create a context. */
    iqr_retval ret = iqr_CreateContext(ctx);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_CreateContext(): %s\n", iqr_StrError(ret));
        return ret;
    }

    fprintf(stdout, "The context has been created.\n");

    /* Globally register the hashing functions. */
    ret = iqr_HashRegisterCallbacks(*ctx, IQR_HASHALGO_SHA2_512, &IQR_HASH_DEFAULT_SHA2_512);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HashRegisterCallbacks(): %s\n", iqr_StrError(ret));
        return ret;
    }

    fprintf(stdout, "Hash functions have been registered in the context.\n");

    /* Create an HMAC DRBG object. */
    ret = iqr_RNGCreateHMACDRBG(*ctx, IQR_HASHALGO_SHA2_512, rng);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_RNGCreateHMACDRBG(): %s\n", iqr_StrError(ret));
        return ret;
    }

    /* The seed should be initialized from a guaranteed entropy source. This is
     * only an example; DO NOT INITIALIZE THE SEED LIKE THIS.
     */
    time_t seed = time(NULL);

    ret = iqr_RNGInitialize(*rng, (void *) &seed, sizeof(seed));
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_RNGInitialize(): %s\n", iqr_StrError(ret));
        return ret;
    }

    fprintf(stdout, "RNG object has been created.\n");

    return IQR_OK;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// These functions are designed to help the end user use the sample or are
// generic utility functions. This section has little value to the developer
// trying to learn how to use the toolkit.
// ---------------------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------------------
// Report the chosen runtime parameters.
// ---------------------------------------------------------------------------------------------------------------------------------

static void preamble(const char *cmd, const char * pub, const char * priv)
{
    fprintf(stdout, "Running %s with the following parameters:\n", cmd);
    fprintf(stdout, "    public key file: %s\n", pub);
    fprintf(stdout, "    private key file: %s\n", priv);
}

/* Parse the command line options. */
static iqr_retval parse_commandline(int argc, const char **argv, const char **public_key_file, const char **private_key_file)
{
    int i = 1;
    while (i != argc) {
        if (paramcmp(argv[i], "--pub") == 0) {
            /* [--pub <filename>] */
            i++;
            *public_key_file = argv[i];
        } else if (paramcmp(argv[i], "--priv") == 0) {
            /* [--priv <filename>] */
            i++;
            *private_key_file = argv[i];
        } else {
            fprintf(stdout, "%s", usage_msg);
            return IQR_EBADVALUE;
        }
        i++;
    }

    return IQR_OK;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Executable entry point.
// ---------------------------------------------------------------------------------------------------------------------------------

int main(int argc, const char **argv)
{
    /* Default values.  Please adjust the usage message if you make changes
     * here.
     */
    const char *public_key_file = "pub.key";
    const char *private_key_file = "priv.key";

    iqr_Context * ctx = NULL;
    iqr_RNG *rng = NULL;
    iqr_NTRUPrimeParams *parameters = NULL;

    /* If the command line arguments were not sane, this function will return
     * an error.
     */
    iqr_retval ret = parse_commandline(argc, argv, &public_key_file, &private_key_file);
    if (ret != IQR_OK) {
        return EXIT_FAILURE;
    }

    /* Show the parameters for the program. */
    preamble(argv[0], public_key_file, private_key_file);

    /* IQR toolkit initialization. */
    ret = init_toolkit(&ctx, &rng);
    if (ret != IQR_OK) {
        goto cleanup;
    }

    /* Showcase the creation of NTRUPrime parameter structure. */
    ret = showcase_ntruprime_params_creation(ctx, &parameters);
    if (ret != IQR_OK) {
        goto cleanup;
    }

    /* Showcase the generation of NTRUPrime public/private keys. */
    ret = showcase_ntruprime_key_gen(parameters, rng, public_key_file, private_key_file);

cleanup:
    iqr_NTRUPrimeDestroyParams(&parameters);
    iqr_RNGDestroy(&rng);
    iqr_DestroyContext(&ctx);
    return (ret == IQR_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
