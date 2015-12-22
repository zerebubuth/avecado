# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure(2) do |config|
  config.vm.provision "shell", inline: <<-SHELL
    sudo apt-get update
    sudo apt-get upgrade -y
    sudo apt-get install -y build-essential autoconf automake libtool libboost-all-dev python-dev libprotobuf-dev protobuf-compiler
  SHELL

  config.vm.define "wily64" do |wily|
    # see https://github.com/mitchellh/vagrant/issues/6683
    wily.vm.box = "sgallen/wily64"

    # wily seems to take an absolute _age_ to boot, seemingly hanging at the
    # "puppet.service" start stage for minutes.
    wily.vm.boot_timeout = 6000

    wily.vm.provider "virtualbox" do |vb|
      # needs lots and lots of memory for all those templates
      vb.memory = "2048"
    end

    wily.vm.provision "shell", inline: <<-SHELL
      sudo apt-get install -y libmapnik-dev
    SHELL
  end

  # note: this doesn't quite work yet - needs a bit more work to get a more
  # recent version of boost compiled up so that mapnik & avecado can use it.
  # mapnik is okay with 1.54, but avecado needs >=1.56, not currently
  # available in mainline trusty.
  #
  # config.vm.define "trusty64", autostart: false do |trusty|
  #   trusty.vm.box = "ubuntu/trusty64"

  #   trusty.vm.provider "virtualbox" do |vb|
  #     # needs lots and lots of memory to compile mapnik
  #     vb.memory = "6144"
  #   end

  #   trusty.vm.provision "shell", inline: <<-SHELL
  #     sudo apt-get install -y git pkg-config libharfbuzz-dev libfreetype6-dev libpq-dev libgdal1-dev libproj-dev libcairo2-dev libtiff5-dev
  #     cd /usr/local/src
  #     sudo mkdir mapnik-3.0.9
  #     sudo chown vagrant:vagrant mapnik-3.0.9
  #     curl -s https://codeload.github.com/mapnik/mapnik/tar.gz/v3.0.9 | tar zxf -
  #     cd mapnik-3.0.9/
  #     ./configure PREFIX=/usr/local CC=gcc CXX=g++
  #     make
  #     sudo make install
  #   SHELL
  # end
end
