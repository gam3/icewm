/*
 *  IceWM - Window tray
 *  Copyright (C) 2001 The Authors of IceWM
 *
 *  Release under terms of the GNU Library General Public License
 *
 *  2001/05/15: Jan Krupa <j.krupa@netweb.cz>
 *  - initial version
 *  2001/05/27: Mathias Hasselmann <mathias.hasselmann@gmx.net>
 *  - ported to IceWM 1.0.9 (gradients, ...)
 *
 *  TODO: share code with atask.cc
 */

#include "config.h"

#include "atray.h"
#include "wmtaskbar.h"
#include "yprefs.h"
#include "prefs.h"
#include "wmframe.h"
#include "wmwinlist.h"
#include "wpixmaps.h"
#include "yrect.h"

static YColorName normalTrayAppFg(&clrNormalTaskBarAppText);
static YColorName normalTrayAppBg(&clrNormalTaskBarApp);
static YColorName activeTrayAppFg(&clrActiveTaskBarAppText);
static YColorName activeTrayAppBg(&clrActiveTaskBarApp);
static YColorName minimizedTrayAppFg(&clrMinimizedTaskBarAppText);
static YColorName minimizedTrayAppBg(&clrNormalTaskBarApp);
static YColorName invisibleTrayAppFg(&clrInvisibleTaskBarAppText);
static YColorName invisibleTrayAppBg(&clrNormalTaskBarApp);
static ref<YFont> normalTrayFont;
static ref<YFont> activeTrayFont;

ref<YImage> TrayApp::taskMinimizedGradient;
ref<YImage> TrayApp::taskActiveGradient;
ref<YImage> TrayApp::taskNormalGradient;

TrayApp::TrayApp(ClientData *frame, YWindow *aParent): YWindow(aParent) {
    if (normalTrayFont == null)
        normalTrayFont = YFont::getFont(XFA(normalTaskBarFontName));
    if (activeTrayFont == null)
        activeTrayFont = YFont::getFont(XFA(activeTaskBarFontName));

    fFrame = frame;
    selected = 0;
    fShown = true;
    setToolTip(frame->getTitle());
    setTitle(cstring(frame->getTitle()));
    //setDND(true);
}

TrayApp::~TrayApp() {
}

bool TrayApp::isFocusTraversable() {
    return true;
}

void TrayApp::setShown(bool ashow) {
    if (ashow != fShown) {
        fShown = ashow;
    }
}

void TrayApp::paint(Graphics &g, const YRect &/*r*/) {
    YColor bg;
    ref<YPixmap> bgPix;
    ref<YImage> bgGrad;

    int sx(parent() ? x() + parent()->x() : x());
    int sy(parent() ? y() + parent()->y() : y());

    unsigned sw((parent() && parent()->parent() ?
                 parent()->parent() : this)->width());
    unsigned sh((parent() && parent()->parent() ?
                 parent()->parent() : this)->height());

    if (!getFrame()->visibleNow()) {
        bg = invisibleTrayAppBg;
        bgPix = taskbackPixmap;
        bgGrad = getGradient();
    } else if (getFrame()->isMinimized()) {
        bg = minimizedTrayAppBg;
        bgPix = taskbuttonminimizedPixmap;
        if (taskMinimizedGradient == null && taskbuttonminimizedPixbuf != null)
            taskMinimizedGradient = taskbuttonminimizedPixbuf->scale(sw, sh);
        bgGrad = taskMinimizedGradient;
    } else if (getFrame()->focused()) {
        bg = activeTrayAppBg;
        bgPix = taskbuttonactivePixmap;
        if (taskActiveGradient == null && taskbuttonactivePixbuf != null)
            taskActiveGradient = taskbuttonactivePixbuf->scale(sw, sh);
        bgGrad = taskActiveGradient;
    } else {
        bg = normalTrayAppBg;
        bgPix = taskbuttonPixmap;
        if (taskNormalGradient == null && taskbuttonPixbuf != null)
            taskNormalGradient = taskbuttonPixbuf->scale(sw, sh);
        bgGrad = taskNormalGradient;
    }

    if (selected == 3) {
        g.setColor(YColor::black);
        g.drawRect(0, 0, width() - 1, height() - 1);
        g.setColor(bg);
        g.fillRect(1, 1, width() - 2, height() - 2);
    } else {
        if (width() > 0 && height() > 0) {
            if (bgGrad != null)
                g.drawImage(bgGrad, sx, sy, width(), height(), 0, 0);
            else
            if (bgPix != null)
                g.fillPixmap(bgPix, 0, 0, width(), height(), 0, 0);
            else {
                g.setColor(bg);
                g.fillRect(0, 0, width(), height());
            }
        }
    }
    ref<YIcon> icon(getFrame()->getIcon());

    if (icon != null) {
        if (g.color() == 0)
            g.setColor(bg);
        icon->draw(g, 2, 2, YIcon::smallSize());
    }
}

