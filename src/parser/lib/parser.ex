defmodule Parser do
  import NimbleParsec
  require Logger

  whitespace = ascii_char([?\s, ?\n, ?\r, ?\t]) |> repeat()
  whitespace_force = ascii_char([?\s, ?\n, ?\r, ?\t]) |> times(min: 1)
  identifier = ascii_string([?a..?z, ?A..?Z, ?_], min: 1)
  quoted_string = ignore(string("\"")) |> ascii_string([not: ?"], min: 0) |> ignore(string("\""))
  equals = ignore(whitespace) |> string("=") |> ignore(whitespace)
  field = fn (name) -> # TODO use fields in parsing
    ignore(string(name))
    |> ignore(equals)
    |> concat(quoted_string)
    |> ignore(whitespace)
    |> unwrap_and_tag(String.to_atom(name))
  end

  def reduce_project([{:name, name}, {:riot_path, riot_path}, wifi, {:mosquitto_broker_address, address}, nodes]) do
    {%{name: name, riot_path: riot_path, wifi: wifi, mosquitto_broker_address: address}, nodes}
  end

  def reduce_wifi([ssid, password]) do
    %{ssid: ssid, password: password}
  end

  def reduce_node([name, sensors]) do
    %{name: name, sensors: sensors}
  end

  def reduce_nodes(nodes) do
    nodes
  end

  def reduce_sensor([type, interval]) do
    %{type: type, interval: interval}
  end

  def reduce_sensor_list(sensors) do
    sensors
  end

  def reduce_environment_variable([name]) do
    case System.get_env(name) do
      nil ->
        Logger.error("Environment variable #{name} not set")
        exit(:failure)
      value ->
        value
    end

  end

  environment_variable =
    ignore(string("$"))
    |> concat(ascii_string([?a..?z, ?A..?Z, ?0..?9, ?_], min: 1))
    |> reduce({:reduce_environment_variable, []})

  sensor =
    ignore(string("{"))
    |> ignore(whitespace)
    |> ignore(string("type")) |> ignore(equals) |> concat(quoted_string)
    |> ignore(whitespace)
    |> ignore(string("interval")) |> ignore(equals) |> concat(quoted_string)
    |> ignore(whitespace)
    |> ignore(string("}"))
    |> reduce({:reduce_sensor, []}) # FIXME weirdly hacky

  sensor_list =
    ignore(string("sensors"))
    |> ignore(equals)
    |> ignore(string("["))
    |> ignore(whitespace)
    |> repeat(
      sensor
      |> ignore(optional(string(",")))
      |> ignore(whitespace)
    )
    |> ignore(string("]"))
    |> reduce({:reduce_sensor_list, []})

  node =
    ignore(string("node"))
    |> ignore(whitespace_force)
    |> concat(identifier)
    |> ignore(equals)
    |> ignore(string("{"))
    |> ignore(whitespace)
    |> optional(sensor_list)
    |> ignore(whitespace)
    |> ignore(string("}"))
    |> reduce({:reduce_node, []})

  nodes =
    repeat(node |> ignore(optional(whitespace)))
    |> reduce({:reduce_nodes, []})

  wifi =
    ignore(string("wifi_ssid"))
    |> ignore(equals)
    |> concat(choice([quoted_string, environment_variable])) # FIXME use fields
    |> ignore(whitespace)
    |> ignore(string("wifi_password"))
    |> ignore(equals)
    |> concat(choice([quoted_string, environment_variable]))
    |> ignore(whitespace)
    |> reduce({:reduce_wifi, []})

  project =
    field.("name")
    |> concat(field.("riot_path"))
    |> concat(wifi)
    |> concat(field.("mosquitto_broker_address"))
    |> concat(nodes)
    |> reduce({:reduce_project, []})

  defparsec(:parse, project)
end
