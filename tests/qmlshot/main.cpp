// SPDX-FileCopyrightText: 2026 plasma-deepin-theme contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// Renders a QML file (splash screen, SDDM theme) to a PNG for visual checks.
// Usage: qmlshot <file.qml> <out.png> [width height] [--stage N] [--sddm]
// --sddm provides minimal mocks of the objects SDDM injects into themes.
#include <QAbstractListModel>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>

class Sddm : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString hostName READ hostName CONSTANT)
    Q_PROPERTY(bool canPowerOff READ yes CONSTANT)
    Q_PROPERTY(bool canReboot READ yes CONSTANT)
    Q_PROPERTY(bool canSuspend READ yes CONSTANT)
    Q_PROPERTY(bool canHibernate READ no CONSTANT)
    Q_PROPERTY(bool canHybridSleep READ no CONSTANT)
public:
    QString hostName() const { return QStringLiteral("cachyos"); }
    bool yes() const { return true; }
    bool no() const { return false; }
    Q_INVOKABLE void login(const QString &, const QString &, int) {}
    Q_INVOKABLE void powerOff() {}
    Q_INVOKABLE void reboot() {}
    Q_INVOKABLE void suspend() {}
Q_SIGNALS:
    void loginFailed();
    void loginSucceeded();
};

class Model : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int lastIndex READ lastIndex CONSTANT)
    Q_PROPERTY(QString lastUser READ lastUser CONSTANT)
    Q_PROPERTY(int count READ count CONSTANT)
public:
    explicit Model(QList<QHash<int, QVariant>> rows, QHash<int, QByteArray> roles)
        : m_rows(rows), m_roles(roles) {}
    int rowCount(const QModelIndex &) const override { return m_rows.size(); }
    QVariant data(const QModelIndex &i, int role) const override { return m_rows.value(i.row()).value(role); }
    QHash<int, QByteArray> roleNames() const override { return m_roles; }
    int lastIndex() const { return 0; }
    QString lastUser() const { return QStringLiteral("gerald"); }
    int count() const { return m_rows.size(); }
private:
    QList<QHash<int, QVariant>> m_rows;
    QHash<int, QByteArray> m_roles;
};

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() < 3) {
        qWarning("usage: qmlshot file.qml out.png [w h] [--stage N] [--sddm]");
        return 1;
    }
    int w = 1280, h = 720;
    if (args.size() > 4 && args.at(3).toInt() > 0) {
        w = args.at(3).toInt();
        h = args.at(4).toInt();
    }
    QQuickView view;
    if (args.contains(QStringLiteral("--sddm"))) {
        auto ctx = view.rootContext();
        ctx->setContextProperty(QStringLiteral("sddm"), new Sddm);
        const int name = Qt::UserRole + 1, realName = Qt::UserRole + 2, icon = Qt::UserRole + 4;
        auto users = new Model({{{name, QStringLiteral("gerald")}, {realName, QStringLiteral("Gerald")}, {icon, QString()}}},
                               {{name, "name"}, {realName, "realName"}, {icon, "icon"}});
        const int file = Qt::UserRole + 2, sname = Qt::UserRole + 4;
        auto sessions = new Model({{{file, QStringLiteral("plasma.desktop")}, {sname, QStringLiteral("Plasma (Wayland)")}},
                                   {{file, QStringLiteral("plasmax11.desktop")}, {sname, QStringLiteral("Plasma (X11)")}}},
                                  {{file, "file"}, {sname, "name"}});
        ctx->setContextProperty(QStringLiteral("userModel"), users);
        ctx->setContextProperty(QStringLiteral("sessionModel"), sessions);
        QVariantMap screen{{QStringLiteral("geometry"), QRect(0, 0, w, h)}};
        ctx->setContextProperty(QStringLiteral("screenModel"), QVariant());
        ctx->setContextProperty(QStringLiteral("__sddm_test_geometry"), QRect(0, 0, w, h));
        QVariantMap config;
        ctx->setContextProperty(QStringLiteral("config"), config);
    }
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(w, h);
    view.setSource(QUrl::fromLocalFile(args.at(1)));
    if (view.status() != QQuickView::Ready) {
        for (const auto &e : view.errors()) {
            qWarning() << e.toString();
        }
        return 2;
    }
    const int stageIdx = args.indexOf(QStringLiteral("--stage"));
    if (stageIdx > 0) {
        view.rootObject()->setProperty("stage", args.at(stageIdx + 1).toInt());
    }
    view.show();
    QTimer::singleShot(2500, &view, [&view, &args] {
        view.grabWindow().save(args.at(2));
        qApp->quit();
    });
    return app.exec();
}
#include "main.moc"
