/*
 * \brief  QGenodeClipboard
 * \author Christian Prochaska
 * \date   2015-09-18
 */

/*
 * Copyright (C) 2015-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _QGENODECLIPBOARD_H_
#define _QGENODECLIPBOARD_H_

#include <QtCore/qglobal.h>

#ifndef QT_NO_CLIPBOARD

/* Genode includes */
#include <base/attached_rom_dataspace.h>
#include <os/reporter.h>
#include <qoost/qmember.h>

/* Qt includes */
#include <qpa/qplatformclipboard.h>

/* local includes */
#include "qgenodesignalproxythread.h"

QT_BEGIN_NAMESPACE

class QGenodeClipboard : public QObject, public QPlatformClipboard
{
	Q_OBJECT

	private:

		QGenodeSignalProxyThread &_signal_proxy;

		Genode::Attached_rom_dataspace              *_clipboard_ds = nullptr;
		Genode::Io_signal_handler<QGenodeClipboard>  _clipboard_signal_handler;

		void _handle_clipboard_changed();

		Genode::Reporter *_clipboard_reporter = nullptr;

		char *_unquoted_clipboard_content = nullptr;

		QMember<QMimeData> _mimedata;

		/*
		 * Genode signals are handled as Qt signals to avoid blocking in the
		 * Genode signal handler, which could cause nested signal handler
		 * execution.
		 */

	private Q_SLOTS:

		void _clipboard_changed();

	public:

		QGenodeClipboard(Genode::Env &env, QGenodeSignalProxyThread &signal_proxy);
		~QGenodeClipboard();
		QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
		void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;
};

QT_END_NAMESPACE

#endif /* QT_NO_CLIPBOARD */
#endif /* _QGENODECLIPBOARD_H_ */
