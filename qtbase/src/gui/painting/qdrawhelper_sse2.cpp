/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qconfig.h>
#include <private/qdrawhelper_x86_p.h>

#ifdef QT_COMPILER_SUPPORTS_SSE2

#include <private/qdrawingprimitive_sse2_p.h>
#include <private/qpaintengine_raster_p.h>

QT_BEGIN_NAMESPACE

#ifndef QDRAWHELPER_AVX
// in AVX mode, we'll use the SSSE3 code
void qt_blend_argb32_on_argb32_sse2(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl,
                                    int w, int h,
                                    int const_alpha)
{
    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;
    if (const_alpha == 256) {
        const __m128i alphaMask = _mm_set1_epi32(0xff000000);
        const __m128i nullVector = _mm_set1_epi32(0);
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i one = _mm_set1_epi16(0xff);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        for (int y = 0; y < h; ++y) {
            BLEND_SOURCE_OVER_ARGB32_SSE2(dst, src, w, nullVector, half, one, colorMask, alphaMask);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    } else if (const_alpha != 0) {
        // dest = (s + d * sia) * ca + d * cia
        //      = s * ca + d * (sia * ca + cia)
        //      = s * ca + d * (1 - sa*ca)
        const_alpha = (const_alpha * 255) >> 8;
        const __m128i nullVector = _mm_set1_epi32(0);
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i one = _mm_set1_epi16(0xff);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
        for (int y = 0; y < h; ++y) {
            BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_SSE2(dst, src, w, nullVector, half, one, colorMask, constAlphaVector)
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    }
}
#endif

// qblendfunctions.cpp
void qt_blend_rgb32_on_rgb32(uchar *destPixels, int dbpl,
                             const uchar *srcPixels, int sbpl,
                             int w, int h,
                             int const_alpha);

void qt_blend_rgb32_on_rgb32_sse2(uchar *destPixels, int dbpl,
                                 const uchar *srcPixels, int sbpl,
                                 int w, int h,
                                 int const_alpha)
{
    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;
    if (const_alpha != 256) {
        if (const_alpha != 0) {
            const __m128i half = _mm_set1_epi16(0x80);
            const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);

            const_alpha = (const_alpha * 255) >> 8;
            int one_minus_const_alpha = 255 - const_alpha;
            const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
            const __m128i oneMinusConstAlpha =  _mm_set1_epi16(one_minus_const_alpha);
            for (int y = 0; y < h; ++y) {
                int x = 0;

                // First, align dest to 16 bytes:
                ALIGNMENT_PROLOGUE_16BYTES(dst, x, w) {
                    dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], one_minus_const_alpha);
                }

                for (; x < w-3; x += 4) {
                    __m128i srcVector = _mm_loadu_si128((const __m128i *)&src[x]);
                    const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);
                    __m128i result;
                    INTERPOLATE_PIXEL_255_SSE2(result, srcVector, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half);
                    _mm_store_si128((__m128i *)&dst[x], result);
                }
                SIMD_EPILOGUE(x, w, 3)
                    dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], one_minus_const_alpha);
                dst = (quint32 *)(((uchar *) dst) + dbpl);
                src = (const quint32 *)(((const uchar *) src) + sbpl);
            }
        }
    } else {
        qt_blend_rgb32_on_rgb32(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
    }
}

void QT_FASTCALL comp_func_SourceOver_sse2(uint *destPixels, const uint *srcPixels, int length, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256);

    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;

    const __m128i nullVector = _mm_set1_epi32(0);
    const __m128i half = _mm_set1_epi16(0x80);
    const __m128i one = _mm_set1_epi16(0xff);
    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
    if (const_alpha == 255) {
        const __m128i alphaMask = _mm_set1_epi32(0xff000000);
        BLEND_SOURCE_OVER_ARGB32_SSE2(dst, src, length, nullVector, half, one, colorMask, alphaMask);
    } else {
        const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
        BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_SSE2(dst, src, length, nullVector, half, one, colorMask, constAlphaVector);
    }
}