void TrayApp::handleButton(const XButtonEvent &button) {
    YWindow::handleButton(button);
    if (button.button == 1 || button.button == 2) {
        if (button.type == ButtonPress) {
            selected = 2;
            repaint();
        } else if (button.type == ButtonRelease) {
            if (selected == 2) {
                if (button.button == 1) {
                    if (getFrame()->visibleNow() &&
                        (!getFrame()->canRaise() || (button.state & ControlMask)))
                        getFrame()->wmMinimize();
                    else {
                        if (button.state & ShiftMask)
                            getFrame()->wmOccupyOnlyWorkspace(manager->activeWorkspace());
                        getFrame()->activateWindow(true);
                    }
                } else if (button.button == 2) {
                    if (getFrame()->visibleNow() &&
                        (!getFrame()->canRaise() || (button.state & ControlMask)))
                        getFrame()->wmLower();
                    else {
                        if (button.state & ShiftMask)
                            getFrame()->wmOccupyWorkspace(manager->activeWorkspace());
                        getFrame()->activateWindow(true);
                    }
                }
            }
            selected = 0;
            repaint();
        }
    }
}

void TrayApp::handleCrossing(const XCrossingEvent &crossing) {
    if (selected > 0) {
        if (crossing.type == EnterNotify) {
            selected = 2;
            repaint();
        } else if (crossing.type == LeaveNotify) {
            selected = 1;
            repaint();
        }
    }
    YWindow::handleCrossing(crossing);
}

void TrayApp::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
                                    YPopupWindow::pfCanFlipVertical |
                                    YPopupWindow::pfCanFlipHorizontal |
                                    YPopupWindow::pfPopupMenu);
    }
}

void TrayApp::handleDNDEnter() {
    fRaiseTimer->setTimer(autoRaiseDelay, this, true);

    selected = 3;
    repaint();
}

void TrayApp::handleDNDLeave() {
    if (fRaiseTimer)
        fRaiseTimer = null;

    selected = 0;
    repaint();
}

bool TrayApp::handleTimer(YTimer *t) {
    if (t == fRaiseTimer) {
        fRaiseTimer = null;
        getFrame()->wmRaise();
    }
    return false;
}

TrayPane::TrayPane(IAppletContainer *taskBar, YWindow *parent): YWindow(parent) {
    fTaskBar = taskBar;
    fNeedRelayout = true;
}

TrayPane::~TrayPane() {
}

TrayApp *TrayPane::addApp(YFrameWindow *frame) {
    TrayApp *tapp = new TrayApp(frame, this);

    if (tapp != 0) {
        fApps.append(tapp);
        tapp->show();

        if (!(frame->visibleOn(manager->activeWorkspace()) ||
              trayShowAllWindows))
            tapp->setShown(0);

        relayout();
    }
    return tapp;
}

void TrayPane::removeApp(YFrameWindow *frame) {
    for (IterType icon = fApps.iterator(); ++icon; ) {
        if (icon->getFrame() == frame) {
            icon->hide();
            icon.remove();

            relayout();
            return;
        }
    }
}

int TrayPane::getRequiredWidth() {
    int tc = 0;

    for (IterType a = fApps.iterator(); ++a; )
        if (a->getShown()) tc++;

    return (tc ? 4 + tc * (height() - 4) : 1);
}

void TrayPane::relayoutNow() {
    if (!fNeedRelayout)
        return ;

    fNeedRelayout = false;

    unsigned nw = getRequiredWidth();
    if (nw != width()) {
        MSG(("tray: nw=%d x=%d w=%d", nw, x(), width()));
        setGeometry(YRect(x() + width() - nw, y(), nw, height()));
        fTaskBar->relayout();
    }

    int x, y;
    unsigned w, h;
    int tc = 0;

    for (IterType a = fApps.iterator(); ++a; )
        if (a->getShown()) tc++;

    w = h = height() - 4;
    x = int(width()) - 2 - tc * w;
    y = 2;

    for (IterType f = fApps.iterator(); ++f; ) {
        if (f->getShown()) {
            f->setGeometry(YRect(x, y, w, h));
            f->show();
            x += w;
        } else
            f->hide();
    }
}

void TrayPane::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
        fTaskBar->contextMenu(up.x_root, up.y_root);
    }
}

void TrayPane::paint(Graphics &g, const YRect &/*r*/) {
    int const w(width());
    int const h(height());

    g.setColor(taskBarBg);

    ref<YImage> gradient(parent() ? parent()->getGradient() : null);

    if (gradient != null)
        g.drawImage(gradient, x(), y(), w, h, 0, 0);
    else
    if (taskbackPixmap != null)
        g.fillPixmap(taskbackPixmap, 0, 0, w, h, x(), y());
    else
        g.fillRect(0, 0, w, h);

    if (trayDrawBevel && w > 1) {
        if (wmLook == lookMetal)
            g.draw3DRect(1, 1, w - 2, h - 2, false);
        else
            g.draw3DRect(0, 0, w - 1, h - 1, false);
    }
}

// vim: set sw=4 ts=4 et:
