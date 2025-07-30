/*
 * \brief  QGenodePlatformWindow
 * \author Christian Prochaska
 * \author Christian Helmuth
 * \date   2013-05-08
 */

/*
 * Copyright (C) 2013-2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */


/* Genode includes */
#include <base/log.h>
#include <gui_session/client.h>

/* Qt includes */
#include <qpa/qplatformscreen.h>
#include <QGuiApplication>
#include <QDebug>

#include "qgenodeplatformwindow.h"
#include "qgenodesignalproxythread.h"

QT_BEGIN_NAMESPACE

static const bool qnpw_verbose = false/*true*/;

QStringList QGenodePlatformWindow::_gui_session_label_list;

QTouchDevice * QGenodePlatformWindow::_init_touch_device()
{
	QVector<QWindowSystemInterface::TouchPoint>::iterator i = _touch_points.begin();
	for (unsigned n = 0; i != _touch_points.end(); ++i, ++n) {
		i->id    = n;
		i->state = Qt::TouchPointReleased;
	}

	QTouchDevice *dev = new QTouchDevice;

	dev->setName("Genode multi-touch device");
	dev->setType(QTouchDevice::TouchScreen);
	dev->setCapabilities(QTouchDevice::Position);

	QWindowSystemInterface::registerTouchDevice(dev);

	return dev;
}

void QGenodePlatformWindow::_process_touch_events(QList<Input::Event> const &events)
{
	if (events.empty()) return;

	for (QList<Input::Event>::const_iterator i = events.begin(); i != events.end(); ++i) {

		i->handle_touch([&] (Input::Touch_id id, int x, int y) {
			QList<QWindowSystemInterface::TouchPoint> touch_points;

			if (id.value >= _touch_points.size()) {
				Genode::warning("drop touch input, out of bounds");
				return;
			}

			QWindowSystemInterface::TouchPoint &otp = _touch_points[id.value];
			QWindowSystemInterface::TouchPoint tp;

			tp.id   = id.value;
			tp.area = QRectF(QPointF(0, 0), QSize(1, 1));

			/* report 1x1 rectangular area centered at screen coordinates */
			tp.area.moveCenter(QPointF(x, y));

			tp.state    = otp.state == Qt::TouchPointReleased
			            ? Qt::TouchPointPressed : Qt::TouchPointMoved;
			tp.pressure = 1;

			otp = tp;
			touch_points.push_back(tp);
			QWindowSystemInterface::handleTouchEvent(0, _touch_device, touch_points);
		});

		i->handle_touch_release([&] (Input::Touch_id id) {
			QList<QWindowSystemInterface::TouchPoint> touch_points;

			if (id.value >= _touch_points.size()) {
				Genode::warning("drop touch input, out of bounds");
				return;
			}

			QWindowSystemInterface::TouchPoint &otp = _touch_points[id.value];
			QWindowSystemInterface::TouchPoint tp;

			tp.id       = id.value;
			tp.area     = otp.area;
			tp.state    = Qt::TouchPointReleased;
			tp.pressure = 0;

			otp = tp;
			touch_points.push_back(tp);
			QWindowSystemInterface::handleTouchEvent(0, _touch_device, touch_points);
		});
	}
}


QGenodePlatformWindow::Mapped_key
QGenodePlatformWindow::_mapped_key_from_codepoint(Codepoint codepoint)
{
	/* special keys: function-key unicodes */
	switch (codepoint.value) {
	case 0x0008: return Mapped_key { Qt::Key_Backspace };
	case 0x0009: return Mapped_key { Qt::Key_Tab };
	case 0x000a: return Mapped_key { Qt::Key_Return };
	case 0x001b: return Mapped_key { Qt::Key_Escape };
	case 0xf700: return Mapped_key { Qt::Key_Up };
	case 0xf701: return Mapped_key { Qt::Key_Down };
	case 0xf702: return Mapped_key { Qt::Key_Left };
	case 0xf703: return Mapped_key { Qt::Key_Right };
	case 0xf704: return Mapped_key { Qt::Key_F1 };
	case 0xf705: return Mapped_key { Qt::Key_F2 };
	case 0xf706: return Mapped_key { Qt::Key_F3 };
	case 0xf707: return Mapped_key { Qt::Key_F4 };
	case 0xf708: return Mapped_key { Qt::Key_F5 };
	case 0xf709: return Mapped_key { Qt::Key_F6 };
	case 0xf70a: return Mapped_key { Qt::Key_F7 };
	case 0xf70b: return Mapped_key { Qt::Key_F8 };
	case 0xf70c: return Mapped_key { Qt::Key_F9 };
	case 0xf70d: return Mapped_key { Qt::Key_F10 };
	case 0xf70e: return Mapped_key { Qt::Key_F11 };
	case 0xf70f: return Mapped_key { Qt::Key_F12 };
	case 0xf727: return Mapped_key { Qt::Key_Insert };
	case 0xf728: return Mapped_key { Qt::Key_Delete };
	case 0xf729: return Mapped_key { Qt::Key_Home };
	case 0xf72b: return Mapped_key { Qt::Key_End };
	case 0xf72c: return Mapped_key { Qt::Key_PageUp };
	case 0xf72d: return Mapped_key { Qt::Key_PageDown };
	default: break;
	};

	/*
	 * Qt key enums are equal to the corresponding Unicode codepoints of the
	 * upper-case character.
	 */

	/* printable keys */
	if ((codepoint.value >= (unsigned)Qt::Key_Space) &&
	    (codepoint.value <= (unsigned)Qt::Key_ydiaeresis))
		return Mapped_key { Qt::Key(QChar(codepoint.value).toUpper().unicode()), codepoint };

	return Mapped_key { };
}


