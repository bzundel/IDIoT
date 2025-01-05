from enum import Enum

class SENSORTYPE(Enum): # TODO map to RIOT sensor values (maybe using definition header?)
    TEMPERATURE = 1
    HUMIDITY = 1

class Node:
    def __init__(self, name: str) -> None:
        self.name: str = name
        self.sensors = list()

    def __repr__(self):
        return f"(Node: {self.name}, {self.sensors})"

    def __str__(self):
        return self.__repr__()

class Sensor:
    def __init__(self, sensor_type: SENSORTYPE, interval: int):
        self.sensor_type: SENSORTYPE = sensor_type
        self.interval: int = interval

    def __repr__(self):
        return f"(Sensor: {self.sensor_type}, {self.interval})"

    def __str__(self):
        return self.__repr__()
