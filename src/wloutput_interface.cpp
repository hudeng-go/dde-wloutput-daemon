#include "wloutput_interface.h"
#include <QDebug>
#include <QtDBus/QDBusMessage>
#include <outputdevice.h>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

static QMap<QString, OutputDevice*> uuid2OutputDevice;

wloutput_interface::wloutput_interface(QObject *parent) : QObject (parent)
{
      m_pConnectThread = new ConnectionThread;
      m_pThread = new QThread;
      m_bConnected = false;
      m_pManager = nullptr;
}

wloutput_interface::~wloutput_interface()
{
    m_bConnected = false;
}

bool wloutput_interface::InitDBus()
{

    QDBusConnection connection = QDBusConnection::sessionBus();
    if(!connection.registerService(SERVER))
    {
        qDebug() << "error:" << connection.lastError().message();
        return false;
    }

    if(!connection.registerObject(PATH, this,
                                   QDBusConnection :: ExportAllSlots | QDBusConnection :: ExportAllSignals))
    {
        qDebug() << "error:" << connection.lastError().message();
        return false;
    }
    return true;

}



OutputInfo wloutput_interface::GetOutputInfo(const OutputDevice* dev)
{
    OutputInfo stOutputInfo;
    stOutputInfo.model = dev->model();
    stOutputInfo.manufacturer = dev->manufacturer();
    stOutputInfo.uuid = dev->uuid();

    switch(dev->enabled())
    {
        case OutputDevice::Enablement::Disabled:
            stOutputInfo.enabled = 0;
            break;
        case OutputDevice::Enablement::Enabled:
            stOutputInfo.enabled = 1;
            break;

    }

    stOutputInfo.x = dev->globalPosition().x();
    stOutputInfo.y = dev->globalPosition().y();
    stOutputInfo.width = dev->pixelSize().width();
    stOutputInfo.height = dev->pixelSize().height();
    stOutputInfo.refresh_rate = dev->refreshRate();

    switch (dev->transform())
    {
        case OutputDevice::Transform::Normal:
            stOutputInfo.transform = 0;
            break;
        case OutputDevice::Transform::Rotated90:
            stOutputInfo.transform = 1;
            break;
        case OutputDevice::Transform::Rotated180:
            stOutputInfo.transform = 2;
            break;
        case OutputDevice::Transform::Rotated270:
            stOutputInfo.transform = 3;
            break;
        case OutputDevice::Transform::Flipped:
            stOutputInfo.transform = 4;
            break;
        case OutputDevice::Transform::Flipped90:
            stOutputInfo.transform = 5;
            break;
        case OutputDevice::Transform::Flipped180:
            stOutputInfo.transform = 6;
            break;
        case OutputDevice::Transform::Flipped270:
            stOutputInfo.transform = 7;
            break;
    }
    stOutputInfo.phys_width = dev->physicalSize().width();
    stOutputInfo.phys_height = dev->physicalSize().height();
    stOutputInfo.scale = dev->scale();
    auto modes = dev->modes();
    for (auto oIter = modes.begin(); oIter != modes.end(); ++oIter)
    {
        ModeInfo stModeInfo;
        stModeInfo.id = oIter->id;
        stModeInfo.width = oIter->size.width();
        stModeInfo.height = oIter->size.height();
        stModeInfo.refresh_rate = oIter->refreshRate;
        stModeInfo.flags = oIter->flags;
        stOutputInfo.lstModeInfos.push_back(stModeInfo);
    }
    return stOutputInfo;
}