QGenodePlatformWindow::Mapped_key QGenodePlatformWindow::_map_key(Input::Keycode    key,
                                                                  Codepoint         codepoint,
                                                                  Mapped_key::Event e)
{
	/* non-printable key mappings */
	switch (key) {
	case Input::KEY_ENTER:        return Mapped_key { Qt::Key_Return };
	case Input::KEY_KPENTER:      return Mapped_key { Qt::Key_Return }; /* resolves aliasing on repeat */
	case Input::KEY_ESC:          return Mapped_key { Qt::Key_Escape };
	case Input::KEY_TAB:          return Mapped_key { Qt::Key_Tab };
	case Input::KEY_BACKSPACE:    return Mapped_key { Qt::Key_Backspace };
	case Input::KEY_INSERT:       return Mapped_key { Qt::Key_Insert };
	case Input::KEY_DELETE:       return Mapped_key { Qt::Key_Delete };
	case Input::KEY_PRINT:        return Mapped_key { Qt::Key_Print };
	case Input::KEY_CLEAR:        return Mapped_key { Qt::Key_Clear };
	case Input::KEY_HOME:         return Mapped_key { Qt::Key_Home };
	case Input::KEY_END:          return Mapped_key { Qt::Key_End };
	case Input::KEY_LEFT:         return Mapped_key { Qt::Key_Left };
	case Input::KEY_UP:           return Mapped_key { Qt::Key_Up };
	case Input::KEY_RIGHT:        return Mapped_key { Qt::Key_Right };
	case Input::KEY_DOWN:         return Mapped_key { Qt::Key_Down };
	case Input::KEY_PAGEUP:       return Mapped_key { Qt::Key_PageUp };
	case Input::KEY_PAGEDOWN:     return Mapped_key { Qt::Key_PageDown };
	case Input::KEY_LEFTSHIFT:    return Mapped_key { Qt::Key_Shift };
	case Input::KEY_RIGHTSHIFT:   return Mapped_key { Qt::Key_Shift };
	case Input::KEY_LEFTCTRL:     return Mapped_key { Qt::Key_Control };
	case Input::KEY_RIGHTCTRL:    return Mapped_key { Qt::Key_Control };
	case Input::KEY_LEFTMETA:     return Mapped_key { Qt::Key_Meta };
	case Input::KEY_RIGHTMETA:    return Mapped_key { Qt::Key_Meta };
	case Input::KEY_LEFTALT:      return Mapped_key { Qt::Key_Alt };
	case Input::KEY_RIGHTALT:     return Mapped_key { Qt::Key_AltGr };
	case Input::KEY_COMPOSE:      return Mapped_key { Qt::Key_Menu };
	case Input::KEY_CAPSLOCK:     return Mapped_key { Qt::Key_CapsLock };
	case Input::KEY_SYSRQ:        return Mapped_key { Qt::Key_SysReq };
	case Input::KEY_SCROLLLOCK:   return Mapped_key { Qt::Key_ScrollLock };
	case Input::KEY_PAUSE:        return Mapped_key { Qt::Key_Pause };
	case Input::KEY_F1:           return Mapped_key { Qt::Key_F1 };
	case Input::KEY_F2:           return Mapped_key { Qt::Key_F2 };
	case Input::KEY_F3:           return Mapped_key { Qt::Key_F3 };
	case Input::KEY_F4:           return Mapped_key { Qt::Key_F4 };
	case Input::KEY_F5:           return Mapped_key { Qt::Key_F5 };
	case Input::KEY_F6:           return Mapped_key { Qt::Key_F6 };
	case Input::KEY_F7:           return Mapped_key { Qt::Key_F7 };
	case Input::KEY_F8:           return Mapped_key { Qt::Key_F8 };
	case Input::KEY_F9:           return Mapped_key { Qt::Key_F9 };
	case Input::KEY_F10:          return Mapped_key { Qt::Key_F10 };
	case Input::KEY_F11:          return Mapped_key { Qt::Key_F11 };
	case Input::KEY_F12:          return Mapped_key { Qt::Key_F12 };
	case Input::KEY_F13:          return Mapped_key { Qt::Key_F13 };
	case Input::KEY_F14:          return Mapped_key { Qt::Key_F14 };
	case Input::KEY_F15:          return Mapped_key { Qt::Key_F15 };
	case Input::KEY_F16:          return Mapped_key { Qt::Key_F16 };
	case Input::KEY_F17:          return Mapped_key { Qt::Key_F17 };
	case Input::KEY_F18:          return Mapped_key { Qt::Key_F18 };
	case Input::KEY_F19:          return Mapped_key { Qt::Key_F19 };
	case Input::KEY_F20:          return Mapped_key { Qt::Key_F20 };
	case Input::KEY_F21:          return Mapped_key { Qt::Key_F21 };
	case Input::KEY_F22:          return Mapped_key { Qt::Key_F22 };
	case Input::KEY_F23:          return Mapped_key { Qt::Key_F23 };
	case Input::KEY_F24:          return Mapped_key { Qt::Key_F24 };
	case Input::KEY_BACK:         return Mapped_key { Qt::Key_Back };
	case Input::KEY_FORWARD:      return Mapped_key { Qt::Key_Forward };
	case Input::KEY_VOLUMEDOWN:   return Mapped_key { Qt::Key_VolumeDown };
	case Input::KEY_MUTE:         return Mapped_key { Qt::Key_VolumeMute };
	case Input::KEY_VOLUMEUP:     return Mapped_key { Qt::Key_VolumeUp };
	case Input::KEY_PREVIOUSSONG: return Mapped_key { Qt::Key_MediaPrevious };
	case Input::KEY_PLAYPAUSE:    return Mapped_key { Qt::Key_MediaTogglePlayPause };
	case Input::KEY_NEXTSONG:     return Mapped_key { Qt::Key_MediaNext };

	default: break;
	};

	/*
	 * We remember the mapping of pressed keys (but not repeated codepoints) in
	 * '_pressed' to derive the release mapping.
	 */

	switch (e) {
	case Mapped_key::PRESSED:
	case Mapped_key::REPEAT:
		{
			Mapped_key const mapped_key = _mapped_key_from_codepoint(codepoint);
			if (mapped_key.key != Qt::Key_unknown) {
				/* do not insert repeated codepoints */
				if (e == Mapped_key::PRESSED)
					_pressed.insert(key, mapped_key.key);

				return mapped_key;
			}
		} break;

	case Mapped_key::RELEASED:
		if (Qt::Key qt_key = _pressed.take(key)) {
			return Mapped_key { qt_key };
		}
		break;
	}

	/* dead keys and aborted sequences end up here */
	Genode::warning("key (", Input::key_name(key), ",", (unsigned)key,
	                ",U+", Genode::Hex((unsigned short)codepoint.value,
	                                   Genode::Hex::OMIT_PREFIX, Genode::Hex::PAD),
	                ") lacks Qt mapping");
	return Mapped_key { Qt::Key_unknown, codepoint };
}


