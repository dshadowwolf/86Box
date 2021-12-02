#include <cstdint>

#include <QDebug>
#include <QThread>

#include <QStatusBar>

#include "qt_mainwindow.hpp"

std::atomic_int resize_pending = 0;
std::atomic_int resize_w = 0;
std::atomic_int resize_h = 0;

MainWindow* main_window = nullptr;

extern "C" {

#include <86box/plat.h>

void
plat_delay_ms(uint32_t count)
{
    QThread::msleep(count);
}

wchar_t* ui_window_title(wchar_t* str)
{
    if (str == nullptr) {
        static wchar_t title[512];
        memset(title, 0, sizeof(title));
        main_window->getTitle(title);
        str = title;
    } else {
        main_window->setTitle(str);
    }
    return str;
}

extern "C" void qt_blit(int x, int y, int w, int h)
{
    main_window->blitToWidget(x, y, w, h);
}

void mouse_poll() {
    main_window->pollMouse();
}

void plat_resize(int w, int h) {
    resize_w = w;
    resize_h = h;
    resize_pending = 1;

    main_window->resizeContents(w, h);
}

void plat_setfullscreen(int on) {
    main_window->setFullscreen(on > 0 ? true : false);
}

void plat_mouse_capture(int on) {
    main_window->setMouseCapture(on > 0 ? true : false);
}

int	ui_msgbox_header(int flags, void *header, void* message) {
    if (header <= (void*)7168) header = plat_get_string((uintptr_t)header);
    if (message <= (void*)7168) message = plat_get_string((uintptr_t)message);

    auto hdr = QString::fromWCharArray(reinterpret_cast<const wchar_t*>(header));
    auto msg = QString::fromWCharArray(reinterpret_cast<const wchar_t*>(message));

    main_window->showMessage(hdr, msg);
    return 0;
}

int	ui_msgbox(int flags, void *message) {
    return ui_msgbox_header(flags, nullptr, message);
}

void ui_sb_set_text_w(wchar_t *wstr) {
    main_window->statusBar()->showMessage(QString::fromWCharArray(wstr));
}

void
ui_sb_update_tip(int arg) {
    qDebug() << Q_FUNC_INFO << arg;
}

void
ui_sb_update_panes() {
    main_window->updateStatusBarPanes();
}

void ui_sb_bugui(char *str) {
    main_window->statusBar()->showMessage(str);
}

void ui_sb_set_ready(int ready) {
    qDebug() << Q_FUNC_INFO << ready;
}

void
ui_sb_update_icon_state(int tag, int state) {
    if (main_window == nullptr) {
        return;
    }
    main_window->updateStatusBarEmpty(tag, state > 0 ? true : false);
}

void
ui_sb_update_icon(int tag, int active) {
    main_window->updateStatusBarActivity(tag, active > 0 ? true : false);
}

}
