defmodule Mix.Tasks.Parse do
  use Mix.Task

  def run(filename) do
    Main.main(filename)
  end

end