void QGenodePlatformWindow::_key_event(Input::Keycode key, Codepoint codepoint,
                                       Mapped_key::Event e)
{
	bool const pressed = e != Mapped_key::RELEASED;

	Qt::KeyboardModifier current_modifier = Qt::NoModifier;

	/* FIXME ignores two keys for one modifier were pressed and only one was released */
	switch (key) {
	case Input::KEY_LEFTALT:    current_modifier = Qt::AltModifier;     break;
	case Input::KEY_LEFTCTRL:
	case Input::KEY_RIGHTCTRL:  current_modifier = Qt::ControlModifier; break;
	case Input::KEY_LEFTSHIFT:
	case Input::KEY_RIGHTSHIFT: current_modifier = Qt::ShiftModifier;   break;
	default: break;
	}

	_keyboard_modifiers.setFlag(current_modifier, pressed);

	QEvent::Type const event_type = pressed ? QEvent::KeyPress : QEvent::KeyRelease;
	Mapped_key   const mapped_key = _map_key(key, codepoint, e);
	unsigned     const unicode    = mapped_key.codepoint.valid()
	                              ? mapped_key.codepoint.value : 0;
	bool         const autorepeat = e == Mapped_key::REPEAT;

	QWindowSystemInterface::handleExtendedKeyEvent(window(),
	                                               event_type,
	                                               mapped_key.key,
	                                               _keyboard_modifiers,
	                                               key, 0, int(_keyboard_modifiers),
	                                               unicode ? QString(unicode) : QString(),
	                                               autorepeat);
}


