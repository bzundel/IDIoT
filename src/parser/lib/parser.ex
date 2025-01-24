defmodule Parser do
  import NimbleParsec

  whitespace = ascii_char([?\s, ?\n, ?\r, ?\t]) |> repeat()
  whitespace_force = ascii_char([?\s, ?\n, ?\r, ?\t]) |> times(min: 1)
  identifier = ascii_string([?a..?z, ?A..?Z, ?_], min: 1)
  quoted_string = ignore(string("\"")) |> ascii_string([not: ?"], min: 0) |> ignore(string("\""))
  equals = ignore(whitespace) |> string("=") |> ignore(whitespace)
  #field = fn (name) -> ignore(string(name)) |> ignore(equals) |> concat(quoted_string) |> ignore(whitespace) end

  def reduce_project([name, wifi, nodes]) do
    %{name: name, wifi: wifi, nodes: nodes}
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
    |> concat(quoted_string)
    |> ignore(whitespace)
    |> ignore(string("wifi_password"))
    |> ignore(equals)
    |> concat(quoted_string)
    |> ignore(whitespace)
    |> reduce({:reduce_wifi, []})

  project =
    ignore(string("name"))
    |> ignore(equals)
    |> concat(quoted_string)
    |> ignore(whitespace)
    |> concat(wifi)
    |> concat(nodes)
    |> reduce({:reduce_project, []})

  defparsec(:parse, project)
end
