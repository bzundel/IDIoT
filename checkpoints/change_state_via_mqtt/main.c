#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <phydat.h>
#include "timex.h"
#include "ztimer.h"
#include "shell.h"
#include "thread.h"
#include "mutex.h"
#include "paho_mqtt.h"
#include "MQTTClient.h"
#include "saul_reg.h"
#include "unistd.h"
#include "math.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

#define BUF_SIZE                        1024
#define MQTT_VERSION_v311               4
#define COMMAND_TIMEOUT_MS              4000

#ifndef DEFAULT_MQTT_CLIENT_ID
#define DEFAULT_MQTT_CLIENT_ID          ""
#endif

#ifndef DEFAULT_MQTT_USER
#define DEFAULT_MQTT_USER               ""
#endif

#ifndef DEFAULT_MQTT_PWD
#define DEFAULT_MQTT_PWD                ""
#endif

#define DEFAULT_MQTT_PORT               1883

#define DEFAULT_KEEPALIVE_SEC           10

#define MAX_LEN_TOPIC                   100
#define MAX_LEN_MESSAGE                 200
#define MAX_LEN_DEVICE_NAME             50

#ifndef MAX_TOPICS
#define MAX_TOPICS                      4
#endif

#define IS_CLEAN_SESSION                1
#define IS_RETAINED_MSG                 0

#define REMOTE_IP "2001:16b8:c597:6700:ef6a:20f9:bf35:b874"

static MQTTClient client;
static Network network;
static char device_name[MAX_LEN_DEVICE_NAME];
char rcv_thread_stack[THREAD_STACKSIZE_MAIN];

typedef struct MQTTPackage {
    char topic[MAX_LEN_TOPIC];
    char message[MAX_LEN_MESSAGE];
} MQTTPackage;

static MQTTPackage* _create_mqttpackage(const char* topic, const char* message) {
    MQTTPackage* package = malloc(sizeof(MQTTPackage));
    strcpy(package->topic, topic);
    strcpy(package->message, message);

    return package;
}

static void first_n_characters(char* dest, const char* src, int n) {
    strncpy(dest, src, n);
    dest[n] = '\0';
}

static int publish(char* topic, char* payload) {
    enum QoS qos = QOS0;

    MQTTMessage message;
    message.qos = qos;
    message.retained = IS_RETAINED_MSG;
    message.payload = payload;
    message.payloadlen = strlen(payload);

    puts("Publishing...");

    int rc;
    if ((rc = MQTTPublish(&client, topic, &message)) < 0) {
        puts("Unable to publish");
    }
    else {
        printf("Successfully published \"%s\"\n", (char*)payload);
    }

    return rc;
}

void* _thread_publish(void* args) {
    MQTTPackage* package = (MQTTPackage*)args;

    puts("---Thread");
    printf("%s\n", package->topic);
    printf("%s\n", package->message);

    publish(package->topic, package->message);

    free(package);

    return NULL;
}

// this might be superfluous
void _publish_async(const char* topic, const char* message) {
    MQTTPackage* package = _create_mqttpackage(topic, message);

    puts("Created package");
    printf("%s\n", package->topic);
    printf("%s\n", package->message);

    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack), THREAD_PRIORITY_MAIN - 1, 0, _thread_publish, package, "Message publish thread");
}

static void _message_received(MessageData *data) {
    char topic[MAX_LEN_TOPIC];
    char message[200];

    first_n_characters(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
    first_n_characters(message, data->message->payload, data->message->payloadlen);

    printf("Message received: %s\n", message);

    if (strcmp("name/get", topic) == 0) {
        printf("Device name: %s\n", device_name);
        _publish_async("all", device_name);
    }
    else if (strcmp("name/set", topic) == 0) {
        strcpy(device_name, message);
    }

    printf("Message received: %.*s: %.*s\n",
           (int)data->topicName->lenstring.len,
           data->topicName->lenstring.data, (int)data->message->payloadlen,
           (char *)data->message->payload);
}

static int subscribe(char* topic) {
    enum QoS qos = QOS0;

    int ret = MQTTSubscribe(&client, topic, qos, _message_received);
    if (ret < 0) {
        printf("Unable to subscribe to %s. Code: %d\n", topic, ret);
    }
    else {
        printf("Now subscribed to %s\n", topic);
    }

    sleep(1);

    return ret;
}


static unsigned char buf[BUF_SIZE];
static unsigned char readbuf[BUF_SIZE];

int main(void)
{
    if (IS_USED(MODULE_GNRC_ICMPV6_ECHO)) {
        msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    }

    NetworkInit(&network);

    MQTTClientInit(&client, &network, COMMAND_TIMEOUT_MS, buf, BUF_SIZE, readbuf, BUF_SIZE);
    MQTTStartTask(&client);

    gnrc_netif_ipv6_wait_for_global_address(NULL, 2 * MS_PER_SEC);

    int ret = -1;
    char* remote_ip = REMOTE_IP;
    int port = DEFAULT_MQTT_PORT;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = MQTT_VERSION_v311;
    data.cleansession = IS_CLEAN_SESSION;
    data.willFlag = 0;

    // initial identifier declaration
    strncpy(device_name, DEVICE_NAME, MAX_LEN_DEVICE_NAME - 1);
    device_name[MAX_LEN_DEVICE_NAME - 1] = '\0';

    printf("Device name initialized as %s\n", device_name);
    ret = NetworkConnect(&network, remote_ip, port);

    sleep(1);

    printf("%d\n", ret);

    if (ret < 0) {
        puts("Unable to connect to broker");
        return ret;
    }

    ret = MQTTConnect(&client, &data);
    if (ret < 0) {
        puts("Unable to connect client");
        return ret;
    }

    sleep(1);

    subscribe("all");
    subscribe("name/get");
    subscribe("name/set");

    publish("all", "Something!");

    return 0;
}
