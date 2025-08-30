#pragma once
#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QMap>
#include <QSet>
#include <QDateTime>

class ModuleManager : public QObject {
    Q_OBJECT
public:
    explicit ModuleManager(const QString& modulesDir, QObject* parent=nullptr)
        : QObject(parent), modulesDir_(modulesDir) {
        scanTimer_.setInterval(3000);
        QObject::connect(&scanTimer_, &QTimer::timeout, this, &ModuleManager::tick);
        scanTimer_.start();
    }
    QStringList modules() const { return modules_.keys(); }
signals:
    void moduleUpdated(const QString& name, const QJsonObject& data);
    void moduleError(const QString& name, const QString& error);
private slots:
    void tick() {
        QDir d(modulesDir_);
        if (!d.exists()) return;
        auto files = d.entryInfoList(QDir::Files | QDir::Executable | QDir::NoDotAndDotDot);
        QSet<QString> seen;
        for (const QFileInfo& fi : files) {
            QString name = fi.completeBaseName();
            seen.insert(name);
            modules_[name] = fi.absoluteFilePath();
        }
        for (auto it = modules_.begin(); it != modules_.end(); ) {
            if (!seen.contains(it.key())) it = modules_.erase(it);
            else ++it;
        }
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (auto it = modules_.cbegin(); it != modules_.cend(); ++it) {
            const QString& name = it.key();
            qint64 &nextAt = nextRun_[name];
            if (nextAt == 0) nextAt = now;
            if (now >= nextAt) {
                auto* p = new QProcess(this);
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                env.insert("SPLITHOST", "1"); p->setProcessEnvironment(env);
                p->setProgram(it.value());
                QObject::connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                                 this, [this, p, name](int code, QProcess::ExitStatus st){
                    QByteArray out=p->readAllStandardOutput(), err=p->readAllStandardError(); p->deleteLater();
                    if (st!=QProcess::NormalExit || code!=0) { emit moduleError(name, QString::fromUtf8(err.isEmpty()? "non-zero exit":"") ); return; }
                    QJsonParseError jerr{}; QJsonDocument doc=QJsonDocument::fromJson(out,&jerr);
                    if (jerr.error!=QJsonParseError::NoError || !doc.isObject()) { emit moduleError(name, "invalid JSON output"); return; }
                    QJsonObject obj=doc.object(); QJsonObject data=obj.value("data").toObject(obj); emit moduleUpdated(name, data);
                });
                p->start();
                nextAt = now + 5000;
            }
        }
    }
private:
    QString modulesDir_;
    QTimer scanTimer_;
    QMap<QString, QString> modules_;
    QMap<QString, qint64> nextRun_;
};
