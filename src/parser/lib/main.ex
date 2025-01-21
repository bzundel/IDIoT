defmodule Main do
  def main(filename) do
    {:ok, content} = File.read(filename)
    {:ok, return, _, _, _, _} = Parser.parse(content)

    for node <- return do
      IO.puts(node.name)
    end
  end
end
