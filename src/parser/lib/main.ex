defmodule Main do
  require Logger

  defp fill_template(content, fields, escape?) do
    Enum.reduce(fields, content, fn {key, value}, acc ->
      String.replace(acc, "%%#{key}%%", cond do
        escape? -> "\"#{value}\""
        true -> value
      end)
    end)
  end

  defp generate_file_from_map(snippet_file, out_file, map, escape?) do
    output = case File.read(snippet_file) do
      {:ok, content} ->
        fill_template(content, map, escape?)
      {:error, reason} ->
        Logger.error("Could not open file #{snippet_file} with reason: #{reason}")
        exit(:failure)
    end

    case File.write(out_file, output) do
      :ok ->
        Logger.info("Successfully written to #{out_file}")
      {:error, reason} ->
        Logger.error("Failed to write to file #{out_file} with reason: #{reason}")
    end
  end

  defp generate_files_from_node(meta, node, outpath) do
    node_outpath = Path.join(outpath, node.name)

    case File.mkdir(node_outpath) do # TODO make a function for mkdir with fancy error handling to reduce redundancy
      :ok -> Logger.info("Created directory #{Path.absname(node_outpath)}")
      {:error, reason} ->
        Logger.error("Failed to create directory #{node_outpath} with reason: #{reason}")
        exit(:failure)
    end

    generate_file_from_map(
      Path.join(RiotConversions.get_path(:snippets), RiotConversions.get_snippet_file(:makefile)),
      Path.join(node_outpath, RiotConversions.get_file(:makefile)),
      %{
        "name" => node.name,
        "riot_path" => meta.riot_path
      },
      false
    )
  end

  defp generate({meta, nodes}) do
    outpath = Path.absname(Path.join(RiotConversions.get_path(:output), meta.name))

    case File.mkdir_p(outpath) do
      :ok -> Logger.info("Created directory #{Path.absname(outpath)}")
      {:error, reason} ->
        Logger.error("Failed to create directory #{outpath} with reason: #{reason}")
        exit(:failure)
    end

    generate_file_from_map(
      Path.join(RiotConversions.get_path(:snippets), RiotConversions.get_snippet_file(:makefile_wifi)),
      Path.join(outpath, RiotConversions.get_file(:makefile_wifi)),
      meta.wifi,
      true)

    for node <- nodes do
      generate_files_from_node(meta, node, outpath)
    end
  end

  def main(filename) do
    {:ok, content} = File.read(filename)
    {:ok, [{meta, nodes}], _, _, _, _} = Parser.parse(content)

    IO.inspect(meta)
    IO.inspect(nodes)

    Logger.info("Read #{Enum.count(nodes)} nodes")

    generate({meta, nodes})
  end
end
