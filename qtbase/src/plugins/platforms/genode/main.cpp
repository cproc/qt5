/*
 * \brief  Genode QPA plugin
 * \author Christian Prochaska
 * \date   2013-05-08
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* libc includes */
#include <assert.h>

/* Qt includes */
#include "qgenodeintegrationplugin.h"

Genode::Env *QGenodeIntegrationPlugin::_env = nullptr;

extern "C" void initialize_qpa_plugin(Genode::Env &env) __attribute__ ((visibility ("default")));
void initialize_qpa_plugin(Genode::Env &env)
{
	QGenodeIntegrationPlugin::env(env);
}


QT_BEGIN_NAMESPACE


QStringList QGenodeIntegrationPlugin::keys() const
{
	QStringList list;
	list << "Gui";
	return list;
}


QPlatformIntegration *QGenodeIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
	Q_UNUSED(paramList);
	if (system.toLower() == "genode") {
		assert(_env != nullptr);
		return new QGenodeIntegration(*_env);
	}

	return 0;
}

QT_END_NAMESPACE