void QT_FASTCALL comp_func_Plus_sse2(uint *dst, const uint *src, int length, uint const_alpha)
{
    int x = 0;

    if (const_alpha == 255) {
        // 1) Prologue: align destination on 16 bytes
        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            dst[x] = comp_func_Plus_one_pixel(dst[x], src[x]);

        // 2) composition with SSE2
        for (; x < length - 3; x += 4) {
            const __m128i srcVector = _mm_loadu_si128((const __m128i *)&src[x]);
            const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);

            const __m128i result = _mm_adds_epu8(srcVector, dstVector);
            _mm_store_si128((__m128i *)&dst[x], result);
        }

        // 3) Epilogue:
        SIMD_EPILOGUE(x, length, 3)
            dst[x] = comp_func_Plus_one_pixel(dst[x], src[x]);
    } else {
        const int one_minus_const_alpha = 255 - const_alpha;
        const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
        const __m128i oneMinusConstAlpha =  _mm_set1_epi16(one_minus_const_alpha);

        // 1) Prologue: align destination on 16 bytes
        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            dst[x] = comp_func_Plus_one_pixel_const_alpha(dst[x], src[x], const_alpha, one_minus_const_alpha);

        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        // 2) composition with SSE2
        for (; x < length - 3; x += 4) {
            const __m128i srcVector = _mm_loadu_si128((const __m128i *)&src[x]);
            const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);

            __m128i result = _mm_adds_epu8(srcVector, dstVector);
            INTERPOLATE_PIXEL_255_SSE2(result, result, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half)
            _mm_store_si128((__m128i *)&dst[x], result);
        }

        // 3) Epilogue:
        SIMD_EPILOGUE(x, length, 3)
            dst[x] = comp_func_Plus_one_pixel_const_alpha(dst[x], src[x], const_alpha, one_minus_const_alpha);
    }
}

void QT_FASTCALL comp_func_Source_sse2(uint *dst, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dst, src, length * sizeof(uint));
    } else {
        const int ialpha = 255 - const_alpha;

        int x = 0;

        // 1) prologue, align on 16 bytes
        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], ialpha);

        // 2) interpolate pixels with SSE2
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
        const __m128i oneMinusConstAlpha =  _mm_set1_epi16(ialpha);
        for (; x < length - 3; x += 4) {
            const __m128i srcVector = _mm_loadu_si128((const __m128i *)&src[x]);
            __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);
            INTERPOLATE_PIXEL_255_SSE2(dstVector, srcVector, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half)
            _mm_store_si128((__m128i *)&dst[x], dstVector);
        }

        // 3) Epilogue
        SIMD_EPILOGUE(x, length, 3)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], ialpha);
    }
}

#ifndef __AVX2__
static Q_NEVER_INLINE
void Q_DECL_VECTORCALL qt_memfillXX_aligned(void *dest, __m128i value128, quintptr bytecount)
{
    __m128i *dst128 = reinterpret_cast<__m128i *>(dest);
    __m128i *end128 = reinterpret_cast<__m128i *>(static_cast<uchar *>(dest) + bytecount);

    while (dst128 + 4 <= end128) {
        _mm_store_si128(dst128 + 0, value128);
        _mm_store_si128(dst128 + 1, value128);
        _mm_store_si128(dst128 + 2, value128);
        _mm_store_si128(dst128 + 3, value128);
        dst128 += 4;
    }

    bytecount %= 4 * sizeof(__m128i);
    switch (bytecount / sizeof(__m128i)) {
    case 3: _mm_store_si128(dst128++, value128); Q_FALLTHROUGH();
    case 2: _mm_store_si128(dst128++, value128); Q_FALLTHROUGH();
    case 1: _mm_store_si128(dst128++, value128);
    }
}

