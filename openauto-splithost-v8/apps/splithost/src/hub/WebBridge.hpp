#pragma once
#include <QObject>
#include <QJsonObject>
class WebBridge : public QObject {
    Q_OBJECT
public:
    explicit WebBridge(QObject* parent=nullptr) : QObject(parent) {}
    Q_INVOKABLE QString ping() const { return QStringLiteral("pong"); }
    Q_INVOKABLE QStringList listModules() const { return modules_; }
signals:
    void moduleUpdated(const QString& name, const QJsonObject& data);
    void moduleError(const QString& name, const QString& error);
public slots:
    void setModules(const QStringList& m) { modules_ = m; }
    void forwardModuleUpdate(const QString& name, const QJsonObject& data) { emit moduleUpdated(name, data); }
    void forwardModuleError(const QString& name, const QString& err) { emit moduleError(name, err); }
private:
    QStringList modules_;
};
