/** @file main.c
 *
 * @brief Sign a message using the toolkit's HSS signature scheme.
 *
 * @copyright Copyright 2016-2018 ISARA Corporation
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
#include <stdio.h>
#include <stdlib.h>
// Declare memset_s() if the platform supports it.
#if !defined(__ANDROID__)
#define __STDC_WANT_LIB_EXT1__ 1
#endif
#include <string.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
// For SecureZeroMemory().
#include <Windows.h>
#endif

#if defined(__FreeBSD__)
// For explicit_bzero().
#include <strings.h>
#endif

#include "iqr_context.h"
#include "iqr_hash.h"
#include "iqr_hss.h"
#include "iqr_retval.h"
#include "iqr_rng.h"

// ---------------------------------------------------------------------------------------------------------------------------------
// Function Declarations.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval save_data(const char *fname, const uint8_t *data, size_t data_size);
static iqr_retval load_data(const char *fname, uint8_t **data, size_t *data_size);
static void secure_memzero(void *b, size_t len);

// ---------------------------------------------------------------------------------------------------------------------------------
// This function showcases signing of a digest using the HSS signature scheme.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval showcase_hss_sign(const iqr_Context *ctx, const iqr_RNG *rng, iqr_HSSWinternitz w, iqr_HSSHeight height,
    const iqr_HSSTreeStrategy *strategy, const uint8_t *digest, const char *priv_file, const char *state_file,
    const char *sig_file)
{
    iqr_HSSParams *params = NULL;
    iqr_HSSPrivateKey *priv = NULL;
    iqr_HSSPrivateKeyState *state = NULL;

    size_t priv_raw_size = 0;
    uint8_t *priv_raw = NULL;

    size_t sig_size = 0;
    uint8_t *sig = NULL;

    size_t state_raw_size = 0;
    uint8_t *state_raw = NULL;

    uint32_t max_sigs = 0;
    uint32_t remaining_sigs = 0;

    iqr_retval ret = iqr_HSSCreateParams(ctx, strategy, w, height, IQR_HSS_LEVEL_1, &params);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HSSCreateParams(): %s\n", iqr_StrError(ret));
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

    ret = iqr_HSSImportPrivateKey(params, priv_raw, priv_raw_size, &priv);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HSSImportPrivateKey(): %s\n", iqr_StrError(ret));
        goto end;
    }

    fprintf(stdout, "Private key has been imported.\n");

    ret = iqr_HSSImportState(params, state_raw, state_raw_size, &state);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HSSImportState(): %s\n", iqr_StrError(ret));
        goto end;
    }

    fprintf(stdout, "Private key state has been imported.\n");

    /* Determine the size of the resulting signature and allocate memory. */
    ret = iqr_HSSGetSignatureSize(params, &sig_size);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HSSGetSignatureSize(): %s\n", iqr_StrError(ret));
        goto end;
    }

    sig = calloc(1, sig_size);
    if (sig == NULL) {
        fprintf(stderr, "Failed on calloc(): %s\n", strerror(errno));
        ret = IQR_ENOMEM;
        goto end;
    }

    ret = iqr_HSSSign(priv, rng, digest, IQR_SHA2_512_DIGEST_SIZE, state, sig, sig_size);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HSSSign(): %s\n", iqr_StrError(ret));
        goto end;
    }

    fprintf(stdout, "Signature has been created.\n");

    /* IMPORTANT: Save the state to disk prior to saving the signature. This
     * mirrors the real world usage pattern where you must persist the state
     * prior to using the signature in order to avoid one-time-signature
     * reuse if something goes wrong.
     */
    ret = iqr_HSSExportState(state, state_raw, state_raw_size);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HSSExportState(): %s\n", iqr_StrError(ret));
        goto end;
    }

    /* Save the updated state. */
    ret = save_data(state_file, state_raw, state_raw_size);
    if (ret != IQR_OK) {
        goto end;
    }

    /* And finally, write the signature to disk. */
    ret = save_data(sig_file, sig, sig_size);
    if (ret != IQR_OK) {
        goto end;
    }

    fprintf(stdout, "Signature and updated state have been saved to disk.\n");

    ret = iqr_HSSGetSignatureCount(state, &max_sigs, &remaining_sigs);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HSSGetMaximumSignatureCount(): %s\n", iqr_StrError(ret));
        goto end;
    }

    fprintf(stdout, "Number of signatures for this state: %d.\nRemaining signatures: %d\n", max_sigs, remaining_sigs);

    if (remaining_sigs == 0) {
        fprintf(stderr, "The private key cannot sign any more messages.\n");
    }

