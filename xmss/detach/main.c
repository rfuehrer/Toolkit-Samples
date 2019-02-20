/** @file main.c
 *
 * @brief Detach a portion of the XMSS state into a separate file.
 *
 * @copyright Copyright (C) 2016-2019, ISARA Corporation
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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "iqr_context.h"
#include "iqr_hash.h"
#include "iqr_retval.h"
#include "iqr_rng.h"
#include "iqr_xmss.h"
#include "isara_samples.h"

// ---------------------------------------------------------------------------------------------------------------------------------
// Document the command-line arguments.
// ---------------------------------------------------------------------------------------------------------------------------------

static const char *usage_msg =
"xmss_detach [--priv <filename>] [--state <filename>]\n"
"    [--detached-state <filename>] [--num-sigs <number>] [--height 10|16|20]\n"
"    [--strategy cpu|memory|full]\n"
"    Defaults are: \n"
"        --priv priv.key\n"
"        --state priv.state\n"
"        --strategy full\n"
"        --height 10\n"
"        --detached-state detached.state\n"
"        --num-sigs 1\n";

// ---------------------------------------------------------------------------------------------------------------------------------
// This function showcases state detachment using the XMSS signature scheme.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval showcase_xmss_detach(const iqr_Context *ctx, iqr_XMSSHeight height, const iqr_XMSSTreeStrategy *strategy,
    const char *priv_file, const char *state_file, uint32_t num_signatures, const char *detached_state_file)
{
    iqr_XMSSParams *params = NULL;
    iqr_XMSSPrivateKey *priv = NULL;
    iqr_XMSSPrivateKeyState *state = NULL;
    iqr_XMSSPrivateKeyState *detached_state = NULL;

    size_t priv_raw_size = 0;
    uint8_t *priv_raw = NULL;

    size_t state_raw_size = 0;
    uint8_t *state_raw = NULL;

    size_t detached_state_raw_size = 0;
    uint8_t *detached_state_raw = NULL;

    uint32_t max_sigs = 0;
    uint32_t remaining_sigs = 0;
    uint32_t detached_remaining_sigs = 0;

    iqr_retval ret = iqr_XMSSCreateParams(ctx, strategy, height, &params);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_XMSSCreateParams(): %s\n", iqr_StrError(ret));
        goto end;
    }

    /* Load the raw private key. */
    ret = load_data(priv_file, &priv_raw, &priv_raw_size);
    if (ret != IQR_OK) {
        goto end;
    }

    /* Load the private key state. */
    ret = load_data(state_file, &state_raw, &state_raw_size);
    if (ret != IQR_OK) {
        goto end;
    }

    ret = iqr_XMSSImportPrivateKey(params, priv_raw, priv_raw_size, &priv);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_XMSSImportPrivateKey(): %s\n", iqr_StrError(ret));
        goto end;
    }

    fprintf(stdout, "Private key has been imported.\n");

    ret = iqr_XMSSImportState(params, state_raw, state_raw_size, &state);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_XMSSImportState(): %s\n", iqr_StrError(ret));
        goto end;
    }

    fprintf(stdout, "Private key state has been imported.\n");

    ret = iqr_XMSSDetachState(priv, state, num_signatures, &detached_state);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_XMSSDetachState(): %s\n", iqr_StrError(ret));
        goto end;
    }

    ret = iqr_XMSSGetSignatureCount(state, &max_sigs, &remaining_sigs);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_XMSSGetSignatureCount() using the original state: %s\n", iqr_StrError(ret));
        goto end;
    }

    ret = iqr_XMSSGetSignatureCount(detached_state, &max_sigs, &detached_remaining_sigs);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_XMSSGetSignatureCount() using the detached state: %s\n", iqr_StrError(ret));
        goto end;
    }

    fprintf(stdout, "Original state has %u signatures remaining.\n", remaining_sigs);
    fprintf(stdout, "Detached state has %u signatures remaining.\n", detached_remaining_sigs);

    /* Export the updated original state. */
    ret = iqr_XMSSExportState(state, state_raw, state_raw_size);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_XMSSExportState(): %s\n", iqr_StrError(ret));
        goto end;
    }

    ret = save_data(state_file, state_raw, state_raw_size);
    if (ret != IQR_OK) {
        goto end;
    }

    /* Export the newly detached state. */
    ret = iqr_XMSSGetStateSize(params, &detached_state_raw_size);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_XMSSGetStateSize(): %s\n", iqr_StrError(ret));
        goto end;
    }

    detached_state_raw = calloc(1, detached_state_raw_size);
    if (detached_state_raw == NULL) {
        fprintf(stderr, "Failed on calloc(): %s\n", strerror(errno));
        ret = IQR_ENOMEM;
        goto end;
    }

    ret = iqr_XMSSExportState(detached_state, state_raw, state_raw_size);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_XMSSExportState(): %s\n", iqr_StrError(ret));
        goto end;
    }

    ret = save_data(detached_state_file, state_raw, state_raw_size);
    if (ret != IQR_OK) {
        goto end;
    }

