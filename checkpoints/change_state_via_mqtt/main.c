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

static int _subscribe(char* topic) {
    enum QoS qos = QOS0;

    int ret = MQTTSubscribe(&client, topic, qos, _message_received);
    if (ret < 0) {
        printf("DEBUG: Unable to subscribe to %s. Code: %d\n", topic, ret);
    }
    else {
        printf("DEBUG: Now subscribed to \"%s\"\n", topic);
    }

    return ret;
}

static int _unsubscribe(char* topic) {
    int ret = MQTTUnsubscribe(&client, topic);

    return ret;
}

static char* _get_device_config_topic(char* subtopic) {
    char* ret = malloc(sizeof(char) * MAX_LEN_TOPIC);
    sprintf(ret, "%s/config/%s", device_name, subtopic);

    return ret;
}

static void first_n_characters(char* dest, const char* src, int n) {
    strncpy(dest, src, n);
    dest[n] = '\0';
}

static int _publish(char* topic, char* payload) {
    enum QoS qos = QOS0;

    MQTTMessage message;
    message.qos = qos;
    message.retained = IS_RETAINED_MSG;
    message.payload = payload;
    message.payloadlen = strlen(payload);

    int rc;
    if ((rc = MQTTPublish(&client, topic, &message)) < 0) {
        printf("DEBUG: Unable to publish \"%s\" to \"%s\"", topic, payload);
    }
    else {
        printf("DEBUG: Published \"%s\"\n", (char*)payload);
    }

    return rc;
}

void* _thread_publish(void* args) {
    MQTTPackage* package = (MQTTPackage*)args;

    _publish(package->topic, package->message);

    free(package);

    return NULL;
}

// this might be superfluous
void _publish_async(const char* topic, const char* message) {
    MQTTPackage* package = _create_mqttpackage(topic, message);

    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack), THREAD_PRIORITY_MAIN - 1, 0, _thread_publish, package, "Message publish thread");
}

static void _message_received(MessageData *data) {
    char topic[MAX_LEN_TOPIC];
    char message[200];

    first_n_characters(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
    first_n_characters(message, data->message->payload, data->message->payloadlen);

    printf("DEBUG: Received \"%s\"on \"%s\"\n", message, topic);

    if (strcmp("config/name/get", topic) == 0) {
        _publish_async("all", device_name);
    }
    else if (strcmp("config/name/set", topic) == 0) {
        char* device_config_topic;
        device_config_topic = _get_device_config_topic("name/get");
        _unsubscribe(device_config_topic);

        strcpy(device_name, message);

        device_config_topic = _get_device_config_topic("name/set");
        _subscribe(device_config_topic);

        free(device_config_topic);

    }
}



static unsigned char buf[BUF_SIZE]; // FIXME these do what?
static unsigned char readbuf[BUF_SIZE];

int main(void)
{
    if (IS_USED(MODULE_GNRC_ICMPV6_ECHO)) {
        msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    }

    NetworkInit(&network);

    MQTTClientInit(&client, &network, COMMAND_TIMEOUT_MS, buf, BUF_SIZE, readbuf, BUF_SIZE);
    MQTTStartTask(&client);

    // wait for assigned ipv6 address
    gnrc_netif_ipv6_wait_for_global_address(NULL, 2 * MS_PER_SEC);

    char* remote_ip = REMOTE_IP;
    int port = DEFAULT_MQTT_PORT;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = MQTT_VERSION_v311;
    data.cleansession = IS_CLEAN_SESSION;
    data.willFlag = 0;

    // initial device_name initialization
    strncpy(device_name, DEVICE_NAME, MAX_LEN_DEVICE_NAME - 1);
    device_name[MAX_LEN_DEVICE_NAME - 1] = '\0';

    int ret = -1;
    ret = NetworkConnect(&network, remote_ip, port);

    if (ret < 0) {
        puts("DEBUG: Unable to connect to broker");
        return ret;
    }

    ret = MQTTConnect(&client, &data);
    if (ret < 0) {
        puts("DEBUG: Unable to connect client");
        return ret;
    }

    _subscribe("all");

    char* config_name;

    config_name = _get_device_config_topic("name/get");
    _subscribe(config_name);
    config_name = _get_device_config_topic("name/set");
    _subscribe(config_name);

    free(config_name);

    return 0;
}
