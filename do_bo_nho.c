%%bash
# Cài đặt ARM GCC Toolchain nếu chưa có
if ! command -v arm-none-eabi-gcc &> /dev/null
then
    echo "Installing ARM GCC Toolchain..."
    sudo apt-get update
    sudo apt-get install -y gcc-arm-none-eabi
    echo "ARM GCC Toolchain installed."
else
    echo "ARM GCC Toolchain already installed."
fi

cd PQClean

# 1. Đảm bảo xóa file random lỗi của hệ điều hành
rm -f common/randombytes.c

# 2. Tạo file C mới với đúng tên hàm PQCLEAN_randombytes
cat << 'EOF' > tb_pq_ake_size.c
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"
#include "fips202.h"

// Đổi tên hàm thành PQCLEAN_randombytes theo đúng chuẩn của PQClean
int PQCLEAN_randombytes(uint8_t *buf, size_t n) {
    for (size_t i = 0; i < n; i++) {
        buf[i] = rand() & 0xFF;
    }
    return 0;
}

int main() {
    uint8_t pk[PQCLEAN_MLKEM768_CLEAN_CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[PQCLEAN_MLKEM768_CLEAN_CRYPTO_SECRETKEYBYTES];
    uint8_t ct[PQCLEAN_MLKEM768_CLEAN_CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_alice[PQCLEAN_MLKEM768_CLEAN_CRYPTO_BYTES];
    uint8_t ss_bob[PQCLEAN_MLKEM768_CLEAN_CRYPTO_BYTES];

    uint8_t S[32] = {0xAA};
    uint8_t r_A[32] = {0x11};
    uint8_t r_B[32] = {0x22};
    uint8_t T_A[32], T_B[32], K[32];

    // GỌI CÁC HÀM ĐỂ TRÌNH BIÊN DỊCH TÍNH TOÁN DUNG LƯỢNG
    PQCLEAN_MLKEM768_CLEAN_crypto_kem_keypair(pk, sk);

    sha3_256incctx ctx;
    sha3_256_inc_init(&ctx);
    sha3_256_inc_absorb(&ctx, S, 32);
    sha3_256_inc_absorb(&ctx, pk, PQCLEAN_MLKEM768_CLEAN_CRYPTO_PUBLICKEYBYTES);
    sha3_256_inc_absorb(&ctx, r_A, 32);
    sha3_256_inc_finalize(T_A, &ctx);

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

    return 0;
}
EOF

# 3. Biên dịch bằng ARM GCC (Với cờ tối ưu dung lượng -Os)
arm-none-eabi-gcc -Os -Wall \
    tb_pq_ake_size.c \
    crypto_kem/ml-kem-768/clean/*.c \
    common/*.c \
    -Icommon \
    -Icrypto_kem/ml-kem-768/clean \
    --specs=nosys.specs \
    -o benchmark_arm.elf

# 4. Đo đạc dung lượng
echo "=== KÍCH THƯỚC BỘ NHỚ TRÊN VI ĐIỀU KHIỂN ARM CORTEX-M4 ==="
arm-none-eabi-size benchmark_arm.elf