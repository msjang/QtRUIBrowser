/*
 * Copyright (C) 2012, 2013 Cable Television Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "mainwindow.h"

#include "locationedit.h"
#include "browsersettings.h"
#include "utils.h"
#include "ruiwebpage.h"

// Temp
//#include "discoverystub.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QKeyEvent>
#include <QAction>
#include <QSplitter>
#include <QWebView>
#include <QWebPage>
#include <QCompleter>
#include <QUrl>
#include <QTimer>
#include <QFrame>
#include <QNetworkProxy>

#define TV_REMOTE_SIMULATOR 1

static const int DEFAULT_HEIGHT = 700;
static const int DEFAULT_WIDTH = 1000;

const char* rui_home = "qrc:/www/index.html";

MainWindow::MainWindow(bool startFullScreen)
    : m_page(0)
    , m_navigationBar(0)
    , m_urlEdit(0)
    , m_discoveryProxy(0)
    , m_browserSettings(BrowserSettings::Instance())
{
    if (startFullScreen)
        m_browserSettings->startFullScreen = true;

    // We house the RUI webview and the web inspector in a splitter.
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    setCentralWidget(splitter);
    splitter->setMinimumWidth(800);
    splitter->setMinimumHeight(450);
    splitter->resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    // RUI webview
    m_page = new RUIWebPage(this);
    m_view = new QWebView(splitter);
    m_view->setPage(m_page);
    m_view->installEventFilter(this);
    m_view->resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    m_inspector = new WebInspector;
    connect(this, SIGNAL(destroyed()), m_inspector, SLOT(deleteLater()));

    splitter->addWidget(m_inspector);
    m_inspector->setPage(m_page);
    if (!m_browserSettings->hasWebInspector) {
        m_inspector->hide();
    }

    buildUI();

    // Process window settings
    Qt::WindowFlags flags = this->windowFlags();

    if (!m_browserSettings->hasTitleBar) {
        flags |= Qt::FramelessWindowHint;
        flags &= ~Qt::WindowMinMaxButtonsHint;  // These buttons force the title bar.
    }

    if (m_browserSettings->staysOnTop) {
        flags |= Qt::WindowStaysOnTopHint;
    }

    if (flags != this->windowFlags()) {
        setWindowFlags(flags);
    }

    resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    if (m_browserSettings->startFullScreen) {
        fullScreen(true);
    }

    // Discovery Proxy
    m_discoveryProxy = DiscoveryProxy::Instance();

    // Connect proxy load signals
    attachProxyObject();
    connect(m_page->mainFrame(), SIGNAL(loadStarted()), this, SLOT(onLoadStarted()));
    connect(m_view, SIGNAL(loadFinished(bool)), this, SLOT(onPageLoaded(bool)));
    connect(m_page->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(onJavaScriptWindowObjectCleared()));
}

void MainWindow::buildUI()
{
    delete m_navigationBar;
    m_navigationBar = 0;

    if (m_browserSettings->hasMenuBar) {
        createMenuBar();
    }

    m_navigationBar = addToolBar("Navigation");

    if (m_browserSettings->hasBackButton) {
        m_navigationBar->addAction(m_page->action(QWebPage::Back));
    }

    if (m_browserSettings->hasStopButton) {
        m_navigationBar->addAction(m_page->action(QWebPage::Stop));
    }

    if (m_browserSettings->hasForwardButton) {
        m_navigationBar->addAction(m_page->action(QWebPage::Forward));
    }

    if (m_browserSettings->hasReloadButton) {
        QAction* reloadAction = m_page->action(QWebPage::Reload);
        connect(reloadAction, SIGNAL(triggered()), this, SLOT(changeLocation()));
        m_navigationBar->addAction(reloadAction);
    }

    if (!m_browserSettings->hasHomeButton) {
        // Not implemented yet. Have at it.
    }

    if (m_browserSettings->hasUrlEdit) {
        m_urlEdit = new LocationEdit(m_navigationBar);
        m_urlEdit->setSizePolicy(QSizePolicy::Expanding, m_urlEdit->sizePolicy().verticalPolicy());
        connect(m_urlEdit, SIGNAL(returnPressed()), SLOT(changeLocation()));
        QCompleter* completer = new QCompleter(m_navigationBar);
        m_urlEdit->setCompleter(completer);
        completer->setModel(&m_urlModel);
        m_navigationBar->addWidget(m_urlEdit);

        connect(m_page->mainFrame(), SIGNAL(urlChanged(QUrl)), this, SLOT(setAddressUrl(QUrl)));
        connect(m_page, SIGNAL(loadProgress(int)), m_urlEdit, SLOT(setProgress(int)));
        connect(m_page->mainFrame(), SIGNAL(iconChanged()), this, SLOT(onIconChanged()));
    }

    if (!m_browserSettings->hasNavigationBar) {
        m_navigationBar->hide();
    }


    if (m_browserSettings->hasTitleBar) {
        connect(m_page->mainFrame(), SIGNAL(titleChanged(QString)), this, SLOT(onTitleChanged(QString)));
    }

    // Not sure what this does
    connect(m_page, SIGNAL(windowCloseRequested()), this, SLOT(close()));

#ifndef QT_NO_SHORTCUT
    // short-cuts
    m_page->action(QWebPage::Back)->setShortcut(QKeySequence::Back);
    m_page->action(QWebPage::Stop)->setShortcut(Qt::Key_Escape);
    m_page->action(QWebPage::Forward)->setShortcut(QKeySequence::Forward);
    m_page->action(QWebPage::Reload)->setShortcut(QKeySequence::Refresh);

    // TODO: Home key shortcut
#endif
}

void MainWindow::createMenuBar()
{
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(tr("Open File..."), this, SLOT(openFile()), QKeySequence::Open);
    fileMenu->addAction(tr("Open Location..."), this, SLOT(openLocation()), QKeySequence(Qt::CTRL | Qt::Key_L));
    fileMenu->addAction("Close Window", this, SLOT(close()), QKeySequence::Close);
    fileMenu->addSeparator();
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", QApplication::instance(), SLOT(closeAllWindows()), QKeySequence(Qt::CTRL | Qt::Key_Q));

    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(m_page->action(QWebPage::Stop));
    viewMenu->addAction(m_page->action(QWebPage::Reload));
    viewMenu->addAction("Full Screen", this, SLOT(fullScreenOn()));
    viewMenu->addSeparator();
    QAction* showNavigationBar = viewMenu->addAction("Navigation Bar", this, SLOT(toggleNavigationBar(bool)));
    showNavigationBar->setCheckable(true);
    showNavigationBar->setChecked(m_browserSettings->hasNavigationBar);

    QMenu* debugMenu = menuBar()->addMenu("&Debug");
    debugMenu->addAction("Dump User Interface Map", this, SLOT(dumpUserInterfaceMap()));
    debugMenu->addSeparator();
    debugMenu->addAction("Dump HTML", this, SLOT(dumpHtml()));
    QString enableProxy = "Enable ";
    enableProxy += m_browserSettings->proxyType;
    enableProxy += " Proxy";
    QAction* enableHttpProxy = debugMenu->addAction(enableProxy, this, SLOT(toggleHttpProxy(bool)));
    enableHttpProxy->setCheckable(true);
    enableHttpProxy->setChecked(m_browserSettings->proxyEnabled);
    debugMenu->addSeparator();
    QAction* showInspectorAction = debugMenu->addAction("Web Inspector", this, SLOT(toggleWebInspector(bool)), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_I));
    showInspectorAction->setCheckable(true);
    showInspectorAction->connect(m_inspector, SIGNAL(visibleChanged(bool)), SLOT(setChecked(bool)));
}

void MainWindow::fullScreenOn()
{
    fullScreen(true);
}

void MainWindow::fullScreen(bool on)
{
    if (on) {
        menuBar()->hide();
        m_navigationBar->hide();
        m_inspector->setVisible(false);
        setWindowState( windowState() | Qt::WindowFullScreen );
        QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    } else {
        setWindowState( windowState() & ~Qt::WindowFullScreen );
        showNormal();
        if (m_browserSettings->hasMenuBar) {
            menuBar()->show();
        }
        if (m_browserSettings->hasNavigationBar) {
            m_navigationBar->show();
        }
        m_inspector->setVisible(m_browserSettings->hasWebInspector);
        QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    }
}

void MainWindow::openFile()
{
    static const QString filter("HTML Files (*.htm *.html);;Text Files (*.txt);;Image Files (*.gif *.jpg *.png);;All Files (*)");

    QFileDialog fileDialog(this, tr("Open"), QString(), filter);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setOptions(QFileDialog::ReadOnly);

    if (fileDialog.exec()) {
        QString selectedFile = fileDialog.selectedFiles()[0];
        if (!selectedFile.isEmpty())
            load(QUrl::fromLocalFile(selectedFile));
    }
}

void MainWindow::home()
{
    // TODO: if RUI list > 1, display list, otherwise display default.

    // Default RUI. What if there is no default RUI and nothing is discovered?

    load("rui:home");
    return;

    QString url = m_browserSettings->defaultRUIUrl;
    if (url.size() > 0 ) {
        load(m_browserSettings->defaultRUIUrl);
    }
}

void MainWindow::setAddressUrl(const QUrl& url)
{
    setAddressUrl(url.toString(QUrl::RemoveUserInfo));
}

void MainWindow::setAddressUrl(const QString& url)
{
    if (m_browserSettings->hasUrlEdit) {
        if (!url.contains("about:")) {
            m_urlEdit->setText(url);
        }
    }
}

void MainWindow::addCompleterEntry(const QUrl& url)
{
    QUrl::FormattingOptions opts;
    opts |= QUrl::RemoveScheme;
    opts |= QUrl::RemoveUserInfo;
    opts |= QUrl::StripTrailingSlash;
    QString s = url.toString(opts);
    s = s.mid(2);
    if (s.isEmpty())
        return;

    if (!m_urlList.contains(s))
        m_urlList += s;
    m_urlModel.setStringList(m_urlList);
}

void MainWindow::load(const QString& url)
{
    QUrl qurl;

    if (url.compare("rui:home") == 0) {
        qurl = QUrl(rui_home);
    } else {
        qurl = urlFromUserInput(url);
    }

    if (qurl.scheme().isEmpty()) {
        qurl = QUrl("http://" + url + "/");
    }

    load(qurl);
}

void MainWindow::load(const QUrl& url)
{
    if (!url.isValid())
        return;

    setAddressUrl(url.toString());
    m_page->mainFrame()->load(url);
}

QString MainWindow::addressUrl() const
{
    if (m_browserSettings->hasUrlEdit) {
        return m_urlEdit->text();
    } else {
        return QString();
    }
}

void MainWindow::changeLocation()
{
    if (m_browserSettings->hasUrlEdit) {
        QString string = m_urlEdit->text();
        QUrl mainFrameURL = m_page->mainFrame()->url();

        if (mainFrameURL.isValid() && string == mainFrameURL.toString()) {
            m_page->triggerAction(QWebPage::Reload);
            return;
        }

        load(string);
    }
}

// Debugging
void MainWindow::dumpHtml()
{
    QString html = m_page->mainFrame()->toHtml();
    fprintf(stderr,"\nHTML:\n%s\n", html.toUtf8().data());
}

void MainWindow::dumpUserInterfaceMap()
{
    m_discoveryProxy->dumpUserInterfaceMap();
}

void MainWindow::toggleNavigationBar(bool b)
{
    m_browserSettings->hasNavigationBar = b;
    m_browserSettings->save();
    b ? m_navigationBar->show() : m_navigationBar->hide();
}

void MainWindow::checkHttpProxyEnabled()
{
    if (m_browserSettings->proxyEnabled) {
        toggleHttpProxy(true);
    }
}

void MainWindow::enableHttpProxy()
{
    QNetworkProxy proxy;
    if ( m_browserSettings->proxyType.compare("Http",Qt::CaseInsensitive) == 0) {
        proxy.setType(QNetworkProxy::HttpProxy);
    } else if ( m_browserSettings->proxyType.compare("Socks5",Qt::CaseInsensitive) == 0) {
        proxy.setType(QNetworkProxy::Socks5Proxy);
    } else {
        fprintf(stderr,"Invalid proxy type in .ini file: %s. Should be Http or Socks5. Defaulting to Http\n", m_browserSettings->proxyType.toUtf8().data());
        proxy.setType(QNetworkProxy::HttpProxy);
        m_browserSettings->proxyType = "Http";
        m_browserSettings->save();
    }
    proxy.setHostName(m_browserSettings->proxyHost);
    proxy.setPort(m_browserSettings->proxyPort);
    QNetworkProxy::setApplicationProxy(proxy);
}

void MainWindow::toggleHttpProxy(bool b)
{
    m_browserSettings->proxyEnabled = b;
    m_browserSettings->save();

    if (b) {
        enableHttpProxy();
    } else {
        QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
    }
}

void MainWindow::toggleWebInspector(bool b) {
    m_browserSettings->hasWebInspector = b;
    m_browserSettings->save();
    m_inspector->setVisible(b);
}


// Don't think we need this
void MainWindow::openLocation()
{
    if (m_browserSettings->hasUrlEdit) {
        m_urlEdit->selectAll();
        m_urlEdit->setFocus();
    }
}

void MainWindow::onIconChanged()
{
    if (m_browserSettings->hasUrlEdit) {
        m_urlEdit->setPageIcon(m_page->mainFrame()->icon());
    }
}

void MainWindow::onLoadStarted()
{
    if (m_browserSettings->hasUrlEdit) {
        m_urlEdit->setPageIcon(QIcon());
    }

    m_discoveryProxy->m_home = false;
}

void MainWindow::onTitleChanged(const QString& title)
{
    if (title.isEmpty())
        setWindowTitle(QCoreApplication::applicationName());
    else
        setWindowTitle(QString::fromLatin1("%1 - %2").arg(title).arg(QCoreApplication::applicationName()));
}

bool MainWindow::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {

        QKeyEvent* keyEvent = (QKeyEvent*)event;


        if (keyEvent->key() == Qt::Key_Escape) {
            home();
            return true;
        }
        else if (keyEvent->key() == Qt::Key_F11) {

            bool isFullScreen = (windowState() & Qt::WindowFullScreen);
            fullScreen(!isFullScreen);
            return true;
        }
        else if (keyEvent->key() == Qt::Key_F1) {

            QCursor* cursor = QApplication::overrideCursor();
            if ( !cursor || cursor->shape() == Qt::ArrowCursor ) {
                QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
            }
            else {
                QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
            }
            return true;
        }

    }

    return QMainWindow::eventFilter(object, event);
}

void MainWindow::attachProxyObject()
{
    m_page->mainFrame()->addToJavaScriptWindowObject( QString("discoveryProxy"), m_discoveryProxy );
    m_discoveryProxy->m_home = true;
}

// Here when the global window object of the JavaScript environment is cleared, e.g., before starting a new load
// If this is the rui:home page, add our discovery proxy to the window.
void MainWindow::onJavaScriptWindowObjectCleared()
{
    QString url = m_view->url().toString();

    if (url.compare(rui_home) == 0) {
        attachProxyObject();
    } else {
        m_discoveryProxy->m_home = false;
    }
}

// Here when a page load is finished.
// If this is the rui:home page, signal that the UI list is available.
void MainWindow::onPageLoaded(bool ok)
{
    if (ok) {
        QString url = m_view->url().toString();
        m_discoveryProxy->m_home = (url.compare(rui_home) == 0);
    }
}
