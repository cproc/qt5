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

#include "qgenodeclipboard.h"

#ifndef QT_NO_CLIPBOARD

/* Genode includes */
#include <base/node.h>

/* Qt includes */
#include <QMimeData>

QT_BEGIN_NAMESPACE


static constexpr bool verbose = false;


QGenodeClipboard::QGenodeClipboard(Genode::Env &env, QGenodeSignalProxyThread &signal_proxy)
: _signal_proxy(signal_proxy),
  _clipboard_signal_handler(env.ep(), *this, &QGenodeClipboard::_handle_clipboard_changed)
{
	try {

		Genode::Attached_rom_dataspace config(env, "config");

		if (config.node().attribute_value("clipboard", false)) {

			try {

				_clipboard_ds = new Genode::Attached_rom_dataspace(env, "clipboard");

				_clipboard_ds->sigh(_clipboard_signal_handler);
				_clipboard_ds->update();

			} catch (...) { }

			try {
				_clipboard_reporter = new Genode::Reporter(env, "clipboard");
				_clipboard_reporter->enabled(true);
			} catch (...) {	}

		}
	} catch (...) { }

	connect(&_signal_proxy, SIGNAL(clipboard_changed_signal()),
	        this, SLOT(_clipboard_changed()),
	        Qt::QueuedConnection);
}


QGenodeClipboard::~QGenodeClipboard()
{
	free(_unquoted_clipboard_content);
	delete _clipboard_ds;
	delete _clipboard_reporter;
}


void QGenodeClipboard::_handle_clipboard_changed()
{
	_signal_proxy.clipboard_changed();
}


void QGenodeClipboard::_clipboard_changed()
{
	emitChanged(QClipboard::Clipboard);
}


QMimeData *QGenodeClipboard::mimeData(QClipboard::Mode mode)
{
	if (!supportsMode(mode) || !_clipboard_ds)
		return nullptr;

	_clipboard_ds->update();

	if (!_clipboard_ds->valid()) {
		if (verbose)
			Genode::error("invalid clipboard dataspace");
		return nullptr;
	}

	Genode::Const_byte_range_ptr clipboard_ds_content {
		_clipboard_ds->local_addr<char>(), _clipboard_ds->size() };

	Genode::Node node(clipboard_ds_content);

	if (!node.has_type("clipboard")) {
		Genode::error("invalid clipboard node syntax");
		return nullptr;
	}

	free(_unquoted_clipboard_content);

	_unquoted_clipboard_content = (char*)malloc(node.num_bytes());

	if (!_unquoted_clipboard_content) {
		Genode::error("could not allocate buffer for unquoted clipboard content");
		return nullptr;
	}

	struct Output : Genode::Output, Genode::Noncopyable
	{
		struct {
			char  *dst;
			size_t capacity; /* remaining capacity in bytes */
		};

		void out_char(char c) override
		{
			if (capacity) { *dst++ = c; capacity--; }
		}

		void out_string(char const *s, size_t n) override
		{
			while (n-- && capacity) out_char(*s++);
		}

		Output(char *dst, size_t n) : dst(dst), capacity(n) { }

	} output { _unquoted_clipboard_content, node.num_bytes() };

	size_t unquoted_clipboard_content_size = 0;

	bool quoted = false;
	node.for_each_quoted_line([&] (auto const &line) {
		quoted = true;
		line.print(output);
		if (!line.last) output.out_char('\n');
	});
	if (quoted) {
		if (!output.capacity)
			Genode::warning("unquoted clipboard content exceeded buffer: ", node);
		else
			unquoted_clipboard_content_size = node.num_bytes() - output.capacity;
	}

	_mimedata->setText(QString::fromUtf8(_unquoted_clipboard_content,
	                                     unquoted_clipboard_content_size));

	return _mimedata;
}


void QGenodeClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
	if (!data || !data->hasText() || !supportsMode(mode))
		return;

	QString text = data->text();
	QByteArray utf8text = text.toUtf8();

	if (!_clipboard_reporter)
		return;

	_clipboard_reporter->generate([&] (Genode::Generator &g) {
		g.append_quoted(utf8text.constData(), utf8text.size());
	}).with_error([] (Genode::Buffer_error) {
		Genode::error("could not write clipboard data");
	});
}

QT_END_NAMESPACE

#endif /* QT_NO_CLIPBOARD */