QString wloutput_interface::OutputInfo2Json(QList<OutputInfo>& listOutputInfos)
{
    QString json;
    QJsonObject root;

    QJsonArray jsonOutputArray;
    auto oIterOutputInfo = listOutputInfos.begin();
    for (int i = 0; oIterOutputInfo != listOutputInfos.end(); ++oIterOutputInfo, ++i)
    {
        QJsonObject jsonOutputInfo;

        jsonOutputInfo.insert("model", oIterOutputInfo->model);
        jsonOutputInfo.insert("manufacturer", oIterOutputInfo->manufacturer);
        jsonOutputInfo.insert("uuid", oIterOutputInfo->uuid);
        jsonOutputInfo.insert("enabled", oIterOutputInfo->enabled);
        jsonOutputInfo.insert("x", oIterOutputInfo->x);
        jsonOutputInfo.insert("y", oIterOutputInfo->y);
        jsonOutputInfo.insert("width", oIterOutputInfo->width);
        jsonOutputInfo.insert("height", oIterOutputInfo->height);
        jsonOutputInfo.insert("refresh_rate", oIterOutputInfo->refresh_rate);
        jsonOutputInfo.insert("transform", oIterOutputInfo->transform);
        jsonOutputInfo.insert("phys_width", oIterOutputInfo->phys_width);
        jsonOutputInfo.insert("phys_height", oIterOutputInfo->phys_height);
        jsonOutputInfo.insert("scale", oIterOutputInfo->scale);



        QJsonArray jsonModeInfoArray;
        auto oIterModeInfo = oIterOutputInfo->lstModeInfos.begin();
        for (int j = 0; oIterModeInfo != oIterOutputInfo->lstModeInfos.end(); ++oIterModeInfo, ++j) {

            QJsonObject jsonModeInfo;
            jsonModeInfo.insert("id", oIterModeInfo->id);
            jsonModeInfo.insert("width", oIterModeInfo->width);
            jsonModeInfo.insert("height", oIterModeInfo->height);
            jsonModeInfo.insert("refresh_rate", oIterModeInfo->refresh_rate);
            jsonModeInfo.insert("flags", oIterModeInfo->flags);
            jsonModeInfoArray.insert(j, jsonModeInfo);
        }

        jsonOutputInfo.insert("ModeInfo", jsonModeInfoArray);

        jsonOutputArray.insert(i, jsonOutputInfo);
    }
    root.insert("OutputInfo", jsonOutputArray);

    QJsonDocument rootDoc;
    rootDoc.setObject(root);
    //json = rootDoc.toJson(QJsonDocument::Compact);
    json = rootDoc.toJson(QJsonDocument::Indented);
    return  json;
}

QList<OutputInfo> wloutput_interface::json2OutputInfo(QString jsonString)
{
    QList<OutputInfo> listOutputInfo;
    QJsonParseError err;
    QJsonDocument  rootDoc = QJsonDocument::fromJson(jsonString.toUtf8(), &err);
    if(err.error != QJsonParseError::NoError) {
        qDebug() << "json fromat error";
        return listOutputInfo;
    }
    else {

        QJsonObject root = rootDoc.object();
        QJsonValue outputInfoValue = root.value("OutputInfo");
        if (outputInfoValue.isArray()) {
            QJsonArray outputInfoArray = outputInfoValue.toArray();
            for (int i = 0; i < outputInfoArray.size(); ++i) {
                OutputInfo stOutputInfo;
                QJsonObject jsonOutputInfo = outputInfoArray.at(i).toObject();
                stOutputInfo.model = jsonOutputInfo.value("model").toString();
                stOutputInfo.manufacturer = jsonOutputInfo.value("manufacturer").toString();
                stOutputInfo.uuid = jsonOutputInfo.value("uuid").toString();
                stOutputInfo.enabled = jsonOutputInfo.value("enabled").toInt();
                stOutputInfo.x = jsonOutputInfo.value("x").toInt();
                stOutputInfo.y = jsonOutputInfo.value("y").toInt();
                stOutputInfo.width = jsonOutputInfo.value("width").toInt();
                stOutputInfo.height = jsonOutputInfo.value("height").toInt();
                stOutputInfo.refresh_rate = jsonOutputInfo.value("refresh_rate").toInt();
                stOutputInfo.transform = jsonOutputInfo.value("transform").toInt();
                stOutputInfo.phys_width = jsonOutputInfo.value("phys_width").toInt();
                stOutputInfo.phys_height = jsonOutputInfo.value("phys_height").toInt();
                stOutputInfo.scale = jsonOutputInfo.value("scale").toDouble();

                QJsonValue modeInfoValue = jsonOutputInfo.value("ModeInfo");
                if (modeInfoValue.isArray()) {
                    QJsonArray jsonModeInfoArray = modeInfoValue.toArray();
                    for (int j = 0; j < jsonModeInfoArray.size(); ++j) {
                        ModeInfo stModeInfo;
                        QJsonObject jsonModeInfo = jsonModeInfoArray.at(i).toObject();
                        stModeInfo.id = jsonModeInfo.value("id").toInt();
                        stModeInfo.width = jsonModeInfo.value("width").toInt();
                        stModeInfo.height = jsonModeInfo.value("height").toInt();
                        stModeInfo.refresh_rate = jsonModeInfo.value("refresh_rate").toInt();
                        stModeInfo.flags = jsonModeInfo.value("flags").toInt();
                        stOutputInfo.lstModeInfos.push_back(stModeInfo);
                    }

                }

                listOutputInfo.push_back(stOutputInfo);

            }
        }

    }
    return listOutputInfo;
}


