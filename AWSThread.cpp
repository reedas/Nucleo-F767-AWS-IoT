
#define MQTTCLIENT_QOS1 0
#define MQTTCLIENT_QOS2 0

#include "MQTTClientMbedOs.h"
#include "MQTT_server_setting.h"
#include "NTPClient.h"
#include "TLSSocket.h"
#include "mbed.h"

#define LED_ON MBED_CONF_APP_LED_ON
#define LED_OFF MBED_CONF_APP_LED_OFF

extern NetworkInterface *network;

static bool isPublish = false;

/* Flag to be set when received a message from the server. */
static volatile bool isMessageArrived = false;
/* Buffer size for a receiving message. */
const int MESSAGE_BUFFER_SIZE = 256;
/* Buffer for a receiving message. */
char messageBuffer[MESSAGE_BUFFER_SIZE];
  bool isSubscribed = false;

  TLSSocket *socket =
      new TLSSocket; // Allocate on heap to avoid stack overflow.
  MQTTClient *mqttClient = NULL;


// An event queue is a very useful structure to debounce information between
// contexts (e.g. ISR and normal threads) This is great because things such as
// network operations are illegal in ISR, so updating a resource in a button's
// fall() function is not allowed
EventQueue eventQueue;
// Thread thread1;
static volatile bool buttonPress = false;

/*
 * Callback function called when the button1 is clicked.
 */
void btn1_rise_handler() { buttonPress = true; }

/* possible publish commands */
typedef enum {
  CMD_sendTemperature,
  CMD_sendIPAddress,
  CMD_sendSetPoint,
  CMD_sendLightLevel,
  CMD_sendRelativeHumidity,
  CMD_sendMode,
  CMD_sendDelta,
  CMD_sendAnnounce1,
  CMD_sendAnnounce2,
  CMD_sendCount
} command_t;
typedef struct {
  command_t cmd;
  float value; /* eg ADC result of measured voltage */
} msg_t;

// An event queue is a very useful structure to debounce information between
// contexts (e.g. ISR and normal threads) This is great because things such as
// network operations are illegal in ISR, so updating a resource in a button's
// fall() function is not allowed
static Queue<msg_t, 16> queue;
static MemoryPool<msg_t, 16> mpool;
void messageArrived(MQTT::MessageData &md) {
  // Copy payload to the buffer.
  MQTT::Message &message = md.message;
  if (message.payloadlen >= MESSAGE_BUFFER_SIZE) {
    printf("Too Big");
  } else {
    memcpy(messageBuffer, message.payload, message.payloadlen);
  }
  messageBuffer[message.payloadlen] = '\0';

  isMessageArrived = true;
}

