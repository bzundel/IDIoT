defmodule Parser do
  import NimbleParsec

  whitespace = ascii_char([?\s, ?\n, ?\r, ?\t]) |> repeat()
  whitespace_force = ascii_char([?\s, ?\n, ?\r, ?\t]) |> times(min: 1)
  identifier = ascii_string([?a..?z, ?A..?Z, ?_], min: 1)
  quoted_string = ignore(string("\"")) |> ascii_string([not: ?"], min: 0) |> ignore(string("\""))
  equals = ignore(whitespace) |> string("=") |> ignore(whitespace)

  def reduce_node([name, sensors]) do
    %{name: name, sensors: sensors}
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

  defparsec(:parse, repeat(node |> ignore(optional(whitespace))))
end