void qt_memfill64_sse2(quint64 *dest, quint64 value, qsizetype count)
{
    quintptr misaligned = quintptr(dest) % sizeof(__m128i);
    if (misaligned && count) {
#if defined(Q_PROCESSOR_X86_32)
        // Before SSE came out, the alignment of the stack used to be only 4
        // bytes and some OS/ABIs (notably, code generated by MSVC) still only
        // align to that. In any case, we cannot count on the alignment of
        // quint64 to be 8 -- see QtPrivate::AlignOf_WorkaroundForI386Abi in
        // qglobal.h.
        //
        // If the pointer is not aligned to at least 8 bytes, then we'll never
        // in turn hit a multiple of 16 for the qt_memfillXX_aligned call
        // below.
        if (Q_UNLIKELY(misaligned % sizeof(quint64)))
            return qt_memfill_template(dest, value, count);
#endif

        *dest++ = value;
        --count;
    }

    if (count % 2) {
        dest[count - 1] = value;
        --count;
    }

    qt_memfillXX_aligned(dest, _mm_set1_epi64x(value), count * sizeof(quint64));
}

void qt_memfill32_sse2(quint32 *dest, quint32 value, qsizetype count)
{
    if (count < 4) {
        // this simplifies the code below: the first switch can fall through
        // without checking the value of count
        switch (count) {
        case 3: *dest++ = value; Q_FALLTHROUGH();
        case 2: *dest++ = value; Q_FALLTHROUGH();
        case 1: *dest   = value;
        }
        return;
    }

    const int align = (quintptr)(dest) & 0xf;
    switch (align) {
    case 4:  *dest++ = value; --count; Q_FALLTHROUGH();
    case 8:  *dest++ = value; --count; Q_FALLTHROUGH();
    case 12: *dest++ = value; --count;
    }

    const int rest = count & 0x3;
    if (rest) {
        switch (rest) {
        case 3: dest[count - 3] = value; Q_FALLTHROUGH();
        case 2: dest[count - 2] = value; Q_FALLTHROUGH();
        case 1: dest[count - 1] = value;
        }
    }

    qt_memfillXX_aligned(dest, _mm_set1_epi32(value), count * sizeof(quint32));
}
#endif // !__AVX2__

void QT_FASTCALL comp_func_solid_Source_sse2(uint *destPixels, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255) {
        qt_memfill32(destPixels, color, length);
    } else {
        const quint32 ialpha = 255 - const_alpha;
        color = BYTE_MUL(color, const_alpha);
        int x = 0;

        quint32 *dst = (quint32 *) destPixels;
        const __m128i colorVector = _mm_set1_epi32(color);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i iAlphaVector = _mm_set1_epi16(ialpha);

        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            destPixels[x] = color + BYTE_MUL(destPixels[x], ialpha);

        for (; x < length-3; x += 4) {
            __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);
            BYTE_MUL_SSE2(dstVector, dstVector, iAlphaVector, colorMask, half);
            dstVector = _mm_add_epi8(colorVector, dstVector);
            _mm_store_si128((__m128i *)&dst[x], dstVector);
        }
        SIMD_EPILOGUE(x, length, 3)
            destPixels[x] = color + BYTE_MUL(destPixels[x], ialpha);
    }
}

void QT_FASTCALL comp_func_solid_SourceOver_sse2(uint *destPixels, int length, uint color, uint const_alpha)
{
    if ((const_alpha & qAlpha(color)) == 255) {
        qt_memfill32(destPixels, color, length);
    } else {
        if (const_alpha != 255)
            color = BYTE_MUL(color, const_alpha);

        const quint32 minusAlphaOfColor = qAlpha(~color);
        int x = 0;

        quint32 *dst = (quint32 *) destPixels;
        const __m128i colorVector = _mm_set1_epi32(color);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i minusAlphaOfColorVector = _mm_set1_epi16(minusAlphaOfColor);

        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);

        for (; x < length-3; x += 4) {
            __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);
            BYTE_MUL_SSE2(dstVector, dstVector, minusAlphaOfColorVector, colorMask, half);
            dstVector = _mm_add_epi8(colorVector, dstVector);
            _mm_store_si128((__m128i *)&dst[x], dstVector);
        }
        SIMD_EPILOGUE(x, length, 3)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);
    }
}

