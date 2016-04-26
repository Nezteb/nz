#!/bin/bash


if [[ "$#" -eq 1 ]];
    then SSHPORT=$1; echo "SSHPORT=$1;";
    else SSHPORT=22; echo "SHPORT=22;";
fi

if [[ "$#" -eq 2 ]];
    then SSHPORT=$1; WEBPORT=$2; echo "SSHPORT=$1; WEBPORT=$2;";
    else SSHPORT=22; WEBPORT=80; echo "SHPORT=22; WEBPORT=80;";
fi

apt-get update -y
apt-get install -y build-essential
apt-get install -y linux-headers-`uname -r`
apt-get install -y g++ gcc git
apt-get install -y python python-pip python-dev

pip install --upgrade pip
pip install locustio

apt-get install -y fail2ban ufw

# sudo ufw --force disable
# sudo ufw --force reset
# sudo ufw logging on
# sudo ufw default deny incoming
# sudo ufw default deny outgoing
# sudo ufw allow out proto udp from any to 8.8.8.8 port 53
# sudo ufw allow out proto udp from any to any port 67
# sudo ufw allow in proto udp from any to any port 68
# sudo ufw allow in proto tcp from any to any port $WEBPORT
# sudo ufw allow in proto tcp from any to any port $SSHPORT
# sudo ufw --force enable

echo -e "\n" >> /etc/ssh/sshd_config
echo "Port $SSHPORT" >> /etc/ssh/sshd_config
echo "PermitRootLogin without-password" >> /etc/ssh/sshd_config
echo "PasswordAuthentication no" >> /etc/ssh/sshd_config
service ssh restart
