# mbedtls_compat is a custom shim component from the Kern project (not shipped
# with ESP-IDF) that restores the legacy mbedtls_sha256() API on mbedTLS v4
# (ESP-IDF >= 6.0). On earlier IDF releases (e.g. 5.5.x) the shim isn't needed
# or present, and naming a nonexistent component fails at configure time.
if(IDF_VERSION_MAJOR GREATER_EQUAL 6)
    set(UR_IDF_REQUIRES mbedtls mbedtls_compat)
else()
    set(UR_IDF_REQUIRES mbedtls)
endif()

idf_component_register(
    SRCS
        "src/ur.c"
        "src/ur_encoder.c"
        "src/ur_decoder.c"
        "src/fountain_encoder.c"
        "src/fountain_decoder.c"
        "src/fountain_utils.c"
        "src/bytewords.c"
        "src/crc32.c"
        "src/utils.c"
        "src/types/cbor_data.c"
        "src/types/bytes_type.c"
        "src/types/cbor_encoder.c"
        "src/types/cbor_decoder.c"
        "src/types/hd_key.c"
        "src/types/multi_key.c"
        "src/types/output.c"
        "src/types/psbt.c"
        "src/types/keypath.c"
        "src/types/registry.c"
        "src/types/byte_buffer.c"
        "src/types/bip39.c"
    INCLUDE_DIRS
        "src"
    REQUIRES
        ${UR_IDF_REQUIRES}
)

# Route sha256_compat.h to mbedtls (hardware-accelerated on ESP32P4).
# PC builds (Makefile) omit this flag and use the bundled src/sha256/sha256.c.
target_compile_definitions(${COMPONENT_LIB} PUBLIC UR_USE_MBEDTLS_SHA256)
