/*
 * SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Small helper that renders a soft drop shadow of a rounded rectangle.
 * Three passes of a box blur approximate a gaussian blur.
 */
#pragma once

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <vector>

namespace DeepinGlass
{

inline void boxBlurAlpha(QImage &img, int radius)
{
    if (radius < 1) {
        return;
    }
    const int w = img.width();
    const int h = img.height();
    std::vector<int> line(std::max(w, h));
    std::vector<int> out(std::max(w, h));
    auto pass = [&](int len) {
        int sum = 0;
        const int win = 2 * radius + 1;
        for (int i = -radius; i <= radius; ++i) {
            sum += line[std::clamp(i, 0, len - 1)];
        }
        for (int i = 0; i < len; ++i) {
            out[i] = sum / win;
            const int add = std::clamp(i + radius + 1, 0, len - 1);
            const int rem = std::clamp(i - radius, 0, len - 1);
            sum += line[add] - line[rem];
        }
    };
    // image is Format_ARGB32_Premultiplied, black shadow: only the alpha channel matters
    for (int y = 0; y < h; ++y) {
        QRgb *row = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            line[x] = qAlpha(row[x]);
        }
        pass(w);
        for (int x = 0; x < w; ++x) {
            row[x] = qRgba(0, 0, 0, out[x]);
        }
    }
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            line[y] = qAlpha(reinterpret_cast<const QRgb *>(img.constScanLine(y))[x]);
        }
        pass(h);
        for (int y = 0; y < h; ++y) {
            reinterpret_cast<QRgb *>(img.scanLine(y))[x] = qRgba(0, 0, 0, out[y]);
        }
    }
}

/**
 * Renders the shadow of a rounded box. The returned image has the box at
 * (padding, padding) with size @p box; the box area itself is cut out.
 */
inline QImage renderShadow(const QSize &box, qreal radius, int blur, const QPoint &offset, qreal strength, QColor color = Qt::black)
{
    const int pad = blur * 2;
    QImage img(box.width() + 2 * pad, box.height() + 2 * pad, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    {
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        color.setAlphaF(std::clamp(strength, 0.0, 1.0));
        p.setBrush(color);
        p.drawRoundedRect(QRectF(pad + offset.x(), pad + offset.y(), box.width(), box.height()), radius, radius);
    }
    const int r = std::max(1, blur / 3);
    boxBlurAlpha(img, r);
    boxBlurAlpha(img, r);
    boxBlurAlpha(img, r);
    {
        // cut out the window area so translucent windows are not darkened
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::black);
        p.drawRoundedRect(QRectF(pad, pad, box.width(), box.height()), radius, radius);
    }
    return img;
}

} // namespace DeepinGlass