void QGenodePlatformWindow::_mouse_button_event(Input::Keycode button, bool press)
{
	Qt::MouseButton current_mouse_button = Qt::NoButton;

	switch (button) {
	case Input::BTN_LEFT:    current_mouse_button = Qt::LeftButton;   break;
	case Input::BTN_RIGHT:   current_mouse_button = Qt::RightButton;  break;
	case Input::BTN_MIDDLE:  current_mouse_button = Qt::MidButton;    break;
	case Input::BTN_SIDE:    current_mouse_button = Qt::ExtraButton1; break;
	case Input::BTN_EXTRA:   current_mouse_button = Qt::ExtraButton2; break;
	case Input::BTN_FORWARD: current_mouse_button = Qt::ExtraButton3; break;
	case Input::BTN_BACK:    current_mouse_button = Qt::ExtraButton4; break;
	case Input::BTN_TASK:    current_mouse_button = Qt::ExtraButton5; break;
	default: return;
	}

	_mouse_button_state.setFlag(current_mouse_button, press);

	/* on mouse click, make this window the focused window */
	if (press) requestActivateWindow();

	QWindowSystemInterface::handleMouseEvent(window(),
	                                         _local_position(),
	                                         _global_position(),
	                                         _mouse_button_state,
	                                         current_mouse_button,
	                                         press ? QEvent::MouseButtonPress
	                                               : QEvent::MouseButtonRelease,
	                                         _keyboard_modifiers);
}


void QGenodePlatformWindow::_handle_input()
{
	_signal_proxy.input();
}


void QGenodePlatformWindow::_handle_hover_enter()
{
	if (!_hovered) {

		/*
		 * If a different window was hovered before and has
		 * not processed its leave event yet, let it report
		 * the leave event now to update the hover state of
		 * the previously hovered widget ('qt_last_mouse_receiver'
		 * in 'qwidgetwindow.cpp') before the variable gets
		 * updated with the new hovered widget.
		 */

		for (QWindow *window : QGuiApplication::topLevelWindows()) {
			if (window->handle()) {
				QGenodePlatformWindow *platform_window =
					static_cast<QGenodePlatformWindow*>(window->handle());
				platform_window->handle_hover_leave();
			}
		}

		_hovered = true;
		QWindowSystemInterface::handleEnterEvent(window());
	}
}


void QGenodePlatformWindow::handle_hover_leave()
{
	if (_hovered) {
		_hovered = false;
		QWindowSystemInterface::handleLeaveEvent(window());
	}
}


void QGenodePlatformWindow::_input()
{
	QList<Input::Event> touch_events;

	_input_session.for_each_event([&] (Input::Event const &event) {

		if (event.hover_leave()) {
			handle_hover_leave();
			return;
		}

		event.handle_absolute_motion([&] (int x, int y) {

			_handle_hover_enter();

			_mouse_position = QPoint(x, y);

			QWindowSystemInterface::handleMouseEvent(window(),
			                                         _local_position(),
			                                         _global_position(),
			                                         _mouse_button_state,
			                                         Qt::NoButton,
			                                         QEvent::MouseMove,
			                                         _keyboard_modifiers);
		});

		event.handle_press([&] (Input::Keycode key, Codepoint codepoint) {
			if (key > 0 && key < 0x100)
				_key_event(key, codepoint, Mapped_key::PRESSED);
			else if (key >= Input::BTN_LEFT && key <= Input::BTN_TASK)
				_mouse_button_event(key, true);
		});

		event.handle_release([&] (Input::Keycode key) {
			if (key > 0 && key < 0x100)
				_key_event(key, Codepoint { Codepoint::INVALID }, Mapped_key::RELEASED);
			else if (key >= Input::BTN_LEFT && key <= Input::BTN_TASK)
				_mouse_button_event(key, false);
		});

		event.handle_repeat([&] (Codepoint codepoint) {
			_key_event(Input::KEY_UNKNOWN, codepoint, Mapped_key::REPEAT);
		});

		event.handle_wheel([&] (int, int y) {
			QWindowSystemInterface::handleWheelEvent(window(),
			                                         _local_position(),
			                                         _local_position(),
			                                         QPoint(),
			                                         QPoint(0, y * 120),
			                                         _keyboard_modifiers); });

		if (event.touch() || event.touch_release())
			touch_events.push_back(event);
	});

	/* process all gathered touch events */
	_process_touch_events(touch_events);
}


