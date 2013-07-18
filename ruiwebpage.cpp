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
#include "ruiwebpage.h"
//#include <QWebPage>
#include "qwebpage.h"
#include <stdio.h>
#include "discoveryproxy.h"
#include "browsersettings.h"

RUIWebPage::RUIWebPage(QObject* parent)
    : QWebPage(parent)
{
}

QString RUIWebPage::userAgentForUrl(const QUrl& url) const
{
    QString userAgent = QWebPage::userAgentForUrl(url);

    QString scheme = url.scheme();
    QString host = url.host();

    DiscoveryProxy* proxy = DiscoveryProxy::Instance();
    BrowserSettings* settings = BrowserSettings::Instance();

    // Always add the product token, but only add the CertID if this is a RUI Transport Server
    // AND the protocol is https.

    userAgent += " DLNADOC/1.50 DLNA-HTML5/1.0";

    if ( scheme.compare("https") == 0) {

        if (proxy->isHostRUITransportServer(host)) {

            userAgent += " (CertID " + settings->certID + ")";
        }
    }

    /*
    fprintf(stderr,"url: %s **  scheme: %s**  host: %s userAgent: %s\n"
            , url.toString().toAscii().data()
            , scheme.toAscii().data()
            , host.toAscii().data()
            , userAgent.toAscii().data()
            );
    */

    return userAgent;
}
