import argparse
import tomllib
import logging
import re

from model import Node, Sensor, SENSORTYPE

logger = logging.getLogger(__name__)
logging.basicConfig(filename="generator.log", level=logging.INFO)

parser = argparse.ArgumentParser(prog="IDIoT processor", description="Processes an IDIoT configuration file into RIOT-buildable code")
parser.add_argument("config", help="Configuration file to be processed")
args = parser.parse_args()

def sensortype_from_config_value(value):
    match(value):
        case "sensor_type.temperature":
            return SENSORTYPE.TEMPERATURE
        case "sensor_type.humidity":
            return SENSORTYPE.HUMIDITY
        case _:
            logger.warning(f"Sensor type \"{value}\" in config file was not found.")
            raise NotImplementedError(f"Sensor type \"{value}\" does not exist or has not been implemented yet")

def interval_to_seconds(interval):
    pattern = r"^(\d+)([smh])$"
    match = re.match(pattern, interval)

    if not match:
        logger.error(f"Converting interval to seconds was unsuccessful. Given value: {interval}")
        raise ValueError(f"Invalid interval format: {interval}")

    number, suffix = match.groups()
    number = int(number)

    match(suffix):
        case "s":
            return number
        case "m":
            return number * 60
        case "h":
            return number * 60 * 60
        case _:
            raise ValueError(f"Invalid interval suffix: {interval}")

with open(args.config, "rb") as f:
    data = tomllib.load(f)

nodes = list()

for node_key in data["node"].keys():
    node = Node(node_key)
    node.sensors = [Sensor(sensortype_from_config_value(x["type"]), interval_to_seconds(x["interval"])) for x in data["node"][node_key]["sensors"]]
    nodes.append(node)

print(nodes)
