host_build {
    QT_CPU_FEATURES.x86_64 = mmx sse sse2
} else {
    QT_CPU_FEATURES.arm64 = cx16 neon
}
QT.global_private.enabled_features = alloca dlopen gui network reduce_exports sql testlib widgets xml
QT.global_private.disabled_features = sse2 alloca_h alloca_malloc_h android-style-assets avx2 private_tests dbus dbus-linked gc_binaries intelcet libudev posix_fallocate reduce_relocations release_tools relocatable stack-protector-strong system-zlib zstd
QMAKE_LIBS_LIBDL = 
QT_COORD_TYPE = double
CONFIG -= precompile_header
CONFIG += cross_compile compile_examples force_debug_info largefile neon nostrip
QT_BUILD_PARTS += libs examples
