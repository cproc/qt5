host_build {
    QT_CPU_FEATURES.x86_64 = mmx sse sse2
} else {
    QT_CPU_FEATURES.i386 = 
}
QT.global_private.enabled_features = sse2 alloca avx2 gui network reduce_exports reduce_relocations sql testlib widgets xml
QT.global_private.disabled_features = alloca_h alloca_malloc_h android-style-assets private_tests dbus dbus-linked gc_binaries libudev posix_fallocate release_tools stack-protector-strong system-zlib zstd
QT_COORD_TYPE = double
CONFIG -= precompile_header
CONFIG += cross_compile sse2 aesni sse3 ssse3 sse4_1 sse4_2 avx avx2 avx512f avx512bw avx512cd avx512dq avx512er avx512ifma avx512pf avx512vbmi avx512vl compile_examples f16c force_debug_info largefile rdrnd shani nostrip x86SimdAlways
QT_BUILD_PARTS += libs examples
QT_HOST_CFLAGS_DBUS += 