void qt_bitmapblit32_sse2_base(QRasterBuffer *rasterBuffer, int x, int y,
                          quint32 color,
                          const uchar *src, int width, int height, int stride)
{
    quint32 *dest = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->stride<quint32>();

    const __m128i c128 = _mm_set1_epi32(color);
    const __m128i maskmask1 = _mm_set_epi32(0x10101010, 0x20202020,
                                            0x40404040, 0x80808080);
    const __m128i maskadd1 = _mm_set_epi32(0x70707070, 0x60606060,
                                           0x40404040, 0x00000000);

    if (width > 4) {
        const __m128i maskmask2 = _mm_set_epi32(0x01010101, 0x02020202,
                                                0x04040404, 0x08080808);
        const __m128i maskadd2 = _mm_set_epi32(0x7f7f7f7f, 0x7e7e7e7e,
                                               0x7c7c7c7c, 0x78787878);
        while (height--) {
            for (int x = 0; x < width; x += 8) {
                const quint8 s = src[x >> 3];
                if (!s)
                    continue;
                __m128i mask1 = _mm_set1_epi8(s);
                __m128i mask2 = mask1;

                mask1 = _mm_and_si128(mask1, maskmask1);
                mask1 = _mm_add_epi8(mask1, maskadd1);
                _mm_maskmoveu_si128(c128, mask1, (char*)(dest + x));
                mask2 = _mm_and_si128(mask2, maskmask2);
                mask2 = _mm_add_epi8(mask2, maskadd2);
                _mm_maskmoveu_si128(c128, mask2, (char*)(dest + x + 4));
            }
            dest += destStride;
            src += stride;
        }
    } else {
        while (height--) {
            const quint8 s = *src;
            if (s) {
                __m128i mask1 = _mm_set1_epi8(s);
                mask1 = _mm_and_si128(mask1, maskmask1);
                mask1 = _mm_add_epi8(mask1, maskadd1);
                _mm_maskmoveu_si128(c128, mask1, (char*)(dest));
            }
            dest += destStride;
            src += stride;
        }
    }
}

void qt_bitmapblit32_sse2(QRasterBuffer *rasterBuffer, int x, int y,
                          const QRgba64 &color,
                          const uchar *src, int width, int height, int stride)
{
    qt_bitmapblit32_sse2_base(rasterBuffer, x, y, color.toArgb32(), src, width, height, stride);
}

void qt_bitmapblit8888_sse2(QRasterBuffer *rasterBuffer, int x, int y,
                            const QRgba64 &color,
                            const uchar *src, int width, int height, int stride)
{
    qt_bitmapblit32_sse2_base(rasterBuffer, x, y, ARGB2RGBA(color.toArgb32()), src, width, height, stride);
}

void qt_bitmapblit16_sse2(QRasterBuffer *rasterBuffer, int x, int y,
                          const QRgba64 &color,
                          const uchar *src, int width, int height, int stride)
{
    const quint16 c = qConvertRgb32To16(color.toArgb32());
    quint16 *dest = reinterpret_cast<quint16*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->stride<quint32>();

    const __m128i c128 = _mm_set1_epi16(c);
QT_WARNING_DISABLE_MSVC(4309) // truncation of constant value
    const __m128i maskmask = _mm_set_epi16(0x0101, 0x0202, 0x0404, 0x0808,
                                           0x1010, 0x2020, 0x4040, 0x8080);
    const __m128i maskadd = _mm_set_epi16(0x7f7f, 0x7e7e, 0x7c7c, 0x7878,
                                          0x7070, 0x6060, 0x4040, 0x0000);

    while (height--) {
        for (int x = 0; x < width; x += 8) {
            const quint8 s = src[x >> 3];
            if (!s)
                continue;
            __m128i mask = _mm_set1_epi8(s);
            mask = _mm_and_si128(mask, maskmask);
            mask = _mm_add_epi8(mask, maskadd);
            _mm_maskmoveu_si128(c128, mask, (char*)(dest + x));
        }
        dest += destStride;
        src += stride;
    }
}

class QSimdSse2
{
public:
    typedef __m128i Int32x4;
    typedef __m128 Float32x4;

