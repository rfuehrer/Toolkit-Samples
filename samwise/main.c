/** @file main.c
 *
 * @brief Demonstrate the toolkit's Samwise implementation.
 *
 * @copyright Copyright (C) 2019, ISARA Corporation
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "iqr_context.h"
#include "iqr_samwise.h"
#include "iqr_hash.h"
#include "iqr_retval.h"
#include "iqr_rng.h"
#include "isara_samples.h"

#include "internal.h"

// ---------------------------------------------------------------------------------------------------------------------------------
// Document the command-line arguments.
// ---------------------------------------------------------------------------------------------------------------------------------

static const char *usage_msg =
"samwise [--dump] [--variant AES|ChaCha20]\n"
"        --dump Dumps the generated keys and secrets to file.\n"
"               Filenames:\n"
"                 Alice's key:    alice_key.dat\n"
"                 Bob's key:      bob_key.dat\n"
"                 Alice's secret: alice_secret.dat\n"
"                 Bob's secret:   bob_secret.dat\n"
"        --variant The variant of Samwise to use.\n"
"               Valid values are:\n"
"                 * AES\n"
"                 * ChaCha20\n";

// ---------------------------------------------------------------------------------------------------------------------------------
// This function showcases the use of the Samwise algorithm to generate a
// shared secret.
//
// This function assumes that all the parameters have already been validated.
// However, the function will exit early if there is a file system related
// failure.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval showcase_samwise(const iqr_Context *ctx, const iqr_RNG *rng, bool dump, const iqr_SamwiseVariant *variant)
{
    iqr_retval ret = init_comms();
    if (ret != IQR_OK) {
        return ret;
    }

    ret = init_alice(ctx, variant);
    if (ret != IQR_OK) {
        cleanup_comms();
        return ret;
    }
    ret = init_bob(ctx, variant);
    if (ret != IQR_OK) {
        cleanup_alice();
        cleanup_comms();
        return ret;
    }

    uint8_t alice_secret[IQR_SAMWISE_SECRET_SIZE] = { 0 };
    uint8_t bob_secret[IQR_SAMWISE_SECRET_SIZE] = { 0 };

    /* Alice must start the transfer. Bob cannot go first since, as the
     * responder, he needs information from Alice. For more information on how
     * the Samwise data protocol works see the README.md.
     */
    ret = alice_start(rng, dump);
    if (ret != IQR_OK) {
        goto end;
    }

    ret = bob_start(rng, dump);
    if (ret != IQR_OK) {
        goto end;
    }

    ret = alice_get_secret(alice_secret, sizeof(alice_secret));
    if (ret != IQR_OK) {
        goto end;
    }

    ret = bob_get_secret(bob_secret, sizeof(bob_secret));
    if (ret != IQR_OK) {
        goto end;
    }

    /* Test to make sure the secrets are the same */
    if (memcmp(alice_secret, bob_secret, sizeof(alice_secret)) == 0) {
        fprintf(stdout, "\nAlice and Bob's secrets match.\n\n");
    } else {
        fprintf(stdout, "\nAlice and Bob's secrets do NOT match.\n\n");
    }

    if (dump) {
        ret = save_data(ALICE_SECRET_FNAME, alice_secret, sizeof(alice_secret));
        if (ret != IQR_OK) {
            goto end;
        }
        ret = save_data(BOB_SECRET_FNAME, bob_secret, sizeof(bob_secret));
    }

end:
    /* These secrets are private, sensitive data, be sure to clear memory
     * containing them when you're done.
     */
    secure_memzero(alice_secret, sizeof(alice_secret));
    secure_memzero(bob_secret, sizeof(bob_secret));

    cleanup_alice();
    cleanup_bob();
    cleanup_comms();

    return ret;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// This next section of code is related to the toolkit, but is not specific to