void QGenodePlatformWindow::_handle_info_changed()
{
	_signal_proxy.info_changed();
}


void QGenodePlatformWindow::_info_changed()
{
	bool window_area_valid = false;

	Gui::Area const window_area = _gui_connection.window().convert<Gui::Area>(
		[&] (Gui::Rect rect) { window_area_valid = true; return rect.area; },
		[&] (Gui::Undefined) { return Gui::Area { 1, 1 }; });

	if (!window_area_valid)
		return;

	if ((window_area.w == 0) && (window_area.h == 0)) {
		/* interpret a size of 0x0 as indication to close the window */
		QWindowSystemInterface::handleCloseEvent(window());
		/* don't actually set geometry to 0x0; either close or remain open */
		return;
	}

	if (window_area != _current_window_area) {

		QRect geo(geometry());
		geo.setWidth (window_area.w);
		geo.setHeight(window_area.h);

		QWindowSystemInterface::handleGeometryChange(window(), geo);
		QWindowSystemInterface::handleExposeEvent(window(),
		                                          QRect(QPoint(0, 0),
		                                                geo.size()));

		setGeometry(geo);
	}
}


void QGenodePlatformWindow::_create_view()
{
	if (_view_valid)
		return;

	if (window()->type() == Qt::Desktop)
		return;

	if (window()->type() == Qt::Dialog) {
		/*
		 * Dialogs are supposed to have a window decoration, so they
		 * get a top-level view regardless of 'transientParent()'.
		 */
		if (!_view_id.constructed())
			_view_id.construct(_view_ref, _gui_connection.view_ids);
		_gui_connection.view(_view_id->id(), { });
		_view_valid = true;
		return;
	}

	if (window()->transientParent()) {

		QGenodePlatformWindow *parent_platform_window =
			static_cast<QGenodePlatformWindow*>(window()->transientParent()->handle());

		Gui::View_ref parent_view_ref { };
		Gui::View_ids::Element parent_view_id { parent_view_ref, _gui_connection.view_ids };

		_gui_connection.associate(parent_view_id.id(), parent_platform_window->view_cap());

		if (!_view_id.constructed())
			_view_id.construct(_view_ref, _gui_connection.view_ids);

		_gui_connection.child_view(_view_id->id(), parent_view_id.id(), { });

		_gui_connection.release_view_id(parent_view_id.id());

		_view_valid = true;

		return;
	}

	if (!_view_id.constructed())
		_view_id.construct(_view_ref, _gui_connection.view_ids);

	_gui_connection.view(_view_id->id(), { });
	_view_valid = true;
}


void QGenodePlatformWindow::_destroy_view()
{
	if (!_view_valid)
		return;

	_gui_connection.destroy_view(_view_id->id());
	_view_valid = false;
}


void QGenodePlatformWindow::_init_view(const QRect &geo)
{
	if (!_view_valid)
		return;

	typedef Gui::Session::Command Command;

	_gui_connection.enqueue<Command::Geometry>(_view_id->id(),
		Gui::Rect(Gui::Point(geo.x(), geo.y()),
		          Gui::Area(geo.width(), geo.height())));

	_gui_connection.enqueue<Command::Title>(_view_id->id(), _title.constData());

	if (_raise) {
		_gui_connection.enqueue<Command::Front>(_view_id->id());
		_raise = false;
	}

	_gui_connection.execute();
}


void QGenodePlatformWindow::_adjust_and_set_geometry(const QRect &rect)
{
	QRect adjusted_rect(rect);

	if (window()->type() == Qt::ToolTip) {
		/* improve tooltip visibility */
		if (adjusted_rect.topLeft().y() + adjusted_rect.height() >
		    window()->transientParent()->geometry().height()) {
			int dy = -adjusted_rect.height() -
			         (adjusted_rect.topLeft().y() - QCursor::pos().y());
			adjusted_rect.translate(0, dy);
		}
	}

	QPlatformWindow::setGeometry(adjusted_rect);

	Framebuffer::Mode const mode { .area = { (unsigned)adjusted_rect.width(),
	                                         (unsigned)adjusted_rect.height() },
	                               .alpha = false };
	_gui_connection.buffer(mode);

	_current_window_area = mode.area;

	_framebuffer_changed = true;

	if (_view_valid) {
		QRect g(geometry());
		if (window()->transientParent() && (window()->type() != Qt::Dialog)) {
			/*
			 * Translate global position to parent-relative position,
			 * except for dialogs, which don't have a parent view.
			 */
			g.moveTo(window()->transientParent()->mapFromGlobal(g.topLeft()));
		}
		typedef Gui::Session::Command Command;
		_gui_connection.enqueue<Command::Geometry>(_view_id->id(),
			Gui::Rect(Gui::Point(g.x(), g.y()),
			Gui::Area(g.width(), g.height())));
		_gui_connection.execute();
	}

	if (_egl_surface != EGL_NO_SURFACE) {
		eglDestroySurface(_egl_display, _egl_surface);
		_egl_surface = EGL_NO_SURFACE;
	}

	emit framebuffer_changed();
}