end:
    if (priv_raw != NULL) {
        /* (Private) Keys are private, sensitive data, be sure to clear memory
         * containing them when you're done.
         */
        secure_memzero(priv_raw, priv_raw_size);
    }
    free(sig);
    free(priv_raw);
    free(state_raw);

    iqr_HSSDestroyPrivateKey(&priv);
    iqr_HSSDestroyState(&state);
    iqr_HSSDestroyParams(&params);

    return ret;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// This next section of code is related to the toolkit, but is not specific to
// HSS.
// ---------------------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------------------
// This function takes a message buffer and creates a digest out of it.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval create_digest(const iqr_Context *ctx, uint8_t *data, size_t data_size, uint8_t *out_digest)
{
    iqr_Hash *hash = NULL;
    iqr_retval ret = iqr_HashCreate(ctx, IQR_HASHALGO_SHA2_512, &hash);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HashCreate(): %s\n", iqr_StrError(ret));
        return ret;
    }

    ret = iqr_HashMessage(hash, data, data_size, out_digest, IQR_SHA2_512_DIGEST_SIZE);
    if (ret != IQR_OK) {
        fprintf(stderr, "Failed on iqr_HashMessage(): %s\n", iqr_StrError(ret));
        iqr_HashDestroy(&hash);
        return ret;
    }

    iqr_HashDestroy(&hash);
    return IQR_OK;
}

