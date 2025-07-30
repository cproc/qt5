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

#ifndef QGENODEGLCONTEXT_H
#define QGENODEGLCONTEXT_H

#include <qpa/qplatformopenglcontext.h>
#include <QtEglSupport/private/qeglplatformcontext_p.h>

#include <EGL/egl.h>


QT_BEGIN_NAMESPACE


class QGenodeGLContext : public QEGLPlatformContext
{
	public:

		QGenodeGLContext(const QSurfaceFormat &format,
		                 QPlatformOpenGLContext *share,
		                 EGLDisplay egl_display);

		void swapBuffers(QPlatformSurface *surface) final;

	protected:

		EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface) final;
};

QT_END_NAMESPACE

#endif // QGENODEGLCONTEXT_H
