/*
 * \brief  QGenodeWindowSurface
 * \author Christian Prochaska
 * \date   2013-05-08
 */

/*
 * Copyright (C) 2013-2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <blit/blit.h>

/* Qt includes */
#include <QPainter>

#include <private/qguiapplication_p.h>

#include <qpa/qplatformscreen.h>

#include "qgenodeplatformwindow.h"

#include "qgenodewindowsurface.h"

#include <QDebug>

static const bool verbose = false;

QT_BEGIN_NAMESPACE

QGenodeWindowSurface::QGenodeWindowSurface(QWindow *window)
    : QPlatformBackingStore(window), _framebuffer_changed(true)
{
	//qDebug() << "QGenodeWindowSurface::QGenodeWindowSurface:" << (long)this;

	/* Calling 'QWindow::winId()' ensures that the platform window has been created */
	window->winId();

	_platform_window = static_cast<QGenodePlatformWindow*>(window->handle());
	connect(_platform_window, SIGNAL(framebuffer_changed()), this, SLOT(framebuffer_changed()));
}

QPaintDevice *QGenodeWindowSurface::paintDevice()
{
	if (verbose)
		qDebug() << "QGenodeWindowSurface::paintDevice()";

	if (_framebuffer_changed) {

		_framebuffer_changed = false;

		/*
		 * It can happen that 'resize()' was not called yet, so the size needs
		 * to be obtained from the window.
		 */
		QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
		QRect geo = _platform_window->geometry();
		_image = QImage(geo.width(), geo.height(), format);

		if (verbose)
			qDebug() << "QGenodeWindowSurface::paintDevice(): w =" << geo.width() << ", h =" << geo.height();
	}

	if (verbose)
		qDebug() << "QGenodeWindowSurface::paintDevice() finished";

	return &_image;
}

void QGenodeWindowSurface::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
	if (verbose)
		qDebug() << "QGenodeWindowSurface::flush("
		         << "window =" << window
		         << ", region =" << region
		         << ", offset =" << offset
		         << ")";

	if (offset != QPoint(0, 0)) {
		Genode::warning("QGenodeWindowSurface::flush(): offset not handled");
	}

	QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
	QRect geo = _platform_window->geometry();

	QImage framebuffer_image(_platform_window->framebuffer(),
	                         geo.width(), geo.height(),
	                         format);

	QPainter framebuffer_painter(&framebuffer_image);

	for (QRect rect : region) {

		/*
		 * It happened that after resizing a window, the given flush region was
		 * bigger than the current window size, so clipping is necessary here.
		 */

		rect &= _image.rect();

		framebuffer_painter.drawImage(rect, _image, rect);

		_platform_window->refresh(rect.x(),
		                          rect.y(),
		                          rect.width(),
		                          rect.height());
	}
}

QImage QGenodeWindowSurface::toImage() const
{
	return _image;
}

void QGenodeWindowSurface::resize(const QSize &size, const QRegion &)
{
	if (verbose)
		qDebug() << "QGenodeWindowSurface::resize:" << this << size;
}

void QGenodeWindowSurface::framebuffer_changed()
{
	_framebuffer_changed = true;
}

QT_END_NAMESPACE