static iqr_retval init_toolkit(iqr_Context **ctx, iqr_RNG **rng, const char *message, uint8_t **digest)
{
    uint8_t *message_raw = NULL;
    size_t message_raw_size = 0;

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

    /* SHA2-512 produces a 64-byte digest, which is required by iqr_HSSSign. Any
     * 64-byte digest is suitable for signing.
     */
    ret = iqr_HashRegisterCallbacks(*ctx, IQR_HASHALGO_SHA2_512, &IQR_HASH_DEFAULT_SHA2_512);
    if (IQR_OK != ret) {
        fprintf(stderr, "Failed on iqr_HashRegisterCallbacks(): %s\n", iqr_StrError(ret));
        return ret;
    }

    /* This will allow us to give satisfactory randomness to the algorithm. */
    ret =  iqr_RNGCreateHMACDRBG(*ctx, IQR_HASHALGO_SHA2_256, rng);
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

    /* Before we do any more work, lets make sure we can load the message
     * file.
     */
    ret = load_data(message, &message_raw, &message_raw_size);
    if (ret != IQR_OK) {
        return ret;
    }
    if (message_raw_size < 1) {
        fprintf(stderr, "Input message must be one or more bytes long.\n");
        return IQR_EINVBUFSIZE;
    }

    *digest = calloc(1, IQR_SHA2_512_DIGEST_SIZE);
    if (NULL == *digest) {
        fprintf(stderr, "Failed to allocate space for the digest\n");
        free(message_raw);
        return IQR_ENOMEM;
    }

    /* Calculate the digest */
    ret = create_digest(*ctx, message_raw, message_raw_size, *digest);
    if (ret != IQR_OK) {
        free(message_raw);
        free(*digest);
        *digest = NULL;
        return ret;
    }

    free(message_raw);
    return IQR_OK;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// These functions are designed to help the end user understand how to use
// this sample and hold little value to the developer trying to learn how to
// use the toolkit.
// ---------------------------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------------------------
// Generic POSIX file stream I/O operations.
// ---------------------------------------------------------------------------------------------------------------------------------

static iqr_retval save_data(const char *fname, const uint8_t *data, size_t data_size)
{
    FILE *fp = fopen(fname, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s: %s\n", fname, strerror(errno));
        return IQR_EBADVALUE;
    }

    iqr_retval ret = IQR_OK;
    fwrite(data, data_size, 1, fp);
    if (ferror(fp) != 0) {
        fprintf(stderr, "Failed on fwrite(): %s\n", strerror(errno));
        ret = IQR_EBADVALUE;
        goto end;
    }

    fprintf(stdout, "Successfully saved %s (%zu bytes)\n", fname, data_size);

end:
    fclose(fp);
    fp = NULL;
    return ret;
}

static iqr_retval load_data(const char *fname, uint8_t **data, size_t *data_size)
{
    iqr_retval ret = IQR_OK;

    FILE *fp = fopen(fname, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s: %s\n", fname, strerror(errno));
        return IQR_EBADVALUE;
    }

    /* Obtain file size. */
    fseek(fp , 0 , SEEK_END);
#if defined(_WIN32) || defined(_WIN64)
    const int64_t tmp_size64 = (int64_t)_ftelli64(fp);

    if (tmp_size64 < 0) {
        fprintf(stderr, "Failed on _ftelli64(): %s\n", strerror(errno));
        ret = IQR_EBADVALUE;
        goto end;
    } else if ((uint64_t)tmp_size64 > (uint64_t)SIZE_MAX) {
        /* On 32-bit systems, we cannot allocate enough memory for large key files. */
        ret = IQR_ENOMEM;
        goto end;
    }

    /* Due to a bug in GCC 7.2, it is necessary to make tmp_size volatile. Otherwise,
     * the variable is removed by the compiler and tmp_size64 is used instead. This
     * causes the calloc() call further down to raise a compiler warning. */
    volatile size_t tmp_size = (size_t)tmp_size64;
#else
    const size_t tmp_size = (size_t)ftell(fp);
#endif
    if (ferror(fp) != 0) {
        fprintf(stderr, "Failed on ftell(): %s\n", strerror(errno));
        ret = IQR_EBADVALUE;
        goto end;
    }

    rewind(fp);

    if (tmp_size > 0) {
        uint8_t *tmp = NULL;

        /* calloc with a param of 0 could return a pointer or NULL depending on
         * implementation, so skip all this when the size is 0 so we
         * consistently return NULL with a size of 0. In some samples it's
         * useful to take empty files as input so users can pass NULL or 0 for
         * optional parameters.
         */
        tmp = calloc(1, tmp_size);
        if (tmp == NULL) {
            fprintf(stderr, "Failed on calloc(): %s\n", strerror(errno));
            ret = IQR_EBADVALUE;
            goto end;
        }

        size_t read_size = fread(tmp, 1, tmp_size, fp);
        if (read_size != tmp_size) {
            fprintf(stderr, "Failed on fread(): %s\n", strerror(errno));
            free(tmp);
            tmp = NULL;
            ret = IQR_EBADVALUE;
            goto end;
        }

        *data_size = tmp_size;
        *data = tmp;

        fprintf(stdout, "Successfully loaded %s (%zu bytes)\n", fname, *data_size);
    }

end:
    fclose(fp);
    fp = NULL;
    return ret;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Tell the user about the command-line arguments.
// ---------------------------------------------------------------------------------------------------------------------------------

static void usage(void)
{
    fprintf(stdout, "hss_sign [--sig <filename>] [--priv <filename>] [--state <filename>]\n"
        "  [--winternitz 1|2|4|8] [--height 5|10|15|20|25] [--strategy bds|full]\n"
        "  [--message <filename>]\n");
    fprintf(stdout, "    Defaults are: \n");
    fprintf(stdout, "        --sig sig.dat\n");
    fprintf(stdout, "        --priv priv.key\n");
    fprintf(stdout, "        --state priv.state\n");
    fprintf(stdout, "        --strategy full\n");
    fprintf(stdout, "        --winternitz 4\n");
    fprintf(stdout, "        --height 5\n");
    fprintf(stdout, "        --message message.dat\n");
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Report the chosen runtime parameters.
// ---------------------------------------------------------------------------------------------------------------------------------

static void preamble(const char *cmd, const char *sig, const char *priv, const char *state, iqr_HSSWinternitz w,
    iqr_HSSHeight height, const iqr_HSSTreeStrategy *strategy, const char *message)
{
    fprintf(stdout, "Running %s with the following parameters...\n", cmd);
    fprintf(stdout, "    signature file: %s\n", sig);
    fprintf(stdout, "    private key file: %s\n", priv);
    fprintf(stdout, "    private key state file: %s\n", state);

    if (IQR_HSS_WINTERNITZ_1 == w) {
        fprintf(stdout, "    winternitz value: IQR_HSS_WINTERNITZ_1\n");
    } else if (IQR_HSS_WINTERNITZ_2 == w) {
        fprintf(stdout, "    winternitz value: IQR_HSS_WINTERNITZ_2\n");
    } else if (IQR_HSS_WINTERNITZ_4 == w) {
        fprintf(stdout, "    winternitz value: IQR_HSS_WINTERNITZ_4\n");
    } else if (IQR_HSS_WINTERNITZ_8 == w) {
        fprintf(stdout, "    winternitz value: IQR_HSS_WINTERNITZ_8\n");
    } else {
        fprintf(stdout, "    winternitz value: INVALID\n");
    }

    if (IQR_HSS_HEIGHT_5 == height) {
        fprintf(stdout, "    height: IQR_HSS_HEIGHT_5\n");
    } else if (IQR_HSS_HEIGHT_10 == height) {
        fprintf(stdout, "    height: IQR_HSS_HEIGHT_10\n");
    } else if (IQR_HSS_HEIGHT_15 == height) {
        fprintf(stdout, "    height: IQR_HSS_HEIGHT_15\n");
    } else if (IQR_HSS_HEIGHT_20 == height) {
        fprintf(stdout, "    height: IQR_HSS_HEIGHT_20\n");
    } else if (IQR_HSS_HEIGHT_25 == height) {
        fprintf(stdout, "    height: IQR_HSS_HEIGHT_25\n");
    } else {
        fprintf(stdout, "    height: INVALID\n");
    }

    if (strategy == &IQR_HSS_FULL_STRATEGY) {
        fprintf(stdout, "    strategy: Full Tree\n");
    } else if (strategy == &IQR_HSS_BDS_STRATEGY) {
        fprintf(stdout, "    strategy: BDS\n");
    } else {
        fprintf(stdout, "    strategy: INVALID\n");
    }

    fprintf(stdout, "    message data file: %s\n", message);
    fprintf(stdout, "\n");
}

/* Tests if two parameters match.
 * Returns 0 if the two parameter match.
 * Non-zero otherwise.
 *
 * Parameters are expected to be less than 32 characters in length
 */
static int paramcmp(const char *p1 , const char *p2) {
    const size_t max_param_size = 32;  // Arbitrary, but reasonable.
    if (strnlen(p1, max_param_size) != strnlen(p2, max_param_size)) {
        return 1;
    }
    return strncmp(p1, p2, max_param_size);
}

static iqr_retval parse_commandline(int argc, const char **argv, const char **sig, const char **priv, const char **state,
    iqr_HSSWinternitz *w, iqr_HSSHeight *height, const iqr_HSSTreeStrategy **strategy, const char **message)
{
    int i = 1;
    while (i != argc) {
        if (i + 2 > argc) {
            usage();
            return IQR_EBADVALUE;
        }

        if (paramcmp(argv[i], "--sig") == 0) {
            /* [--sig <filename>] */
            i++;
            *sig = argv[i];
        } else if (paramcmp(argv[i], "--priv") == 0) {
            /* [--priv <filename>] */
            i++;
            *priv = argv[i];
        } else if (paramcmp(argv[i], "--state") == 0) {
            /* [--state <filename>] */
            i++;
            *state = argv[i];
        } else if (paramcmp(argv[i], "--winternitz") == 0) {
            /* [--winternitz 1|2|4|8] */
            i++;
            if (paramcmp(argv[i], "1") == 0) {
                *w = IQR_HSS_WINTERNITZ_1;
            } else if  (paramcmp(argv[i], "2") == 0) {
                *w = IQR_HSS_WINTERNITZ_2;
            } else if  (paramcmp(argv[i], "4") == 0) {
                *w = IQR_HSS_WINTERNITZ_4;
            } else if  (paramcmp(argv[i], "8") == 0) {
                *w = IQR_HSS_WINTERNITZ_8;
            } else {
                usage();
                return IQR_EBADVALUE;
            }
        } else if (paramcmp(argv[i], "--height") == 0) {
            /* [--height 5|10|15|20|25] */
            i++;
            if (paramcmp(argv[i], "5") == 0) {
                *height = IQR_HSS_HEIGHT_5;
            } else if  (paramcmp(argv[i], "10") == 0) {
                *height = IQR_HSS_HEIGHT_10;
            } else if  (paramcmp(argv[i], "15") == 0) {
                *height = IQR_HSS_HEIGHT_15;
            } else if  (paramcmp(argv[i], "20") == 0) {
                *height = IQR_HSS_HEIGHT_20;
            } else if  (paramcmp(argv[i], "25") == 0) {
                *height = IQR_HSS_HEIGHT_25;
            } else {
                usage();
                return IQR_EBADVALUE;
            }
        } else if (paramcmp(argv[i], "--message") == 0) {
           /* [--message <filename>] */
           i++;
           *message = argv[i];
        } else if (paramcmp(argv[i], "--strategy") == 0) {
            /* [--strategy bds|full] */
            i++;
            if (paramcmp(argv[i], "bds") == 0) {
                *strategy = &IQR_HSS_BDS_STRATEGY;
            } else if (paramcmp(argv[i], "full") == 0) {
                *strategy = &IQR_HSS_FULL_STRATEGY;
            } else {
                usage();
                return IQR_EBADVALUE;
            }
        }
        i++;
    }

    return IQR_OK;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Secure memory wipe.
// ---------------------------------------------------------------------------------------------------------------------------------

static void secure_memzero(void *b, size_t len)
{
    /* You may need to substitute your platform's version of a secure memset()
     * (one that won't be optimized out by the compiler). There isn't a secure,
     * portable memset() available before C11 which provides memset_s(). Windows
     * provides SecureZeroMemory() for this purpose, and FreeBSD provides
     * explicit_bzero().
     */
#if defined(__STDC_LIB_EXT1__) || (defined(__APPLE__) && defined(__MACH__))
    memset_s(b, len, 0, len);
#elif defined(_WIN32) || defined(_WIN64)
    SecureZeroMemory(b, len);
#elif defined(__FreeBSD__)
    explicit_bzero(b, len);
#else
    /* This fallback will not be optimized out, if the compiler has a conforming
     * implementation of "volatile". It also won't take advantage of any faster
     * intrinsics, so it may end up being slow.
     *
     * Implementation courtesy of this paper:
     * http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1381.pdf
     */
    volatile unsigned char *ptr = b;
    while (len--) {
        *ptr++ = 0x00;
    }
#endif
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Executable entry point.
// ---------------------------------------------------------------------------------------------------------------------------------

int main(int argc, const char **argv)
{
    /* Default values.  Please adjust the usage() message if you make changes
     *  here.
     */
    const char *sig = "sig.dat";
    const char *priv = "priv.key";
    const char *state = "priv.state";
    const char *message = "message.dat";
    const iqr_HSSTreeStrategy *strategy = &IQR_HSS_FULL_STRATEGY;
    iqr_HSSWinternitz w = IQR_HSS_WINTERNITZ_4;
    iqr_HSSHeight height =  IQR_HSS_HEIGHT_5;

    iqr_Context *ctx = NULL;
    iqr_RNG *rng = NULL;
    uint8_t *digest = NULL;

    /* If the command line arguments were not sane, this function will return
     * an error.
     */
    iqr_retval ret = parse_commandline(argc, argv, &sig, &priv, &state, &w, &height, &strategy, &message);
    if (ret != IQR_OK) {
        return EXIT_FAILURE;
    }

    /* Make sure the user understands what we are about to do. */
    preamble(argv[0], sig, priv, state, w, height, strategy, message);

    /* IQR initialization that is not specific to HSS. */
    ret = init_toolkit(&ctx, &rng, message, &digest);
    if (ret != IQR_OK) {
        goto cleanup;
    }

    /* This function showcases the usage of HSS signing.
     */
    ret = showcase_hss_sign(ctx, rng, w, height, strategy, digest, priv, state, sig);

cleanup:
    iqr_RNGDestroy(&rng);
    iqr_DestroyContext(&ctx);
    free(digest);
    return (ret == IQR_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
