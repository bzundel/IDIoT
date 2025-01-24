defmodule Main do
  require Logger

  defp fill_template(content, fields) do # FIXME insert escaped?
    Enum.reduce(fields, content, fn {key, value}, acc ->
      String.replace(acc, "%%#{key}%%", value)
    end)
  end

  def main(filename) do
    {:ok, content} = File.read(filename)
    {:ok, [result], _, _, _, _} = Parser.parse(content)

    outpath = Path.absname(Path.join("out", result.name))
    snippets_path = "idsnippets"

    case File.mkdir_p(outpath) do
      :ok -> Logger.info("Created directory #{Path.absname(outpath)}")
      {:error, reason} ->
        Logger.error("Failed to create directory #{outpath} with reason: #{reason}")
        exit(:failure)
    end

    make_wifi_content = case File.read(Path.join(snippets_path, "Makefile.WIFI.include.idsnippet")) do
      {:ok, content} ->
        fill_template(content, result.wifi)
      {:error, reason} ->
        Logger.error("Could not open file #{Path.join(snippets_path, "Makefile.WIFI.include.idsnippet")} with reason: #{reason}")
        exit(:failure)
    end

    make_wifi_path = Path.join(outpath, "Makefile.WIFI.include")
    case File.write(make_wifi_path, make_wifi_content) do
      :ok ->
        Logger.info("Successfully written to #{make_wifi_path}")
      {:error, reason} ->
        Logger.error("Failed to write to file #{make_wifi_path} with reason: #{reason}")
    end
  end
end
