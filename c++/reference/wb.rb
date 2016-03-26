#!/usr/bin/env ruby

require 'webrick'

root = File.expand_path './www.cplusplus.com'
server = WEBrick::HTTPServer.new :Port => 8081, :DocumentRoot => root

trap 'INT' do server.shutdown end

server.start
