// SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// Small widget gallery used to check the Deepin Glass style.
// Usage: gallery [--grab out.png] [-style DeepinGlass]
#include <QApplication>
#include <QPainter>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QToolBar>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--dark"))) {
        // DTK dark palette (dguiapplicationhelper.cpp)
        QPalette p;
        p.setColor(QPalette::Window, QColor(0x25, 0x25, 0x25));
        p.setColor(QPalette::WindowText, QColor(0xde, 0xde, 0xde));
        p.setColor(QPalette::Base, QColor(0x28, 0x28, 0x28));
        p.setColor(QPalette::Text, QColor(0xde, 0xde, 0xde));
        p.setColor(QPalette::Button, QColor(0x44, 0x44, 0x44));
        p.setColor(QPalette::ButtonText, QColor(0xde, 0xde, 0xde));
        p.setColor(QPalette::Highlight, QColor(0x00, 0x59, 0xd2));
        p.setColor(QPalette::HighlightedText, QColor(0xf1, 0xf6, 0xff));
        app.setPalette(p);
    } else if (app.arguments().contains(QStringLiteral("--light"))) {
        QPalette p;
        p.setColor(QPalette::Window, QColor(0xf8, 0xf8, 0xf8));
        p.setColor(QPalette::WindowText, QColor(0x25, 0x25, 0x25));
        p.setColor(QPalette::Base, Qt::white);
        p.setColor(QPalette::Text, QColor(0x25, 0x25, 0x25));
        p.setColor(QPalette::Button, QColor(0xe5, 0xe5, 0xe5));
        p.setColor(QPalette::ButtonText, QColor(0x25, 0x25, 0x25));
        p.setColor(QPalette::Highlight, QColor(0x00, 0x81, 0xff));
        p.setColor(QPalette::HighlightedText, Qt::white);
        app.setPalette(p);
    }
    if (app.arguments().contains(QStringLiteral("--backdrop"))) {
        // colourful "wallpaper" to see the blur working
        class Backdrop : public QWidget
        {
        protected:
            void paintEvent(QPaintEvent *) override
            {
                QPainter p(this);
                QLinearGradient g(0, 0, width(), height());
                g.setColorAt(0, QColor(0x1d, 0x4e, 0x89));
                g.setColorAt(0.5, QColor(0x2a, 0x9d, 0x8f));
                g.setColorAt(1, QColor(0xe9, 0xc4, 0x6a));
                p.fillRect(rect(), g);
                p.setRenderHint(QPainter::Antialiasing);
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0xe7, 0x6f, 0x51));
                p.drawEllipse(QPoint(width() / 3, height() / 2), 160, 160);
                p.setBrush(QColor(0xf4, 0xa2, 0x61));
                p.drawEllipse(QPoint(2 * width() / 3, height() / 3), 120, 120);
                p.setPen(Qt::white);
                QFont f = font();
                f.setPointSize(48);
                f.setBold(true);
                p.setFont(f);
                p.drawText(rect().adjusted(0, 0, 0, -40), Qt::AlignHCenter | Qt::AlignBottom, QStringLiteral("Deepin Glass"));
            }
        };
        auto b = new Backdrop;
        b->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
        b->showFullScreen();
        return app.exec();
    }
    QMainWindow w;
    w.setWindowTitle(QStringLiteral("Deepin Glass gallery"));
    auto file = w.menuBar()->addMenu(QStringLiteral("&File"));
    file->addAction(QStringLiteral("Open"));
    file->addAction(QStringLiteral("Save"));
    auto tb = w.addToolBar(QStringLiteral("Main"));
    tb->addAction(QStringLiteral("Back"));
    tb->addAction(QStringLiteral("Forward"));

    auto central = new QWidget;
    auto grid = new QGridLayout(central);
    auto list = new QListWidget;
    list->addItems({QStringLiteral("Home"), QStringLiteral("Documents"), QStringLiteral("Pictures"), QStringLiteral("Music")});
    for (int i = 0; i < 20; ++i) {
        list->addItem(QStringLiteral("Folder %1").arg(i));
    }
    list->setCurrentRow(1);
    list->setMaximumWidth(140);
    grid->addWidget(list, 0, 0, 6, 1);

    auto def = new QPushButton(QStringLiteral("Default"));
    def->setDefault(true);
    grid->addWidget(def, 0, 1);
    grid->addWidget(new QPushButton(QStringLiteral("Button")), 0, 2);
    auto dis = new QPushButton(QStringLiteral("Disabled"));
    dis->setEnabled(false);
    grid->addWidget(dis, 0, 3);

    auto le = new QLineEdit;
    le->setPlaceholderText(QStringLiteral("Search"));
    grid->addWidget(le, 1, 1, 1, 2);
    auto spin = new QSpinBox;
    spin->setValue(42);
    grid->addWidget(spin, 1, 3);

    auto combo = new QComboBox;
    combo->addItems({QStringLiteral("Deepin Light"), QStringLiteral("Deepin Dark")});
    grid->addWidget(combo, 2, 1);
    auto ecombo = new QComboBox;
    ecombo->setEditable(true);
    ecombo->addItem(QStringLiteral("editable"));
    grid->addWidget(ecombo, 2, 2, 1, 2);

    auto c1 = new QCheckBox(QStringLiteral("Checked"));
    c1->setChecked(true);
    grid->addWidget(c1, 3, 1);
    grid->addWidget(new QCheckBox(QStringLiteral("Unchecked")), 3, 2);
    auto c3 = new QCheckBox(QStringLiteral("Partial"));
    c3->setTristate(true);
    c3->setCheckState(Qt::PartiallyChecked);
    grid->addWidget(c3, 3, 3);

    auto r1 = new QRadioButton(QStringLiteral("Radio on"));
    r1->setChecked(true);
    grid->addWidget(r1, 4, 1);
    grid->addWidget(new QRadioButton(QStringLiteral("Radio off")), 4, 2);
    auto slider = new QSlider(Qt::Horizontal);
    slider->setValue(60);
    grid->addWidget(slider, 4, 3);

    auto pb = new QProgressBar;
    pb->setValue(65);
    grid->addWidget(pb, 5, 1, 1, 3);
    grid->addWidget(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel), 6, 1, 1, 3);
    w.setCentralWidget(central);
    w.resize(640, 360);

    const QStringList args = app.arguments();
    const int grabIndex = args.indexOf(QStringLiteral("--grab"));
    w.show();
    if (grabIndex > 0 && grabIndex + 1 < args.size()) {
        const QString out = args.at(grabIndex + 1);
        QTimer::singleShot(300, &w, [&w, out] {
            w.grab().save(out);
            qApp->quit();
        });
    }
    return app.exec();
}