QString QGenodePlatformWindow::_sanitize_label(QString label)
{
	enum { MAX_LABEL = 25 };

	/* remove any occurences of '"' */
	label.remove("\"");

	/* truncate label and append '..' */
	if (label.length() > MAX_LABEL) {
		label.truncate(MAX_LABEL - 2);
		label.append("..");
	}

	/* Make sure that the window is distinguishable by the layouter */
	if (label.isEmpty())
		label = QString("Untitled Window");

	if (_gui_session_label_list.contains(label))
		for (unsigned int i = 2; ; i++) {
			QString versioned_label = label + "." + QString::number(i);
			if (!_gui_session_label_list.contains(versioned_label)) {
				label = versioned_label;
				break;
			}
		}

	return label;
}


QGenodePlatformWindow::QGenodePlatformWindow(Genode::Env &env,
                                             QGenodeSignalProxyThread &signal_proxy,
                                             QWindow *window,
                                             EGLDisplay egl_display)
: QPlatformWindow(window),
  _env(env),
  _signal_proxy(signal_proxy),
  _gui_session_label(_sanitize_label(window->title())),
  _gui_connection(env, _gui_session_label.toStdString().c_str()),
  _gui_session(_gui_connection.cap()),
  _framebuffer_session(_gui_session.framebuffer()),
  _framebuffer(nullptr),
  _framebuffer_changed(false),
  _input_session(env.rm(), _gui_session.input()),
  _ev_buf(env.rm(), _input_session.dataspace()),
  _egl_display(egl_display),
  _input_signal_handler(_env.ep(), *this,
                        &QGenodePlatformWindow::_handle_input),
  _info_changed_signal_handler(_env.ep(), *this,
                               &QGenodePlatformWindow::_handle_info_changed),
  _touch_device(_init_touch_device())
{
	if (qnpw_verbose)
		if (window->transientParent())
			qDebug() << "QGenodePlatformWindow(): child window of" << window->transientParent();

	_gui_session_label_list.append(_gui_session_label);

	_input_session.sigh(_input_signal_handler);

	_gui_connection.info_sigh(_info_changed_signal_handler);

	/*
	 * Popup menus and tooltips should never get a window decoration,
	 * therefore we set a top level Qt window as 'transient parent'.
	 */
	if (!window->transientParent() &&
	    ((window->type() == Qt::Popup) || (window->type() == Qt::ToolTip))) {
		QWindow *top_window = QGuiApplication::topLevelWindows().first();
	    window->setTransientParent(top_window);
	}

	_adjust_and_set_geometry(geometry());

	connect(&signal_proxy, SIGNAL(input_signal()),
	        this, SLOT(_input()),
	        Qt::QueuedConnection);

	connect(&signal_proxy, SIGNAL(info_changed_signal()),
	        this, SLOT(_info_changed()),
	        Qt::QueuedConnection);

	_info_changed();
}

QGenodePlatformWindow::~QGenodePlatformWindow()
{
	_gui_session_label_list.removeOne(_gui_session_label);
}

QSurfaceFormat QGenodePlatformWindow::format() const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::format()";
	return QPlatformWindow::format();
}

void QGenodePlatformWindow::setGeometry(const QRect &rect)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setGeometry(" << rect << ")";

	_adjust_and_set_geometry(rect);

	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setGeometry() finished";
}

QRect QGenodePlatformWindow::geometry() const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::geometry(): returning" << QPlatformWindow::geometry();
	return QPlatformWindow::geometry();
}

QMargins QGenodePlatformWindow::frameMargins() const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::frameMargins()";
	return QPlatformWindow::frameMargins();
}