end:
    if (priv_raw != NULL) {
        /* (Private) Keys are private, sensitive data, be sure to clear memory
         * containing them when you're done.
         */
        secure_memzero(priv_raw, priv_raw_size);
    }
    free(priv_raw);
    free(state_raw);
    free(detached_state_raw);

    iqr_XMSSDestroyPrivateKey(&priv);
    iqr_XMSSDestroyState(&state);
    iqr_XMSSDestroyState(&detached_state);
    iqr_XMSSDestroyParams(&params);

    return ret;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// This next section of code is related to the toolkit, but is not specific to
// XMSS.
// ---------------------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------------------
// This function takes a message buffer and creates a digest out of it.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval init_toolkit(iqr_Context **ctx)
{
    /* Create a Global Context. */
    iqr_retval ret = iqr_CreateContext(ctx);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_CreateContext(): %s\n", iqr_StrError(ret));
        return ret;
    }

    /* This sets the hashing functions that will be used globally. */
    ret = iqr_HashRegisterCallbacks(*ctx, IQR_HASHALGO_SHA2_256, &IQR_HASH_DEFAULT_SHA2_256);
    if (IQR_OK != ret) {
        fprintf(stderr, "Failed on iqr_HashRegisterCallbacks(): %s\n", iqr_StrError(ret));
        return ret;
    }

    return IQR_OK;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// These functions are designed to help the end user understand how to use
// this sample and hold little value to the developer trying to learn how to
// use the toolkit.
// ---------------------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------------------
// Report the chosen runtime parameters.
// ---------------------------------------------------------------------------------------------------------------------------------

static void preamble(const char *cmd, const char *priv, const char *state, iqr_XMSSHeight height,
    const iqr_XMSSTreeStrategy *strategy, uint32_t num_sigs, const char *detached_state)
{
    fprintf(stdout, "Running %s with the following parameters...\n", cmd);
    fprintf(stdout, "    private key file: %s\n", priv);
    fprintf(stdout, "    private key state file: %s\n", state);
    fprintf(stdout, "    private key detached state file: %s\n", detached_state);
    fprintf(stdout, "    detaching %u signatures\n", num_sigs);

    if (IQR_XMSS_HEIGHT_10 == height) {
        fprintf(stdout, "    height: IQR_XMSS_HEIGHT_10\n");
    } else if (IQR_XMSS_HEIGHT_16 == height) {
        fprintf(stdout, "    height: IQR_XMSS_HEIGHT_16\n");
    } else if (IQR_XMSS_HEIGHT_20 == height) {
        fprintf(stdout, "    height: IQR_XMSS_HEIGHT_20\n");
    } else {
        fprintf(stdout, "    height: INVALID\n");
    }

    if (strategy == &IQR_XMSS_FULL_TREE_STRATEGY) {
        fprintf(stdout, "    strategy: Full Tree\n");
    } else if (strategy == &IQR_XMSS_MEMORY_CONSTRAINED_STRATEGY) {
        fprintf(stdout, "    strategy: Memory Constrained\n");
    } else if (strategy == &IQR_XMSS_CPU_CONSTRAINED_STRATEGY) {
        fprintf(stdout, "    strategy: CPU Constrained\n");
    } else {
        fprintf(stdout, "    strategy: INVALID\n");
    }

    fprintf(stdout, "\n");
}

