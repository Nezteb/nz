#!/bin/bash
# Noah Betzen
# setup script for my nz project on Ubuntu 14.04 servers

SSHPORT=""
read -p "SSH port? [default: 22] " SSHPORT
if [[ -z "$SSHPORT" ]];
then
    SSHPORT="22"
fi

WEBPORT=""
read -p "Web port? [default: 80] " WEBPORT
if [[ -z "$WEBPORT" ]];
then
    WEBPORT="80"
fi

ISPROXY=""
read -p "Is this server the proxy? (y/n) [default: y] " ISPROXY
if [[ -z "$ISPROXY" ]];
then
    ISPROXY="y"
fi

if [[ "$ISPROXY" = "y" ]];
then
    SERVERS=()
    INPUT=""
    while read -p "Enter web server (blank when done) -> " INPUT; [[ -n "$INPUT" ]];
    do
            SERVERS+=($INPUT)
    done
else # is server, not proxy
    read -p "What is the proxy server's IP? " PROXYIP
fi

LOCAL=127.0.0.1 # you probably shouldn't to change this
MYIP=`/sbin/ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'` # bash magic to get your ip

# install dependencies
apt-get update -y
apt-get install -y build-essential
apt-get install -y linux-headers-`uname -r`
apt-get install -y g++ gcc git
apt-get install -y python python-pip python-dev
pip install --upgrade pip
pip install locustio
pip install beautifulsoup4

# set up fail2ban and ufw
apt-get install -y fail2ban ufw
ufw --force disable
ufw --force reset
ufw logging on
ufw default deny incoming
ufw default deny outgoing
ufw allow out proto udp from any to 8.8.8.8 port 53 # dns out
ufw allow out proto udp from any to any port 67 # dhcp out
ufw allow in proto udp from any to any port 68 # dhcp in
ufw allow in proto tcp from any to any port $SSHPORT # ssh in

if [[ "$ISPROXY" = "y" ]];
then
    ufw allow in proto tcp from any to any port $WEBPORT # web in

    for SERVER in "${SERVERS[@]}"; do
        ufw allow out proto tcp from any to $SERVER port $WEBPORT # web out
    done
else # is server, not proxy
    ufw allow in proto tcp from $PROXYIP to any port $WEBPORT # web in
fi

# disable IPv6
IPV6=no
echo -e "\n" >> /etc/default/ufw
echo "IPV6=no" >> /etc/default/ufw

ufw --force enable
ufw status numbered

# reconfigure ssh
echo -e "\n" >> /etc/ssh/sshd_config
echo "Port $SSHPORT" >> /etc/ssh/sshd_config
echo "PermitRootLogin without-password" >> /etc/ssh/sshd_config
echo "PasswordAuthentication no" >> /etc/ssh/sshd_config
service ssh restart