    union Vect_buffer_i { Int32x4 v; int i[4]; };
    union Vect_buffer_f { Float32x4 v; float f[4]; };

    static inline Float32x4 Q_DECL_VECTORCALL v_dup(float x) { return _mm_set1_ps(x); }
    static inline Float32x4 Q_DECL_VECTORCALL v_dup(double x) { return _mm_set1_ps(x); }
    static inline Int32x4 Q_DECL_VECTORCALL v_dup(int x) { return _mm_set1_epi32(x); }
    static inline Int32x4 Q_DECL_VECTORCALL v_dup(uint x) { return _mm_set1_epi32(x); }

    static inline Float32x4 Q_DECL_VECTORCALL v_add(Float32x4 a, Float32x4 b) { return _mm_add_ps(a, b); }
    static inline Int32x4 Q_DECL_VECTORCALL v_add(Int32x4 a, Int32x4 b) { return _mm_add_epi32(a, b); }

    static inline Float32x4 Q_DECL_VECTORCALL v_max(Float32x4 a, Float32x4 b) { return _mm_max_ps(a, b); }
    static inline Float32x4 Q_DECL_VECTORCALL v_min(Float32x4 a, Float32x4 b) { return _mm_min_ps(a, b); }
    static inline Int32x4 Q_DECL_VECTORCALL v_min_16(Int32x4 a, Int32x4 b) { return _mm_min_epi16(a, b); }

    static inline Int32x4 Q_DECL_VECTORCALL v_and(Int32x4 a, Int32x4 b) { return _mm_and_si128(a, b); }

    static inline Float32x4 Q_DECL_VECTORCALL v_sub(Float32x4 a, Float32x4 b) { return _mm_sub_ps(a, b); }
    static inline Int32x4 Q_DECL_VECTORCALL v_sub(Int32x4 a, Int32x4 b) { return _mm_sub_epi32(a, b); }

    static inline Float32x4 Q_DECL_VECTORCALL v_mul(Float32x4 a, Float32x4 b) { return _mm_mul_ps(a, b); }

    static inline Float32x4 Q_DECL_VECTORCALL v_sqrt(Float32x4 x) { return _mm_sqrt_ps(x); }

    static inline Int32x4 Q_DECL_VECTORCALL v_toInt(Float32x4 x) { return _mm_cvttps_epi32(x); }

    static inline Int32x4 Q_DECL_VECTORCALL v_greaterOrEqual(Float32x4 a, Float32x4 b) { return _mm_castps_si128(_mm_cmpgt_ps(a, b)); }
};

const uint * QT_FASTCALL qt_fetch_radial_gradient_sse2(uint *buffer, const Operator *op, const QSpanData *data,
                                                       int y, int x, int length)
{
    return qt_fetch_radial_gradient_template<QRadialFetchSimd<QSimdSse2>,uint>(buffer, op, data, y, x, length);
}