static iqr_retval parse_commandline(int argc, const char **argv, const char **priv, const char **state, iqr_XMSSHeight *height,
    const iqr_XMSSTreeStrategy **strategy, uint32_t *num_signatures, const char **detached_state)
{
    int i = 1;
    while (i != argc) {
        if (i + 2 > argc) {
            fprintf(stdout, "%s", usage_msg);
            return IQR_EBADVALUE;
        }

        if (paramcmp(argv[i], "--priv") == 0) {
            /* [--priv <filename>] */
            i++;
            *priv = argv[i];
        } else if (paramcmp(argv[i], "--state") == 0) {
            /* [--state <filename>] */
            i++;
            *state = argv[i];
        } else if (paramcmp(argv[i], "--detached-state") == 0) {
            /* [--detached-state <filename>] */
            i++;
            *detached_state = argv[i];
        } else if (paramcmp(argv[i], "--height") == 0) {
            /* [--height 10|16|20] */
            i++;
            if  (paramcmp(argv[i], "10") == 0) {
                *height = IQR_XMSS_HEIGHT_10;
            } else if  (paramcmp(argv[i], "16") == 0) {
                *height = IQR_XMSS_HEIGHT_16;
            } else if  (paramcmp(argv[i], "20") == 0) {
                *height = IQR_XMSS_HEIGHT_20;
            } else {
                fprintf(stdout, "%s", usage_msg);
                return IQR_EBADVALUE;
            }
        } else if (paramcmp(argv[i], "--strategy") == 0) {
            /* [--strategy cpu|memory|full] */
            i++;
            if (paramcmp(argv[i], "cpu") == 0) {
                *strategy = &IQR_XMSS_CPU_CONSTRAINED_STRATEGY;
            } else if (paramcmp(argv[i], "memory") == 0) {
                *strategy = &IQR_XMSS_MEMORY_CONSTRAINED_STRATEGY;
            } else if (paramcmp(argv[i], "full") == 0) {
                *strategy = &IQR_XMSS_FULL_TREE_STRATEGY;
            } else {
                fprintf(stdout, "%s", usage_msg);
                return IQR_EBADVALUE;
            }
        } else if (paramcmp(argv[i], "--num-sigs") == 0) {
            /* [--num-sigs <number>] */
            i++;

            char *end = NULL;
            const uint64_t val = strtoull(argv[i], &end, 10);
            if (end == argv[i] || *end != '\0' || (val == ULLONG_MAX && errno == ERANGE)) {
                fprintf(stdout, "%s", usage_msg);
                return IQR_EBADVALUE;
            }
            *num_signatures = (uint32_t)val;
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
     *  here.
     */
    const char *priv = "priv.key";
    const char *state = "priv.state";
    const char *detached_state = "detached.state";
    const iqr_XMSSTreeStrategy *strategy = &IQR_XMSS_FULL_TREE_STRATEGY;
    iqr_XMSSHeight height = IQR_XMSS_HEIGHT_10;
    uint32_t num_sigs = 1;

    iqr_Context *ctx = NULL;

    /* If the command line arguments were not sane, this function will return
     * an error.
     */
    iqr_retval ret = parse_commandline(argc, argv, &priv, &state, &height, &strategy, &num_sigs, &detached_state);
    if (ret != IQR_OK) {
        return EXIT_FAILURE;
    }

    /* Make sure the user understands what we are about to do. */
    preamble(argv[0], priv, state, height, strategy, num_sigs, detached_state);

    /* IQR initialization that is not specific to XMSS. */
    ret = init_toolkit(&ctx);
    if (ret != IQR_OK) {
        goto cleanup;
    }

    /* This function showcases the usage of XMSS signing.
     */
    ret = showcase_xmss_detach(ctx, height, strategy, priv, state, num_sigs, detached_state);

cleanup:
    iqr_DestroyContext(&ctx);
    return (ret == IQR_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}