void client_connect() {
    {
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)MQTT_CLIENT_ID;
    data.username.cstring = (char *)MQTT_USERNAME;
    data.password.cstring = (char *)MQTT_PASSWORD;

    mqttClient = new MQTTClient(socket);
    int rc = mqttClient->connect(data);
    if (rc != MQTT::SUCCESS) {
      printf("ERROR: rc from MQTT connect is %d\r\n", rc);
      return;
    }
  }
  printf("Client connected.\r\n");
  printf("\r\n");

  printf("Client is trying to subscribe a topic \"%s\".\r\n", MQTT_TOPIC_SUB);
  {
    int rc = mqttClient->subscribe(MQTT_TOPIC_SUB, MQTT::QOS0, messageArrived);
    if (rc != MQTT::SUCCESS) {
      printf("ERROR: rc from MQTT subscribe is %d\r\n", rc);
      return;
    }
    isSubscribed = true;
  }
  printf("Client has subscribed a topic \"%s\".\r\n", MQTT_TOPIC_SUB);
  printf("\r\n");

}
void awsSendUpdateTemperature(float temperature) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendTemperature;
    message->value = temperature;
    queue.put(message);
    isPublish = true;
  }
}
void awsSendAnnounce1(void) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendAnnounce1;
    message->value = 0;
    queue.put(message);
    isPublish = true;
  }
}
void awsSendAnnounce2(void) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendAnnounce2;
    message->value = 0;
    queue.put(message);
    isPublish = true;
  }
}
void awsSendIPAddress(void) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendIPAddress;
    message->value = 0;
    queue.put(message);
    isPublish = true;
  }
}
void awsSendUpdateSetPoint(float setPoint) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendSetPoint;
    message->value = setPoint;
    queue.put(message);
    isPublish = true;
  }
}
void awsSendUpdateDelta(float delta) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendDelta;
    message->value = delta;
    queue.put(message);
    isPublish = true;
  }
}
void awsSendUpdateMode(float controlMode) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendMode;
    message->value = controlMode;
    queue.put(message);
    isPublish = true;
  }
}
void awsSendUpdateLightLevel(float lighLlevel) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendLightLevel;
    message->value = lighLlevel;
    queue.put(message);
    isPublish = true;
  }
}
void awsSendUpdateRelativeHumidity(float relHumidity) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendRelativeHumidity;
    message->value = relHumidity;
    queue.put(message);
    isPublish = true;
  }
}
void awsSendcount(void) {
  msg_t *message = mpool.alloc();
  if (message) {
    message->cmd = CMD_sendCount;
    message->value = 0;
    queue.put(message);
    isPublish = true;
  }
}

/*
 * Callback function called when a message arrived from server.
 */

