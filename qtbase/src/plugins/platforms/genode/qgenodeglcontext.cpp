/*
 * \brief  QGenodeGLContext
 * \author Christian Prochaska
 * \date   2013-11-18
 */

/*
 * Copyright (C) 2013-2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/log.h>

/* EGL includes */
#include <EGL/egl.h>

/* Qt includes */
#include <QtEglSupport/private/qeglconvenience_p.h>
#include <QtEglSupport/private/qeglpbuffer_p.h>

/* local includes */
#include "qgenodeplatformwindow.h"
#include "qgenodeglcontext.h"

static const bool qnglc_verbose = false;

QT_BEGIN_NAMESPACE

QGenodeGLContext::QGenodeGLContext(const QSurfaceFormat &format,
                                   QPlatformOpenGLContext *share,
                                   EGLDisplay egl_display)
: QEGLPlatformContext(format, share, egl_display, 0) { }


EGLSurface QGenodeGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window) {
        return static_cast<QGenodePlatformWindow *>(surface)->eglSurface(eglConfig());
    } else {
        return static_cast<QEGLPbuffer *>(surface)->pbuffer();
    }
}


void QGenodeGLContext::swapBuffers(QPlatformSurface *surface)
{
	if (qnglc_verbose)
		Genode::log(__func__, " called");

	QEGLPlatformContext::swapBuffers(surface);

	QGenodePlatformWindow *w = static_cast<QGenodePlatformWindow*>(surface);
	w->refresh(0, 0, w->geometry().width(), w->geometry().height());
}

QT_END_NAMESPACE
