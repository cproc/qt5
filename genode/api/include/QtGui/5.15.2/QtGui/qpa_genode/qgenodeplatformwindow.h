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


#ifndef _QGENODEPLATFORMWINDOW_H_
#define _QGENODEPLATFORMWINDOW_H_

/* Genode includes */
#include <util/reconstructible.h>
#include <input/event.h>
#include <gui_session/connection.h>

/* libc includes */
#include <libc/component.h>

/* EGL includes */
#include <EGL/egl.h>

/* Qt includes */
#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>
#include <qtouchdevice.h>

/* Qoost includes */
#include <qoost/qmember.h>

class QGenodeSignalProxyThread;

QT_BEGIN_NAMESPACE

class __attribute__ ((visibility ("default"))) QGenodePlatformWindow;
class QGenodePlatformWindow : public QObject, public QPlatformWindow
{
	Q_OBJECT

	private:

		Genode::Env                 &_env;
		QGenodeSignalProxyThread    &_signal_proxy;
		QString                      _gui_session_label;
		static QStringList           _gui_session_label_list;
		Gui::Connection              _gui_connection;
		Gui::Session_client          _gui_session;
		Framebuffer::Session_client  _framebuffer_session;
		unsigned char               *_framebuffer;
		bool                         _framebuffer_changed;
		Gui::Area                    _current_window_area;
		Input::Session_client        _input_session;
		Genode::Attached_dataspace   _ev_buf;
		QPoint                       _mouse_position;
		Qt::KeyboardModifiers        _keyboard_modifiers;
		Qt::MouseButtons             _mouse_button_state;
		QByteArray                   _title;
		EGLDisplay                   _egl_display;
		EGLSurface                   _egl_surface { EGL_NO_SURFACE };
		bool                         _hovered { false };
		bool                         _raise { true };

		Gui::View_ref                                 _view_ref { };
		Genode::Constructible<Gui::View_ids::Element> _view_id { };
		bool                                          _view_valid { false };

		QPoint _global_position() const
		{
			QWindow *w = window();

			if (!w->transientParent() || (w->type() == Qt::Dialog)) {
				/*
				 * '_mouse_position' is the global position for windows
				 * with top-level views.
				 */
				return _mouse_position;
			}

			/*
			 * '_mouse_position' is relative to the parent window
			 * with top-level view.
			 */

			while (w->transientParent() && (w->type() != Qt::Dialog))
				w = w->transientParent();

			return w->geometry().topLeft() + _mouse_position;
		}

		QPoint _local_position() const
		{
			return _global_position() - geometry().topLeft();
		}

		typedef Genode::Codepoint Codepoint;

		struct Mapped_key
		{
			enum Event { PRESSED, RELEASED, REPEAT };

			Qt::Key   key       { Qt::Key_unknown };
			Codepoint codepoint { Codepoint::INVALID };
		};

		QHash<Input::Keycode, Qt::Key> _pressed;

		Mapped_key _mapped_key_from_codepoint(Codepoint);
		Mapped_key _map_key(Input::Keycode, Codepoint, Mapped_key::Event);
		void _key_event(Input::Keycode, Codepoint, Mapped_key::Event);
		void _mouse_button_event(Input::Keycode, bool press);

		Genode::Io_signal_handler<QGenodePlatformWindow> _input_signal_handler;
		Genode::Io_signal_handler<QGenodePlatformWindow> _info_changed_signal_handler;

		void _handle_input();
		void _handle_info_changed();

		QVector<QWindowSystemInterface::TouchPoint>  _touch_points { 16 };
		QTouchDevice                                *_touch_device;
		QTouchDevice                                *_init_touch_device();

		void _process_touch_events(QList<Input::Event> const &events);

		void _create_view();
		void _init_view(const QRect &geo);
		void _destroy_view();

		void _adjust_and_set_geometry(const QRect &rect);

		QString _sanitize_label(QString label);

		void _handle_hover_enter();

		/*
		 * Genode signals are handled as Qt signals to avoid blocking in the
		 * Genode signal handler, which could cause nested signal handler
		 * execution.
		 */

	private Q_SLOTS:

		void _input();
		void _info_changed();

	public:

		QGenodePlatformWindow(Genode::Env &env,
		                      QGenodeSignalProxyThread &signal_proxy,
		                      QWindow *window,
		                      EGLDisplay egl_display);

		~QGenodePlatformWindow();

	    QSurfaceFormat format() const override;

	    void setGeometry(const QRect &rect) override;

	    QRect geometry() const override;

	    QMargins frameMargins() const override;

	    void setVisible(bool visible) override;

	    void setWindowFlags(Qt::WindowFlags flags) override;

	    void setWindowState(Qt::WindowStates state) override;

	    WId winId() const override;

	    void setParent(const QPlatformWindow *window) override;

	    void setWindowTitle(const QString &title) override;

	    void setWindowFilePath(const QString &title) override;

	    void setWindowIcon(const QIcon &icon) override;

	    void raise() override;

	    void lower() override;

	    bool isExposed() const override;

	    bool isActive() const override;

	    bool isEmbedded() const override;

	    QPoint mapToGlobal(const QPoint &pos) const override;

	    QPoint mapFromGlobal(const QPoint &pos) const override;

	    void propagateSizeHints() override;

	    void setOpacity(qreal level) override;

	    void setMask(const QRegion &region) override;

	    void requestActivateWindow() override;

	    void handleContentOrientationChange(Qt::ScreenOrientation orientation) override;

	    qreal devicePixelRatio() const override;

	    bool setKeyboardGrabEnabled(bool grab) override;

	    bool setMouseGrabEnabled(bool grab) override;

	    bool setWindowModified(bool modified) override;

	    bool windowEvent(QEvent *event) override;

	    bool startSystemResize(Qt::Edges edges) override;

	    void setFrameStrutEventsEnabled(bool enabled) override;

	    bool frameStrutEventsEnabled() const override;


	    /* for QGenodeWindowSurface */

	    unsigned char *framebuffer();

		void refresh(int x, int y, int w, int h);


		/* for QGenodeGLContext */

		EGLSurface eglSurface(EGLConfig egl_config);


		/* for QGenodeViewWidget */

		Gui::Session_client &gui_session();
		Gui::View_capability view_cap() const;


		/* for any QGenodePlatformWindow */

		void handle_hover_leave();

	signals:

		void framebuffer_changed();

};

QT_END_NAMESPACE

#endif /* _QGENODEPLATFORMWINDOW_H_ */
