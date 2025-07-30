/*
 * \brief  QGenodeIntegration
 * \author Christian Prochaska
 * \date   2013-05-08
 */

/*
 * Copyright (C) 2013-2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Qt includes */
#include <QtGui/private/qguiapplication_p.h>
#include <QOffscreenSurface>
#include <qpa/qplatforminputcontextfactory_p.h>

#include "qgenodeclipboard.h"
#include "qgenodeglcontext.h"
#include "qgenodeintegration.h"
#include "qgenodeplatformwindow.h"
#include "qgenodescreen.h"
#include "qgenodewindowsurface.h"
#include "QtEglSupport/private/qeglpbuffer_p.h"
#include "QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h"
#include "QtFontDatabaseSupport/private/qfreetypefontdatabase_p.h"

QT_BEGIN_NAMESPACE

static const bool verbose = false;


QGenodeIntegration::QGenodeIntegration(Genode::Env &env)
: _env(env),
  _genode_screen(new QGenodeScreen(env, _signal_proxy))
{
	if (!eglBindAPI(EGL_OPENGL_API))
		qFatal("eglBindAPI() failed");

	m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	if (m_eglDisplay == EGL_NO_DISPLAY)
		qFatal("eglGetDisplay() failed");

	int major = -1;
	int minor = -1;
	if (!eglInitialize(m_eglDisplay, &major, &minor))
		qFatal("eglInitialize() failed");

	if (verbose)
		Genode::log("eglInitialize() returned major: ", major, ", minor: ", minor);
}


bool QGenodeIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
	switch (cap) {
		case ThreadedPixmaps: return true;
		case OpenGL:          return true;
		case ThreadedOpenGL:  return true;
		case RasterGLSurface: return true;
		default: return QPlatformIntegration::hasCapability(cap);
	}
}


QPlatformWindow *QGenodeIntegration::createPlatformWindow(QWindow *window) const
{
	if (verbose)
		qDebug() << "QGenodeIntegration::createPlatformWindow(" << window << ")";

    return new QGenodePlatformWindow(_env, _signal_proxy, window, m_eglDisplay);
}


QPlatformBackingStore *QGenodeIntegration::createPlatformBackingStore(QWindow *window) const
{
	if (verbose)
		qDebug() << "QGenodeIntegration::createPlatformBackingStore(" << window << ")";
    return new QGenodeWindowSurface(window);
}


QAbstractEventDispatcher *QGenodeIntegration::createEventDispatcher() const
{
	if (verbose)
		qDebug() << "QGenodeIntegration::createEventDispatcher()";
	return createUnixEventDispatcher();
}


void QGenodeIntegration::initialize()
{
    QWindowSystemInterface::handleScreenAdded(_genode_screen);

    QString icStr = QPlatformInputContextFactory::requested();
    if (icStr.isNull())
        icStr = QLatin1String("compose");
    m_inputContext.reset(QPlatformInputContextFactory::create(icStr));
}


QPlatformFontDatabase *QGenodeIntegration::fontDatabase() const
{
    static QFreeTypeFontDatabase db;
    return &db;
}


#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QGenodeIntegration::clipboard() const
{
	static QGenodeClipboard cb(_env, _signal_proxy);
	return &cb;
}
#endif


QPlatformOffscreenSurface *QGenodeIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
	return new QEGLPbuffer(m_eglDisplay, surface->requestedFormat(), surface);
}

QPlatformOpenGLContext *QGenodeIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QSurfaceFormat format(context->format());

    return new QGenodeGLContext(format, context->shareHandle(), m_eglDisplay);
}

QPlatformInputContext *QGenodeIntegration::inputContext() const
{
    return m_inputContext.data();
}

QPlatformNativeInterface *QGenodeIntegration::nativeInterface() const
{
	return const_cast<QGenodeIntegration*>(this);
}

void *QGenodeIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
	if (resource == "eglconfig") {
		if (context->handle())
			return static_cast<QGenodeGLContext *>(context->handle())->eglConfig();
	}

	return nullptr;
}

void *QGenodeIntegration::nativeResourceForIntegration(const QByteArray &resource)
{
	if (resource == "egldisplay")
		return m_eglDisplay;

	return nullptr;
}

QT_END_NAMESPACE
