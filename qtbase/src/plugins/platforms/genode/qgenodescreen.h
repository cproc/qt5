/*
 * \brief  QGenodeScreen
 * \author Christian Prochaska
 * \date   2013-05-08
 */

/*
 * Copyright (C) 2013-2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */


#ifndef _QGENODESCREEN_H_
#define _QGENODESCREEN_H_

/* Genode includes */
#include <gui_session/connection.h>

/* Qt includes */
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <QDebug>

#include "qgenodecursor.h"
#include "qgenodesignalproxythread.h"

QT_BEGIN_NAMESPACE

class QGenodeScreen : public QObject, public QPlatformScreen
{
	Q_OBJECT

	private:

		Genode::Env              &_env;
		QRect                     _geometry;

		QGenodeSignalProxyThread &_signal_proxy;

		Gui::Connection           _gui { _env, "QGenodeScreen" };

		Genode::Io_signal_handler<QGenodeScreen>
			_info_changed_signal_handler{_env.ep(), *this,
			                             &QGenodeScreen::_handle_info_changed};

		void _handle_info_changed()
		{
			_signal_proxy.screen_info_changed();
		}

	private slots:

		void _info_changed()
		{
			Gui::Area const screen_area = _gui.panorama().convert<Gui::Area>(
				[&] (Gui::Rect rect) { return rect.area; },
				[&] (Gui::Undefined) { return Gui::Area { 1, 1 }; });

			_geometry.setRect(0, 0, screen_area.w,
			                        screen_area.h);

			QWindowSystemInterface::handleScreenGeometryChange(screen(),
			                                                   _geometry,
			                                                   _geometry);
		}

	public:

		QGenodeScreen(Genode::Env &env,
		              QGenodeSignalProxyThread &signal_proxy)
		: _env(env),
		  _signal_proxy(signal_proxy)
		{
			_gui.info_sigh(_info_changed_signal_handler);

			connect(&_signal_proxy, SIGNAL(screen_info_changed_signal()),
			        this, SLOT(_info_changed()),
			        Qt::QueuedConnection);

			Gui::Area const screen_area = _gui.panorama().convert<Gui::Area>(
				[&] (Gui::Rect rect) { return rect.area; },
				[&] (Gui::Undefined) { return Gui::Area { 1, 1 }; });

			_geometry.setRect(0, 0, screen_area.w,
			                        screen_area.h);
		}

		QRect geometry() const override { return _geometry; }
		int depth() const override { return 32; }
		QImage::Format format() const override{ return QImage::Format_RGB32; }
		QDpi logicalDpi() const override { return QDpi(80, 80); };

		QSizeF physicalSize() const override
		{
			/* 'overrideDpi()' takes 'QT_FONT_DPI' into account */
			static const int dpi = overrideDpi(logicalDpi()).first;
			return QSizeF(geometry().size()) / dpi * qreal(25.4);
		}

		QPlatformCursor *cursor() const override
		{
			static QGenodeCursor instance(_env);
			return &instance;
		}
};

QT_END_NAMESPACE

#endif /* _QGENODESCREEN_H_ */
