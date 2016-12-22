# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure(2) do |config|
  config.vm.box = "myos"
  config.vm.synced_folder ".", "/home/vagrant/", type: "rsync"
  config.ssh.forward_x11 = true
  config.vm.provider "virtualbox" do |vb|
    vb.memory = "4096"
  end
end
