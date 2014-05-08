/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FullscreenController.h"

#include "WebFrame.h"
#include "WebViewClient.h"
#include "WebViewImpl.h"
#include "core/dom/Document.h"
#include "core/dom/FullscreenElementStack.h"
#include "core/html/HTMLMediaElement.h"
#include "core/page/Frame.h"

using namespace WebCore;

namespace WebKit {

PassOwnPtr<FullscreenController> FullscreenController::create(WebViewImpl* webViewImpl)
{
    return adoptPtr(new FullscreenController(webViewImpl));
}

FullscreenController::FullscreenController(WebViewImpl* webViewImpl)
    : m_webViewImpl(webViewImpl)
    , m_exitFullscreenPageScaleFactor(0)
    , m_isCancelingFullScreen(false)
{
}

void FullscreenController::willEnterFullScreen()
{
    if (!m_provisionalFullScreenElement)
        return;

    // Ensure that this element's document is still attached.
    Document& doc = m_provisionalFullScreenElement->document();
    if (doc.frame()) {
        FullscreenElementStack::from(&doc)->webkitWillEnterFullScreenForElement(m_provisionalFullScreenElement.get());
        m_fullScreenFrame = doc.frame();
    }
    m_provisionalFullScreenElement.clear();
}

void FullscreenController::didEnterFullScreen()
{
    if (!m_fullScreenFrame)
        return;

    if (Document* doc = m_fullScreenFrame->document()) {
        if (FullscreenElementStack::isFullScreen(doc)) {
            if (!m_exitFullscreenPageScaleFactor) {
                m_exitFullscreenPageScaleFactor = m_webViewImpl->pageScaleFactor();
                m_exitFullscreenScrollOffset = m_webViewImpl->mainFrame()->scrollOffset();
                m_webViewImpl->setPageScaleFactorPreservingScrollOffset(1.0f);
            }

            FullscreenElementStack::from(doc)->webkitDidEnterFullScreenForElement(0);
        }
    }
}

void FullscreenController::willExitFullScreen()
{
    if (!m_fullScreenFrame)
        return;

    if (Document* doc = m_fullScreenFrame->document()) {
        FullscreenElementStack* fullscreen = FullscreenElementStack::fromIfExists(doc);
        if (!fullscreen)
            return;
        if (fullscreen->isFullScreen(doc)) {
            // When the client exits from full screen we have to call webkitCancelFullScreen to
            // notify the document. While doing that, suppress notifications back to the client.
            m_isCancelingFullScreen = true;
            fullscreen->webkitCancelFullScreen();
            m_isCancelingFullScreen = false;
            fullscreen->webkitWillExitFullScreenForElement(0);
        }
    }
}

void FullscreenController::didExitFullScreen()
{
    if (!m_fullScreenFrame)
        return;

    if (Document* doc = m_fullScreenFrame->document()) {
        if (FullscreenElementStack* fullscreen = FullscreenElementStack::fromIfExists(doc)) {
            if (fullscreen->webkitIsFullScreen()) {
                if (m_exitFullscreenPageScaleFactor) {
                    m_webViewImpl->setPageScaleFactor(m_exitFullscreenPageScaleFactor,
                        WebPoint(m_exitFullscreenScrollOffset.width(), m_exitFullscreenScrollOffset.height()));
                    m_exitFullscreenPageScaleFactor = 0;
                    m_exitFullscreenScrollOffset = IntSize();
                }

                fullscreen->webkitDidExitFullScreenForElement(0);
            }
        }
    }

    m_fullScreenFrame.clear();
}

void FullscreenController::enterFullScreenForElement(WebCore::Element* element)
{
    // We are already transitioning to fullscreen for a different element.
    if (m_provisionalFullScreenElement) {
        m_provisionalFullScreenElement = element;
        return;
    }

    // We are already in fullscreen mode.
    if (m_fullScreenFrame) {
        m_provisionalFullScreenElement = element;
        willEnterFullScreen();
        didEnterFullScreen();
        return;
    }

#if USE(NATIVE_FULLSCREEN_VIDEO)
    if (element && element->isMediaElement()) {
        HTMLMediaElement* mediaElement = toHTMLMediaElement(element);
        if (mediaElement->player() && mediaElement->player()->canEnterFullscreen()) {
            mediaElement->player()->enterFullscreen();
            m_provisionalFullScreenElement = element;
        }
        return;
    }
#endif

    // We need to transition to fullscreen mode.
    if (WebViewClient* client = m_webViewImpl->client()) {
        if (client->enterFullScreen())
            m_provisionalFullScreenElement = element;
    }
}

void FullscreenController::exitFullScreenForElement(WebCore::Element* element)
{
    // The client is exiting full screen, so don't send a notification.
    if (m_isCancelingFullScreen)
        return;
#if USE(NATIVE_FULLSCREEN_VIDEO)
    if (element && element->isMediaElement()) {
        HTMLMediaElement* mediaElement = toHTMLMediaElement(element);
        if (mediaElement->player())
            mediaElement->player()->exitFullscreen();
        return;
    }
#endif
    if (WebViewClient* client = m_webViewImpl->client())
        client->exitFullScreen();
}

}