void QGenodePlatformWindow::setVisible(bool visible)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setVisible(" << visible << ")";

	if (visible) {

		_create_view();

		QRect g(geometry());

		if (window()->transientParent() && (window()->type() != Qt::Dialog)) {
			/*
			 * Translate global position to parent-relative position,
			 * except for dialogs, which don't have a parent view.
			 */
			g.moveTo(window()->transientParent()->mapFromGlobal(g.topLeft()));
		}

		_init_view(g);

		/*
		 * 'QWindowSystemInterface::handleExposeEvent()' was previously called
		 * via 'QPlatformWindow::setVisible()', but that method also called
		 * 'QWindowSystemInterface::flushWindowSystemEvents()', which had the
		 * negative effect that a button with a visible tooltip "lost" the
		 * mouse release event on a fast click, apparently because it got
		 * handled too early (during tooltip cleanup).
		 */
		QRect expose_rect(QPoint(), g.size());
		QWindowSystemInterface::handleExposeEvent(window(), expose_rect);

		/*
		 * xcb sends an enter event when a popup menu opens and this
		 * appears to be necessary for correct button display in some cases.
		 */
		if (window()->type() == Qt::Popup)
			QWindowSystemInterface::handleEnterEvent(window());

	} else {

		_destroy_view();

		QWindowSystemInterface::handleExposeEvent(window(), QRegion());

		/*
		 * xcb sends an enter event when a popup menu is closed and this
		 * appears to be necessary for correct button display in some cases.
		 */
		if (window()->type() == Qt::Popup)
			QWindowSystemInterface::handleEnterEvent(window()->transientParent(),
			                                         QCursor::pos());
	}

	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setVisible() finished";
}

void QGenodePlatformWindow::setWindowFlags(Qt::WindowFlags flags)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setWindowFlags(" << flags << ")";

	QPlatformWindow::setWindowFlags(flags);

	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setWindowFlags() finished";
}

void QGenodePlatformWindow::setWindowState(Qt::WindowStates state)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setWindowState(" << state << ")";

	QPlatformWindow::setWindowState(state);

	if ((state == Qt::WindowMaximized) || (state == Qt::WindowFullScreen)) {
		QRect screen_geometry { screen()->geometry() };
		QWindowSystemInterface::handleGeometryChange(window(), screen_geometry);
		QWindowSystemInterface::handleExposeEvent(window(),
		                                          QRect(QPoint(0, 0),
		                                                screen_geometry.size()));
		setGeometry(screen_geometry);
	}
}

WId QGenodePlatformWindow::winId() const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::winId()";
	return WId(this);
}

void QGenodePlatformWindow::setParent(const QPlatformWindow *)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setParent()";

	/* don't call the base class function which only prints a warning */
}

void QGenodePlatformWindow::setWindowTitle(const QString &title)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setWindowTitle(" << title << ")";

	QPlatformWindow::setWindowTitle(title);

	_title = title.toLocal8Bit();

	typedef Gui::Session::Command Command;

	if (_view_valid) {
		_gui_connection.enqueue<Command::Title>(_view_id->id(), _title.constData());
		_gui_connection.execute();
	}

	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setWindowTitle() finished";
}

void QGenodePlatformWindow::setWindowFilePath(const QString &title)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setWindowFilePath(" << title << ")";
	QPlatformWindow::setWindowFilePath(title);
}

void QGenodePlatformWindow::setWindowIcon(const QIcon &icon)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setWindowIcon()";
	QPlatformWindow::setWindowIcon(icon);
}

void QGenodePlatformWindow::raise()
{
	if (_view_valid) {
		/* bring the view to the top */
		_gui_connection.enqueue<Gui::Session::Command::Front>(_view_id->id());
		_gui_connection.execute();
	} else {
		_raise = true;
	}

	/* don't call the base class function which only prints a warning */
}

void QGenodePlatformWindow::lower()
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::lower()";

	/* don't call the base class function which only prints a warning */
}

bool QGenodePlatformWindow::isExposed() const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::isExposed()";
	return QPlatformWindow::isExposed();
}

bool QGenodePlatformWindow::isActive() const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::isActive()";
	return QPlatformWindow::isActive();
}

bool QGenodePlatformWindow::isEmbedded() const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::isEmbedded()";
	return QPlatformWindow::isEmbedded();
}

QPoint QGenodePlatformWindow::mapToGlobal(const QPoint &pos) const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::mapToGlobal(" << pos << ")";
	return QPlatformWindow::mapToGlobal(pos);
}

QPoint QGenodePlatformWindow::mapFromGlobal(const QPoint &pos) const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::mapFromGlobal(" << pos << ")";
	return QPlatformWindow::mapFromGlobal(pos);
}

