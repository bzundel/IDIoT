# Build requirements

Two files, `Makefile.WIFI.include` and `Makefile.MQTT.include`, of the following structure are required which contain information about connecting to the network and connecting to the broker, respectively:

```
# Makefile.WIFI.include
WIFI_SSID ?= "SSID"
WIFI_PASS ?= "PASSWORD"
```

```
# Makefile.MQTT.include
REMOTE_ID ?= IP
```