void wloutput_interface::StartWork()
{
    m_pConnectThread->moveToThread(m_pThread);
    m_pThread->start();
    m_pConnectThread->initConnection();


    QObject::connect(m_pConnectThread, &ConnectionThread::connected, [ & ] {

        m_bConnected = true;
        m_pRegisry = new Registry;
        m_pRegisry->create(m_pConnectThread);
        m_pRegisry->setup();


        QObject::connect(m_pRegisry, &Registry::outputManagementAnnounced, [ & ](quint32 name, quint32 version) {
            m_pManager = m_pRegisry->createOutputManagement(name, version);
            if (!m_pManager || !m_pManager->isValid()) {
                qDebug() << "create manager is nullptr or not valid!";
                return;
            }

            m_pConf = m_pManager->createConfiguration();
            if (!m_pManager || !m_pManager->isValid()) {
                qDebug() << "create output configure is null or is not vaild";
                return;
            }

            QObject::connect(m_pRegisry, &Registry::outputDeviceAnnounced, [ & ](quint32 name, quint32 version) {
                auto dev = m_pRegisry->createOutputDevice(name, version);
                if (!dev || !dev->isValid())
                {
                    qDebug() << "get dev is null or not valid!";
                    return;
                }

                QObject::connect(dev, &OutputDevice::changed, [=] {

                    QString uuid = dev->uuid();
                    if (uuid2OutputDevice.find(uuid) == uuid2OutputDevice.end()) {
                        uuid2OutputDevice.insert(uuid, dev);
                        qDebug() << "OutputDevice::Added uuid=" << uuid;
                        OutputInfo stOutputInfo = GetOutputInfo(dev);
                        QList<OutputInfo> listOutputInfos;
                        listOutputInfos.push_back(stOutputInfo);
                        QString json = OutputInfo2Json(listOutputInfos);
                        //qDebug() << json;
                        QDBusMessage message = QDBusMessage::createSignal(PATH, INTERFACE, "OutputAdded");
                        QList<QVariant> arg;
                        message.setArguments(arg);
                        QDBusConnection::sessionBus().send(message);
                    }
                    else {
                        qDebug() << "OutputDevice::changed";
                        OutputInfo stOutputInfo = GetOutputInfo(dev);
                        QList<OutputInfo> listOutputInfos;
                        listOutputInfos.push_back(stOutputInfo);
                        QString json = OutputInfo2Json(listOutputInfos);
                        //qDebug() << json;
                        QDBusMessage message = QDBusMessage::createSignal(PATH, INTERFACE, "OutputChanged");
                        QList<QVariant> arg;
                        message.setArguments(arg);
                        QDBusConnection::sessionBus().send(message);
                    }
                });

                QObject::connect(dev, &OutputDevice::removed, [dev]{
                   qDebug() << "OutputDevice::removed";
                   OutputInfo stOutputInfo = GetOutputInfo(dev);
                   QList<OutputInfo> listOutputInfos;
                   listOutputInfos.push_back(stOutputInfo);
                   QString json = OutputInfo2Json(listOutputInfos);
                   //qDebug() << json;
                   QDBusMessage message = QDBusMessage::createSignal(PATH, INTERFACE, "OutputRemoved");
                   QList<QVariant> arg;
                   message.setArguments(arg);
                   QDBusConnection::sessionBus().send(message);
                });

           });
            QObject::connect(m_pRegisry, &Registry::outputDeviceRemoved, [](quint32 name) {

                qDebug() << "output device removed with name: " << name;
//                OutputInfo stOutputInfo = GetOutputInfo(dev);
//                QList<OutputInfo> listOutputInfos;
//                listOutputInfos.push_back(stOutputInfo);
//                QString json = OutputInfo2Json(listOutputInfos);
//                //qDebug() << json;
//                QDBusMessage message = QDBusMessage::createSignal(PATH, INTERFACE, "OutputRemoved");
//                QList<QVariant> arg;
//                message.setArguments(arg);
//                QDBusConnection::sessionBus().send(message);


           });


        });

        do
        {
            m_pConnectThread->roundtrip();

        }while (m_bConnected);

        if (m_pConnectThread) {
                m_pConnectThread->deleteLater();
                m_pThread->quit();
                m_pThread->wait();
        }

        if (m_pRegisry)
            m_pRegisry->deleteLater();
        if (m_pConf)
            m_pRegisry->deleteLater();
        if (m_pManager)
            m_pManager->deleteLater();

    });

    QObject::connect(m_pConnectThread, &ConnectionThread::failed, [ & ] {
          qDebug() << "connect failed to wayland at socket:" << m_pConnectThread->socketName();
          m_bConnected = true;

    });
    QObject::connect(m_pConnectThread, &ConnectionThread::connectionDied, [ & ] {
          qDebug() << "connect failed to wayland at socket:" << m_pConnectThread->socketName();
          if (m_pRegisry)
          {
              m_pRegisry->deleteLater();
          }
          if (m_pConf)
          {
              m_pConf->deleteLater();
          }

    });

}


