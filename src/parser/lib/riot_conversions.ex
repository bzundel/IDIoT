defmodule RiotConversions do
  defp files do
    %{
      :makefile => "Makefile",
      :makefile_wifi => "Makefile.WIFI.include"
    }
  end

  defp paths do
    %{
      :snippets => "idsnippets",
      :output => "out"
    }
  end

  def get_file(key) do
    files()[key]
  end

  def get_snippet_file(key) do
    files()[key] <> ".idsnippet"
  end

  def get_path(key) do
    paths()[key]
  end
end
