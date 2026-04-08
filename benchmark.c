%%bash
# 1. Tải thư viện chuẩn (Clean)
rm -rf PQClean
git clone https://github.com/PQClean/PQClean.git

# 2. Tạo file đo lường C (Đã cấu hình lại đường dẫn Header trực tiếp)
cat << 'EOF' > PQClean/tb_pq_ake_bench.c
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <x86intrin.h>

// Include trực tiếp thư viện
#include "api.h"
#include "fips202.h"

// Macro đếm chu kỳ CPU
#define MEASURE_CYCLES(cycles, code) \
    do { \
        uint64_t start = __rdtsc(); \
        code; \
        uint64_t end = __rdtsc(); \
        cycles = end - start; \
    } while (0)

int main() {
    uint8_t pk[PQCLEAN_MLKEM768_CLEAN_CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[PQCLEAN_MLKEM768_CLEAN_CRYPTO_SECRETKEYBYTES];
    uint8_t ct[PQCLEAN_MLKEM768_CLEAN_CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_alice[PQCLEAN_MLKEM768_CLEAN_CRYPTO_BYTES];
    uint8_t ss_bob[PQCLEAN_MLKEM768_CLEAN_CRYPTO_BYTES];

    uint8_t S[32] = {0xAA}; // Khóa PSK
    uint8_t r_A[32] = {0x11}; // Nonce A
    uint8_t r_B[32] = {0x22}; // Nonce B
    uint8_t T_A[32], T_B[32], K[32];

    uint64_t cycles_alice_step1, cycles_bob_step2, cycles_alice_step3;

    printf("\n=== KET QUA BENCHMARK: TB-PQ-AKE PROTOCOL (REAL CYCLES) ===\n\n");

    MEASURE_CYCLES(cycles_alice_step1, {
        PQCLEAN_MLKEM768_CLEAN_crypto_kem_keypair(pk, sk);

        sha3_256incctx ctx;
        sha3_256_inc_init(&ctx);
        sha3_256_inc_absorb(&ctx, S, 32);
        sha3_256_inc_absorb(&ctx, pk, PQCLEAN_MLKEM768_CLEAN_CRYPTO_PUBLICKEYBYTES);
        sha3_256_inc_absorb(&ctx, r_A, 32);
        sha3_256_inc_finalize(T_A, &ctx);
    });

    MEASURE_CYCLES(cycles_bob_step2, {
        PQCLEAN_MLKEM768_CLEAN_crypto_kem_enc(ct, ss_bob, pk);

        sha3_256incctx kdf_ctx;
        sha3_256_inc_init(&kdf_ctx);
        sha3_256_inc_absorb(&kdf_ctx, ss_bob, 32);
        sha3_256_inc_absorb(&kdf_ctx, S, 32);
        sha3_256_inc_finalize(K, &kdf_ctx);

        sha3_256incctx mac_ctx;
        sha3_256_inc_init(&mac_ctx);
        sha3_256_inc_absorb(&mac_ctx, K, 32);
        sha3_256_inc_absorb(&mac_ctx, ct, PQCLEAN_MLKEM768_CLEAN_CRYPTO_CIPHERTEXTBYTES);
        sha3_256_inc_absorb(&mac_ctx, r_B, 32);
        sha3_256_inc_absorb(&mac_ctx, T_A, 32);
        sha3_256_inc_finalize(T_B, &mac_ctx);
    });

    MEASURE_CYCLES(cycles_alice_step3, {
        PQCLEAN_MLKEM768_CLEAN_crypto_kem_dec(ss_alice, ct, sk);

        sha3_256incctx kdf_ctx_a;
        sha3_256_inc_init(&kdf_ctx_a);
        sha3_256_inc_absorb(&kdf_ctx_a, ss_alice, 32);
        sha3_256_inc_absorb(&kdf_ctx_a, S, 32);
        sha3_256_inc_finalize(K, &kdf_ctx_a);

        uint8_t check_T_B[32];
        sha3_256incctx mac_ctx_a;
        sha3_256_inc_init(&mac_ctx_a);
        sha3_256_inc_absorb(&mac_ctx_a, K, 32);
        sha3_256_inc_absorb(&mac_ctx_a, ct, PQCLEAN_MLKEM768_CLEAN_CRYPTO_CIPHERTEXTBYTES);
        sha3_256_inc_absorb(&mac_ctx_a, r_B, 32);
        sha3_256_inc_absorb(&mac_ctx_a, T_A, 32);
        sha3_256_inc_finalize(check_T_B, &mac_ctx_a);
    });

    uint64_t total_cycles = cycles_alice_step1 + cycles_bob_step2 + cycles_alice_step3;

    printf("- Step 1 (Alice Init)   : %lu CPU Cycles\n", cycles_alice_step1);
    printf("- Step 2 (Bob Reply)    : %lu CPU Cycles\n", cycles_bob_step2);
    printf("- Step 3 (Alice Verify) : %lu CPU Cycles\n", cycles_alice_step3);
    printf("--------------------------------------------------\n");
    printf("=> TOTAL CPU COST       : %lu CPU Cycles\n", total_cycles);

    return 0;
}
EOF

# 3. Biên dịch trực tiếp (Không dùng Makefile)
cd PQClean
gcc -O3 -Wall \
    tb_pq_ake_bench.c \
    crypto_kem/ml-kem-768/clean/*.c \
    common/*.c \
    -Icommon \
    -Icrypto_kem/ml-kem-768/clean \
    -o benchmark_tb_pq_ake

# 4. Xuất kết quả
./benchmark_tb_pq_ake