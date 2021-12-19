#include "mqtt_dialog.h"

MQTT_Dialog::MQTT_Dialog(MQTTClient::Ptr mosq_client):
  QDialog(nullptr),
  ui(new Ui::DataStreamMQTT),
  _client(mosq_client)
{
  ui->setupUi(this);
  ui->lineEditPort->setValidator( new QIntValidator );

  ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );

  static QString uuid =  QString("Plotjuggler-") + QString::number(rand());

  QSettings settings;
  restoreGeometry(settings.value("MosquittoMQTT::geometry").toByteArray());

  QString host = settings.value("MosquittoMQTT::host").toString();
  ui->lineEditHost->setText( host );

  int port = settings.value("MosquittoMQTT::port", 1883).toInt();
  ui->lineEditPort->setText( QString::number(port) );

  int qos = settings.value("MosquittoMQTT::qos", 0).toInt();
  ui->comboBoxQoS->setCurrentIndex(qos);

  int protocol_mqtt = settings.value("MosquittoMQTT::protocol_version").toInt();
  ui->comboBoxVersion->setCurrentIndex(protocol_mqtt);

  QString topic_filter = settings.value("MosquittoMQTT::filter").toString();
  ui->lineEditTopicFilter->setText(topic_filter );

  QString username = settings.value("MosquittoMQTT::username", "").toString();
  ui->lineEditUsername->setText(username);

  QString password = settings.value("MosquittoMQTT::password", "").toString();
  ui->lineEditPassword->setText(password);

  _topic_list_timer = new QTimer(this);

  connect(ui->buttonConnect, &QPushButton::clicked,
          this, &MQTT_Dialog::onButtonConnect);

  connect(_topic_list_timer, &QTimer::timeout,
          this, &MQTT_Dialog::onUpdateTopicList);

  connect(ui->listWidget, &QListWidget::itemSelectionChanged,
          this, &MQTT_Dialog::onSelectionChanged);
}

MQTT_Dialog::~MQTT_Dialog()
{
  while( ui->layoutOptions->count() > 0)
  {
    auto item = ui->layoutOptions->takeAt(0);
    item->widget()->setParent(nullptr);
  }

  QSettings settings;
  settings.setValue("MosquittoMQTT::geometry", this->saveGeometry());

  // save back to service
  settings.setValue("MosquittoMQTT::host", ui->lineEditHost->text());
  settings.setValue("MosquittoMQTT::port", ui->lineEditPort->text().toInt());
  settings.setValue("MosquittoMQTT::filter", ui->lineEditTopicFilter->text());
  settings.setValue("MosquittoMQTT::username", ui->lineEditPassword->text());
  settings.setValue("MosquittoMQTT::password", ui->lineEditUsername->text());
  settings.setValue("MosquittoMQTT::protocol_version", ui->comboBoxVersion->currentIndex());
  settings.setValue("MosquittoMQTT::qos", ui->comboBoxQoS->currentIndex());
  settings.setValue("MosquittoMQTT::serialization_protocol", ui->comboBoxProtocol->currentText());

  delete ui;
}

void MQTT_Dialog::onButtonConnect()
{
  if( _client->isConnected() )
  {
    _client->disconnect();
    changeConnectionState(false);
    ui->listWidget->clear();
    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );
    return;
  }

  MosquittoConfig config;

  config.host = ui->lineEditHost->text().toStdString();
  config.port = ui->lineEditPort->text().toInt();
  config.topics.clear();
  config.topics.push_back(ui->lineEditTopicFilter->text().toStdString());
  config.username = ui->lineEditUsername->text().toStdString();
  config.password = ui->lineEditPassword->text().toStdString();
  config.qos = ui->comboBoxQoS->currentIndex();
  config.protocol_version =
      MQTT_PROTOCOL_V31 + ui->comboBoxVersion->currentIndex();

  config.keepalive = 60; // TODO
  config.bind_address = ""; //TODO
  config.max_inflight = 20; //TODO
  config.clean_session = true;  //TODO
  config.eol = true;  //TODO

  changeConnectionState(true);
  _client->connect(config);
  _topic_list_timer->start(500);
}

void MQTT_Dialog::onUpdateTopicList()
{
  auto topic_list = _client->getTopicList();
  bool added = false;
  for(const auto& topic: topic_list)
  {
    QString name = QString::fromStdString(topic);
    if( ui->listWidget->findItems(name, Qt::MatchExactly).empty() )
    {
      ui->listWidget->addItem(name);
      added = true;
    }
  }
  if( added )
  {
    ui->listWidget->sortItems();
  }
  changeConnectionState( _client->isConnected() );
}

void MQTT_Dialog::onSelectionChanged()
{
  bool selected = ui->listWidget->selectedItems().count() > 0;
  ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( selected );
}

void MQTT_Dialog::changeConnectionState(bool connected)
{
  ui->connectionFrame->setEnabled(!connected);
  ui->buttonConnect->setText( connected ? "Disconnect" : "Connect");
  ui->lineEditTopicFilter->setEnabled(!connected);
  ui->listWidget->setEnabled(connected);
}