void AWSThread() {

  DigitalOut led(MBED_CONF_APP_LED_PIN, LED_ON);

  // sync the real time clock (RTC)
  NTPClient ntp(network);
  char ntpServer[20] = "time.google.com";
  ntp.set_server(ntpServer, 123);
  time_t now = ntp.get_timestamp() + 3600; // BST hack add an hour
  //   time_t now = ntp.get_timestamp(); // GMT
  set_time(now);
  printf("Time is now %s", ctime(&now));

  printf("Connecting to host %s:%d ...\r\n", MQTT_SERVER_HOST_NAME,
         MQTT_SERVER_PORT);
  {
    nsapi_error_t ret = socket->open(network);
    if (ret != NSAPI_ERROR_OK) {
      printf("Could not open socket! Returned %d\n", ret);
      return;
    }
    ret = socket->set_root_ca_cert(SSL_CA_PEM);
    if (ret != NSAPI_ERROR_OK) {
      printf("Could not set ca cert! Returned %d\n", ret);
      return;
    }
    ret = socket->set_client_cert_key(SSL_CLIENT_CERT_PEM,
                                      SSL_CLIENT_PRIVATE_KEY_PEM);
    if (ret != NSAPI_ERROR_OK) {
      printf("Could not set keys! Returned %d\n", ret);
      return;
    }
    ret = socket->connect(MQTT_SERVER_HOST_NAME, MQTT_SERVER_PORT);
    if (ret != NSAPI_ERROR_OK) {
      printf("Could not connect! Returned %d\n", ret);
      return;
    }
  }
  printf("Connection established.\r\n");
  printf("\r\n");

  printf("MQTT client is trying to connect the server ...\r\n");
  client_connect(); 
  // Enable button 1
  InterruptIn btn1(MBED_CONF_APP_USER_BUTTON);
  btn1.rise(btn1_rise_handler);

      /* Pass control to other thread. */
  awsSendIPAddress();

  printf("To send a packet, push the button 1 on your board.\r\n\r\n");

  // Turn off the LED to let users know connection process done.
  led = LED_OFF;

  while (1) {
    /* Check connection */
    if (!mqttClient->isConnected()) {
      client_connect();
    }
    /* Pass control to other thread. */
    if (mqttClient->yield() != MQTT::SUCCESS) {
        printf("Yield failed\n");
        break;    
    }
    if (buttonPress) {
      awsSendcount();
      buttonPress = false;
    }

    /* Received a control message. */
    if (isMessageArrived) {
      isMessageArrived = false;
      // Just print it out here.
      printf("\r\nMessage arrived:\r\n%s\r\n\r\n", messageBuffer);
    }
    /* Publish data */
    while (!queue.empty()) {
      osEvent evt = queue.get(0);
      static unsigned short id = 0;
      static unsigned int count = 0;

      count++;

      // When sending a message, LED lights blue.
      led = LED_ON;

      MQTT::Message message;
      message.retained = false;
      message.dup = false;

      const size_t buf_size = 200;
      char *buf = new char[buf_size];
      message.payload = (void *)buf;

      message.qos = MQTT::QOS0;
      message.id = id++;
      int ret;
      if (evt.status == osEventMessage) {
        msg_t *msg = (msg_t *)evt.value.p;
        //        printf("%d", message->cmd);
        switch (msg->cmd) {
        case CMD_sendIPAddress:
          isPublish = true;
          ret = snprintf(buf, buf_size,
                   "{\"state\":{\"reported\":{\"IP\":\"%s\"}}}",
                   network->get_ip_address());
          break;
        case CMD_sendTemperature:
          isPublish = true;
          ret = snprintf(buf, buf_size,
                   "{\"state\":{\"reported\":{\"Temp\":\"%2.1f C\"}}}",
                   msg->value);
          break;
        case CMD_sendSetPoint:
          isPublish = true;
          ret = snprintf(buf, buf_size,
                   "{\"state\":{\"reported\":{\"setPoint\":\"%2.1f\"}}}",
                   msg->value);
          break;
        case CMD_sendRelativeHumidity:
          isPublish = true;
          ret = snprintf(buf, buf_size,
                   "{\"state\":{\"reported\":{\"Humidity\":\"%2.1f\"}}}",
                   msg->value);
          break;
        case CMD_sendLightLevel:
          isPublish = true;
          ret = snprintf(buf, buf_size,
                   "{\"state\":{\"reported\":{\"Light Lev\":\"%2.1f\"}}}",
                   msg->value);
          break;
        case CMD_sendMode:
          isPublish = true;
          char mode[5];
          if ((msg->value) >= 1)
            sprintf(mode, "Cool");
          else if ((msg->value) <= -1)
            sprintf(mode, "Heat");
          else
            sprintf(mode, "Off");
          ret = snprintf(buf, buf_size,
                   "{\"state\":{\"reported\":{\"Mode\":\"%s\"}}}",
                   mode);
          break;
        case CMD_sendCount:
          ret = snprintf(buf, buf_size,
                       "{\"state\":{\"reported\":{\"count\":\"%d\"}}}", count);
          isPublish = true;
          break;
        default:
          isPublish = true;
          ret = snprintf(buf, buf_size,
                   "{\"state\":{\"reported\":{\"???\":\"%2.1f\"}}}",
                   msg->value);
          break;
        }
        message.payloadlen = ret;
        mpool.free(msg);
        if (isPublish) {
          // Publish a message.
          isPublish = false;

          printf("Publishing message. %s to %s\r\n", buf, MQTT_TOPIC_PUB);
          int rc = mqttClient->publish(MQTT_TOPIC_PUB, message);
          if (rc != MQTT::SUCCESS) {
            printf("ERROR: rc from MQTT publish is %d\r\n", rc);
          }
          printf("Message published.\r\n");
          delete[] buf;

          thread_sleep_for(50);
          led = LED_OFF;
        }
      }
    }
  }
  printf("The client has disconnected.\r\n");

  if (mqttClient) {
    if (isSubscribed) {
      mqttClient->unsubscribe(MQTT_TOPIC_SUB);
    }
    if (mqttClient->isConnected())
      mqttClient->disconnect();
    delete mqttClient;
  }
  if (socket) {
    socket->close();
  }
  if (network) {
    network->disconnect();
    // network is not created by new.
  }
}