// Samwise.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval init_toolkit(iqr_Context **ctx, iqr_RNG **rng)
{
    /* Create a Global Context. */
    iqr_retval ret = iqr_CreateContext(ctx);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_ContextCreate(): %s\n", iqr_StrError(ret));
        return ret;
    }

    /* This sets the hashing functions that will be used globally. */
    ret = iqr_HashRegisterCallbacks(*ctx, IQR_HASHALGO_SHA2_256, &IQR_HASH_DEFAULT_SHA2_256);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HashRegisterCallbacks(): %s\n", iqr_StrError(ret));
        return ret;
    }

    /* This lets us give satisfactory randomness to the algorithm. */
    ret = iqr_RNGCreateHMACDRBG(*ctx, IQR_HASHALGO_SHA2_256, rng);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_RNGCreateHMACDRBG(): %s\n", iqr_StrError(ret));
        return ret;
    }

    /* The seed should be initialized from a guaranteed entropy source. This is
     * only an example; DO NOT INITIALIZE THE SEED LIKE THIS.
     */
    time_t seed = time(NULL);

    ret = iqr_RNGInitialize(*rng, (uint8_t *)&seed, sizeof(seed));
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_RNGInitialize(): %s\n", iqr_StrError(ret));
        return ret;
    }

    return IQR_OK;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// These functions are designed to help the end user understand how to use
// this sample and hold little value to the developer trying to learn how to
// use the toolkit.
//---------------------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------------------
// Report the chosen runtime parameters.
// ---------------------------------------------------------------------------------------------------------------------------------

static void preamble(const char *cmd, bool dump, const iqr_SamwiseVariant *variant)
{
    fprintf(stdout, "Running %s with the following parameters...\n", cmd);
    fprintf(stdout, "    Dump data to files: ");
    if (dump) {
        fprintf(stdout, "True\n");
    } else {
        fprintf(stdout, "False\n");
    }

    fprintf(stdout, "    Variant: ");
    if (variant == &IQR_SAMWISE_976_AES) {
        fprintf(stdout, "AES\n");
    } else if (variant == &IQR_SAMWISE_976_CHACHA20) {
        fprintf(stdout, "ChaCha20\n");
    } else {
        fprintf(stdout, "Invalid\n");
    }

    fprintf(stdout, "\n");
}

static iqr_retval parse_commandline(int argc, const char **argv, bool *dump, const iqr_SamwiseVariant **variant)
{
    int i = 1;

    while (i != argc) {
        if (paramcmp(argv[i], "--dump") == 0) {
            *dump = true;
        } else if (paramcmp(argv[i], "--variant") == 0) {
            i++;
            if (paramcmp(argv[i], "AES") == 0) {
                *variant = &IQR_SAMWISE_976_AES;
            } else if (paramcmp(argv[i], "ChaCha20") == 0) {
                *variant = &IQR_SAMWISE_976_CHACHA20;
            } else {
                fprintf(stdout, "%s", usage_msg);
                return IQR_EBADVALUE;
            }
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
    const iqr_SamwiseVariant *variant = &IQR_SAMWISE_976_AES;
    bool dump = false;

    iqr_Context *ctx = NULL;
    iqr_RNG *rng = NULL;

    /* If the command line arguments were not sane, this function will return
     * an error.
     */
    iqr_retval ret = parse_commandline(argc, argv, &dump, &variant);
    if (ret != IQR_OK) {
        return EXIT_FAILURE;
    }

    /* Make sure the user understands what we are about to do. */
    preamble(argv[0], dump, variant);

    /* IQR initialization that is not specific to Samwise. */
    ret = init_toolkit(&ctx, &rng);
    if (ret != IQR_OK) {
        goto cleanup;
    }

    /* This function showcases the usage of Samwise. */
    ret = showcase_samwise(ctx, rng, dump, variant);

cleanup:
    /* Clean up. */
    iqr_RNGDestroy(&rng);
    iqr_DestroyContext(&ctx);
    return (ret == IQR_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