void qt_scale_image_argb32_on_argb32_sse2(uchar *destPixels, int dbpl,
                                          const uchar *srcPixels, int sbpl, int srch,
                                          const QRectF &targetRect,
                                          const QRectF &sourceRect,
                                          const QRect &clip,
                                          int const_alpha)
{
    if (const_alpha != 256) {
        // from qblendfunctions.cpp
        extern void qt_scale_image_argb32_on_argb32(uchar *destPixels, int dbpl,
                                               const uchar *srcPixels, int sbpl, int srch,
                                               const QRectF &targetRect,
                                               const QRectF &sourceRect,
                                               const QRect &clip,
                                               int const_alpha);
        return qt_scale_image_argb32_on_argb32(destPixels, dbpl, srcPixels, sbpl, srch, targetRect, sourceRect, clip, const_alpha);
    }

    qreal sx = targetRect.width() / (qreal) sourceRect.width();
    qreal sy = targetRect.height() / (qreal) sourceRect.height();

    int ix = 0x00010000 / sx;
    int iy = 0x00010000 / sy;

    int cx1 = clip.x();
    int cx2 = clip.x() + clip.width();
    int cy1 = clip.top();
    int cy2 = clip.y() + clip.height();

    int tx1 = qRound(targetRect.left());
    int tx2 = qRound(targetRect.right());
    int ty1 = qRound(targetRect.top());
    int ty2 = qRound(targetRect.bottom());

    if (tx2 < tx1)
        qSwap(tx2, tx1);
    if (ty2 < ty1)
        qSwap(ty2, ty1);

    if (tx1 < cx1)
        tx1 = cx1;
    if (tx2 >= cx2)
        tx2 = cx2;

    if (tx1 >= tx2)
        return;

    if (ty1 < cy1)
        ty1 = cy1;
    if (ty2 >= cy2)
       ty2 = cy2;
    if (ty1 >= ty2)
        return;

    int h = ty2 - ty1;
    int w = tx2 - tx1;

    quint32 basex;
    quint32 srcy;

    if (sx < 0) {
        int dstx = qFloor((tx1 + qreal(0.5) - targetRect.right()) * ix) + 1;
        basex = quint32(sourceRect.right() * 65536) + dstx;
    } else {
        int dstx = qCeil((tx1 + qreal(0.5) - targetRect.left()) * ix) - 1;
        basex = quint32(sourceRect.left() * 65536) + dstx;
    }
    if (sy < 0) {
        int dsty = qFloor((ty1 + qreal(0.5) - targetRect.bottom()) * iy) + 1;
        srcy = quint32(sourceRect.bottom() * 65536) + dsty;
    } else {
        int dsty = qCeil((ty1 + qreal(0.5) - targetRect.top()) * iy) - 1;
        srcy = quint32(sourceRect.top() * 65536) + dsty;
    }

    quint32 *dst = ((quint32 *) (destPixels + ty1 * dbpl)) + tx1;

    const __m128i nullVector = _mm_set1_epi32(0);
    const __m128i half = _mm_set1_epi16(0x80);
    const __m128i one = _mm_set1_epi16(0xff);
    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
    const __m128i ixVector = _mm_set1_epi32(4*ix);

    // this bounds check here is required as floating point rounding above might in some cases lead to
    // w/h values that are one pixel too large, falling outside of the valid image area.
    const int ystart = srcy >> 16;
    if (ystart >= srch && iy < 0) {
        srcy += iy;
        --h;
    }
    const int xstart = basex >> 16;
    if (xstart >=  (int)(sbpl/sizeof(quint32)) && ix < 0) {
        basex += ix;
        --w;
    }
    int yend = (srcy + iy * (h - 1)) >> 16;
    if (yend < 0 || yend >= srch)
        --h;
    int xend = (basex + ix * (w - 1)) >> 16;
    if (xend < 0 || xend >= (int)(sbpl/sizeof(quint32)))
        --w;

    while (h--) {
        const uint *src = (const quint32 *) (srcPixels + (srcy >> 16) * sbpl);
        int srcx = basex;
        int x = 0;

        ALIGNMENT_PROLOGUE_16BYTES(dst, x, w) {
            uint s = src[srcx >> 16];
            dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s));
            srcx += ix;
        }

        __m128i srcxVector = _mm_set_epi32(srcx, srcx + ix, srcx + ix + ix, srcx + ix + ix + ix);

        for (; x<w - 3; x += 4) {
            union Vect_buffer { __m128i vect; quint32 i[4]; };
            Vect_buffer addr;
            addr.vect = _mm_srli_epi32(srcxVector, 16);
            srcxVector = _mm_add_epi32(srcxVector, ixVector);

            const __m128i srcVector = _mm_set_epi32(src[addr.i[0]], src[addr.i[1]], src[addr.i[2]], src[addr.i[3]]);
            BLEND_SOURCE_OVER_ARGB32_SSE2_helper(dst, srcVector, nullVector, half, one, colorMask, alphaMask);
        }

        SIMD_EPILOGUE(x, w, 3) {
            uint s = src[(basex + x*ix) >> 16];
            dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s));
        }
        dst = (quint32 *)(((uchar *) dst) + dbpl);
        srcy += iy;
    }
}


QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_SSE2