QString wloutput_interface::ListOutput()
{
    qDebug() << "in ListOutput()";
    QString rst;
    if(uuid2OutputDevice.empty())
    {
        return rst;
    }

    QList<OutputInfo> listOutputInfos;
    auto oIter = uuid2OutputDevice.begin();
    for (; oIter != uuid2OutputDevice.end(); ++oIter) {
        OutputInfo stOutputInfo = GetOutputInfo(oIter.value());
        //qDebug() << stOutputInfo.uuid;
        listOutputInfos.push_back(stOutputInfo);
    }

    return OutputInfo2Json(listOutputInfos);
}

QString wloutput_interface::GetOutput(QString uuid)
{
    qDebug() << "in GetOutput() uuid=" << uuid;
    QString rst;
    auto itFind = uuid2OutputDevice.find(uuid);
    if (itFind != uuid2OutputDevice.end()) {
        QList<OutputInfo> listOutputInfos;
        OutputInfo stOutputInfo = GetOutputInfo(itFind.value());
        listOutputInfos.push_back(stOutputInfo);
        rst = OutputInfo2Json(listOutputInfos);
    }
    return  rst;
}

void wloutput_interface::Apply(QString outputs)
{
    qDebug() << "in Apply()";
    //qDebug() <<  outputs;
    QList<OutputInfo> listOutputInfo = json2OutputInfo(outputs);
    if (!listOutputInfo.empty()) {

        auto itFind = uuid2OutputDevice.find(listOutputInfo.at(0).uuid);
        if (itFind != uuid2OutputDevice.end()) {
            OutputInfo stOutputInfo = listOutputInfo.at(0);
            auto dev = itFind.value();
            if (!stOutputInfo.lstModeInfos.empty())
            {

                for (auto m : dev->modes())
                {
//                    if (m.size.width() == stOutputInfo.lstModeInfos.at(0).width &&
//                            m.size.height() == stOutputInfo.lstModeInfos.at(0).height &&
//                            m.refreshRate == stOutputInfo.lstModeInfos.at(0).refresh_rate)
//                    {
                        qDebug() << "setmode id" << stOutputInfo.lstModeInfos.at(0).id;
                        m_pConf->setMode(dev, stOutputInfo.lstModeInfos.at(0).id);
//                    }
                }
            }
            m_pConf->setPosition(dev, QPoint(stOutputInfo.x, stOutputInfo.y));
            m_pConf->setEnabled(dev, OutputDevice::Enablement(stOutputInfo.enabled));
            qDebug() << "set output transform to " << stOutputInfo.transform;
            m_pConf->setTransform(dev, OutputDevice::Transform(stOutputInfo.transform));
            m_pConf->apply();

            QObject::connect(m_pConf, &OutputConfiguration::applied, [this]() {
                qDebug() << "Configuration applied!";
                m_bConnected = true;
            });
            QObject::connect(m_pConf, &OutputConfiguration::failed, [this]() {
                qDebug() << "Configuration failed!";
                m_bConnected = true;
            });

        }

    }
    else {
        qDebug() << "listOutputInfo is empty";
    }
}