void QGenodePlatformWindow::propagateSizeHints()
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::propagateSizeHints()";

	/* don't call the base class function which only prints a warning */
}

void QGenodePlatformWindow::setOpacity(qreal level)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setOpacity(" << level << ")";

	/* don't call the base class function which only prints a warning */
}

void QGenodePlatformWindow::setMask(const QRegion &region)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setMask(" << region << ")";

	/* don't call the base class function which only prints a warning */
}

void QGenodePlatformWindow::requestActivateWindow()
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::requestActivateWindow()";
	QPlatformWindow::requestActivateWindow();
}

void QGenodePlatformWindow::handleContentOrientationChange(Qt::ScreenOrientation orientation)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::handleContentOrientationChange()";
	QPlatformWindow::handleContentOrientationChange(orientation);
}

qreal QGenodePlatformWindow::devicePixelRatio() const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::devicePixelRatio()";
	return QPlatformWindow::devicePixelRatio();
}

bool QGenodePlatformWindow::setKeyboardGrabEnabled(bool)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setKeyboardGrabEnabled()";

	/* don't call the base class function which only prints a warning */

	return false;
}

bool QGenodePlatformWindow::setMouseGrabEnabled(bool)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setMouseGrabEnabled()";

	/* don't call the base class function which only prints a warning */

	return false;
}

bool QGenodePlatformWindow::setWindowModified(bool modified)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setWindowModified()";
	return QPlatformWindow::setWindowModified(modified);
}

bool QGenodePlatformWindow::windowEvent(QEvent *event)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::windowEvent(" << event->type() << ")";
	return QPlatformWindow::windowEvent(event);
}

bool QGenodePlatformWindow::startSystemResize(Qt::Edges edges)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::startSystemResize()";
	return QPlatformWindow::startSystemResize(edges);
}

void QGenodePlatformWindow::setFrameStrutEventsEnabled(bool enabled)
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::setFrameStrutEventsEnabled()";
	QPlatformWindow::setFrameStrutEventsEnabled(enabled);
}

bool QGenodePlatformWindow::frameStrutEventsEnabled() const
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::frameStrutEventsEnabled()";
	return QPlatformWindow::frameStrutEventsEnabled();
}

/* functions used by the window surface */

unsigned char *QGenodePlatformWindow::framebuffer()
{
	if (qnpw_verbose)
	    qDebug() << "QGenodePlatformWindow::framebuffer()" << _framebuffer;

	/*
	 * The new framebuffer is acquired in this function to avoid a time span when
	 * the GUI buffer would be black and not refilled yet by Qt.
	 */

	if (_framebuffer_changed) {

	    _framebuffer_changed = false;

		if (_framebuffer != nullptr)
		    _env.rm().detach((Genode::addr_t)_framebuffer);

		Genode::Region_map::Attr attr { };
		attr.writeable = true;
		_env.rm().attach(_framebuffer_session.dataspace(), attr).with_result(
			[&] (Genode::Env::Local_rm::Attachment &a) {
				a.deallocate = false;
				_framebuffer = (unsigned char*)a.ptr; },
			[&] (Genode::Env::Local_rm::Error) {
				_framebuffer = nullptr;
				Genode::error("could not attach framebuffer");
			}
		);
	}

	return _framebuffer;
}

void QGenodePlatformWindow::refresh(int x, int y, int w, int h)
{
	if (qnpw_verbose)
	    qDebug("QGenodePlatformWindow::refresh(%d, %d, %d, %d)", x, y, w, h);

	_framebuffer_session.refresh(x, y, w, h);
}

EGLSurface QGenodePlatformWindow::eglSurface(EGLConfig egl_config)
{
	if (_egl_surface == EGL_NO_SURFACE) {

		QRect geo = geometry();

		Genode_egl_window egl_window = { geo.width(),
		                                 geo.height(),
		                                 framebuffer(),
		                                 PIXMAP };

		_egl_surface = eglCreatePixmapSurface(_egl_display,
		                                      egl_config,
		                                      &egl_window, 0);

		if (_egl_surface == EGL_NO_SURFACE)
			qFatal("eglCreatePixmapSurface() failed");
	}

	return _egl_surface;
}

Gui::Session_client &QGenodePlatformWindow::gui_session()
{
	return _gui_session;
}

Gui::View_capability QGenodePlatformWindow::view_cap() const
{
	if (_view_valid) {
		QGenodePlatformWindow *non_const_platform_window =
			const_cast<QGenodePlatformWindow *>(this);
		return non_const_platform_window->_gui_connection.view_capability(_view_id->id());
	}

	return Gui::View_capability();
}

QT_END_NAMESPACE
