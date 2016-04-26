#!/bin/bash

if [[ "$#" -eq 1 ]]; then SSHPORT=$1; else SSHPORT=22; fi

apt-get update -y
apt-get install -y build-essential
apt-get install -y linux-headers-`uname -r`
apt-get install -y g++ gcc git
apt-get install -y python python-pip python-dev

pip install --upgrade pip
pip install locustio

apt-get install -y fail2ban

echo -e "\n" >> /etc/ssh/sshd_config
echo "Port $SSHPORT" >> /etc/ssh/sshd_config
echo "PermitRootLogin without-password" >> /etc/ssh/sshd_config
echo "PasswordAuthentication no" >> /etc/ssh/sshd_config
service ssh restart
