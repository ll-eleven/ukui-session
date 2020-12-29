/*****************************************************************
ksmserver - the KDE session management server

Copyright 2000 Matthias Ettrich <ettrich@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/
#include <fixx11h.h>
//#include <config-workspace.h>
//#include <config-ksmserver.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <KMessageBox>

#include <kconfiggroup.h>
#include <kdbusservice.h>
#include <KLocalizedString>
#include <kconfig.h>
#include <KSharedConfig>
#include <kmanagerselection.h>
#include <kwindowsystem.h>
//#include <ksmserver_debug.h>
#include "server.h"
#include <QX11Info>

#include <QApplication>
#include <QCommandLineParser>
#include <QQuickWindow>
#include <X11/extensions/Xrender.h>

static const char version[] = "0.4";
static const char description[] = I18N_NOOP( "The reliable KDE session manager that talks the standard X11R6 \nsession management protocol (XSMP)." );

Display* dpy = nullptr;
Colormap colormap = 0;
Visual *visual = nullptr;

extern KSMServer* the_server;

void IoErrorHandler ( IceConn iceConn)
{
    the_server->ioError( iceConn );
}


//extern "C" Q_DECL_EXPORT
int main( int argc, char* argv[] )
{
//    sanity_check(argc, argv);//命令执行参数检查

//    putenv((char*)"SESSION_MANAGER=");
//    checkComposite();//kwin混成器检查

    // force xcb QPA plugin as ksmserver is very X11 specific
    const QByteArray origQpaPlatform = qgetenv("QT_QPA_PLATFORM");
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("xcb"));

//    QQuickWindow::setDefaultAlphaBuffer(true);
    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication *a = new QApplication(argc, argv);

    // now the QPA platform is set, unset variable again to not launch apps with incorrect environment
    if (origQpaPlatform.isEmpty()) {
        qunsetenv("QT_QPA_PLATFORM");
    } else {
        qputenv("QT_QPA_PLATFORM", origQpaPlatform);
    }

    QApplication::setApplicationName( QStringLiteral( "ksmserver") );
    QApplication::setApplicationVersion( QString::fromLatin1( version ) );
    QApplication::setOrganizationDomain( QStringLiteral( "kde.org") );

    fcntl(ConnectionNumber(QX11Info::display()), F_SETFD, 1);

//    a->setQuitOnLastWindowClosed(false); // #169486

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n(description));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption restoreOption(QStringList() << QStringLiteral("r") << QStringLiteral("restore"),
                                     i18n("Restores the saved user session if available"));
    parser.addOption(restoreOption);

    QCommandLineOption wmOption(QStringList() << QStringLiteral("w") << QStringLiteral("windowmanager"),
                                i18n("Starts <wm> in case no other window manager is \nparticipating in the session. Default is 'kwin'"),
                                i18n("wm"));
    parser.addOption(wmOption);

    QCommandLineOption nolocalOption(QStringLiteral("nolocal"),
                                     i18n("Also allow remote connections"));
    parser.addOption(nolocalOption);

    QCommandLineOption lockscreenOption(QStringLiteral("lockscreen"),
                                        i18n("Starts the session in locked mode"));
    parser.addOption(lockscreenOption);

    QCommandLineOption noLockscreenOption(QStringLiteral("no-lockscreen"),
                                         i18n("Starts without lock screen support. Only needed if other component provides the lock screen."));
    parser.addOption(noLockscreenOption);

    parser.process(*a);

    QString wm = parser.value(wmOption);

    bool only_local = !parser.isSet(nolocalOption);
#ifndef HAVE__ICETRANSNOLISTEN
    /* this seems strange, but the default is only_local, so if !only_local
     * the option --nolocal was given, and we warn (the option --nolocal
     * does nothing on this platform, as here the default is reversed)
     */
    if (!only_local) {
        qWarning("--nolocal is not supported on your platform. Sorry.");
    }
    only_local = false;
#endif

    KSMServer::InitFlags flags = KSMServer::InitFlag::None;
    if (only_local) {
        flags |= KSMServer::InitFlag::OnlyLocal;
    }
    if (parser.isSet(lockscreenOption)) {
        flags |= KSMServer::InitFlag::ImmediateLockScreen;
    }
    if (parser.isSet(noLockscreenOption)) {
        flags |= KSMServer::InitFlag::NoLockScreen;
    }

    qDebug()<<"初始化 ksmserver";
    KSMServer *server = new KSMServer( wm, flags);

    // for the KDE-already-running check in startkde
//    KSelectionOwner kde_running( "_KDE_RUNNING", 0 );
//    kde_running.claim( false );

    IceSetIOErrorHandler( IoErrorHandler );

//    KConfigGroup config(KSharedConfig::openConfig(), "General");

//    QString loginMode = config.readEntry( "loginMode", "restorePreviousLogout" );

//    if ( parser.isSet( restoreOption ))
//        server->restoreSession( QStringLiteral( SESSION_BY_USER ) );
//    else if ( loginMode == QLatin1String( "restorePreviousLogout" ) )
//        server->restoreSession( QStringLiteral( SESSION_PREVIOUS_LOGOUT ) );
//    else if ( loginMode == QLatin1String( "restoreSavedSession" ) )
//        server->restoreSession( QStringLiteral( SESSION_BY_USER ) );
//    else
    qDebug()<<"main 函数 server->startDefaultSession();";
    server->startDefaultSession();

//    KDBusService service(KDBusService::Unique);

//    server->setupShortcuts();
    int ret = a->exec();
//    kde_running.release(); // needs to be done before QApplication destruction
    delete a;
    return ret;
}
