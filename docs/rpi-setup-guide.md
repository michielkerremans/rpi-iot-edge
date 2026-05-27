# Raspberry Pi Setup Guide

## Installation

### Raspberry Pi Imager

Install the imager.

```powershell
winget install raspberryPiFoundation.RaspberryPiImager
```

Run the imager.

- Select your Raspberry Pi device : `Raspberry Pi 4/400 (4GB/8GB)`
- Choose operating system : `Raspberry Pi OS (other)`
- Choose operating system (version) : `Raspberry Pi OS Full (64-bit)`
- Select your storage device : `Generic-SD/MMC USB Device`
- Customisation: Choose hostname: `HOSTNAME`
- Customisation: Localisation: `LOCALE`
- Customisation: Choose username:
  - Username: `USERNAME`
  - Password: `PASSWORD`
- Customisation: Choose Wi-Fi:
  - SSID: `SSID`
  - Password: `WIFI_PASSWORD`
- Customisation: SSH authentication:
  - Enable SSH: [x]
  - Authentication mechanism: `Use password authentication`
- Customisation: Raspberry Pi Connect:
  - Enable Raspberry Pi Connect: [x]
- Write image: Click "Write" and wait for the process to complete.

### Connect to Raspberry Pi

Go to Network & internet > Advanced network settings > More adapter options.
Go to Wi-Fi Properties > Sharing:
  - Allow other network users to connect through this computer's Internet connection: [x]
  - Home networking connection: `Ethernet`

Find the IP address of your Ethernet adapter.

```powershell
ipconfig /all
```

Find the IP address of your Raspberry Pi.

```powershell
arp -a
```

Use SSH to connect to your Raspberry Pi.

```powershell
ssh USERNAME@RASPBERRY_PI_IP_ADDRESS
```

### Raspi-Config

```bash
sudo raspi-config
```

- System Options > Configure system settings (optional)
- Interface Options > Enable SSH
- Interface Options > Enable VNC
- Advanced Options > Expand Filesystem
- System Options > Boot > Desktop GUI


### Update and Upgrade

```bash
sudo apt update
sudo apt full-upgrade
```

### Configuration files

On the SD card:
- `user-data` : Cloud-init configuration file for initial setup.
- `network-config` : Netplan configuration file for network settings.

## SSH key based authentication

Generate the SSH key pair on your local machine.

```powershell
cd $env:USERPROFILE\.ssh
ssh-keygen -f pi
```

Copy the public key to the Raspberry Pi.

```powershell
scp .\pi.pub USERNAME@RASPBERRY_PI_IP_ADDRESS:~/.ssh/authorized_keys
```

Create config file for SSH on your local machine.

```txt
Host HOSTNAME
    HostName HOSTNAME.local
    User USERNAME
    ForwardAgent yes
    IdentityFile ~/.ssh/pi
```

## Github access

Generate a new SSH key pair on the Raspberry Pi.

```bash
ssh-keygen -t ed25519 -f ~/.ssh/pi-github
```

Go to GitHub > Settings > SSH and GPG keys > New SSH key, and add the contents of `~/.ssh/pi-github.pub` as a new SSH key.

```bash
vim ~/.ssh/config
```
```txt
Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/pi-github
    IdentitiesOnly yes
```

Set the permissions for the SSH config file.

```bash
sudo chmod go-r ~/.ssh/*
```

Test the SSH connection to GitHub.

```bash
ssh -vT git@github.com
```

Syntax for cloning a repository using SSH.

```bash
git clone "git@github.com:user/repository.git"
```

## Visual Studio Code

- Install the *Remote Development* extension in Visual Studio Code.
- Click on the green "><" icon in the bottom left corner of Visual Studio Code.
- Select "Remote-SSH: Connect to Host...".
- Select "Add New SSH Host...".
- Enter the SSH connection details for the host.

```powershell
ssh USERNAME@HOSTNAME.local -A
```

And if there are SSH connection problems:

```bash
sudo sed -i 's/#MaxSessions 10/MaxSessions 128/' /etc/ssh/sshd_config
sudo systemctl reload sshd.service
```

[Multiple authentication check in VS Code workspaces · Issue #193 · maxgoedjen/secretive (github.com)](https://github.com/maxgoedjen/secretive/issues/193#issuecomment-813411351)

## .NET Core

Install the .NET SDK on the Raspberry Pi.

```bash
wget https://dot.net/v1/dotnet-install.sh -O dotnet-install.sh
chmod +x ./dotnet-install.sh
./dotnet-install.sh --channel 8.0 # 8.0: Long Term Support
echo 'export DOTNET_ROOT=$HOME/.dotnet' >> ~/.bashrc
echo 'export PATH=$PATH:$DOTNET_ROOT:$DOTNET_ROOT/tools' >> ~/.bashrc
```

Create a new .NET console application.

```bash
dotnet new console
dotnet add package iot.device.bindings
```

Install the *C# Dev Kit* extension in Visual Studio Code for debugging and development.

Open the project in Visual Studio Code, wait until the project is fully loaded, and start debugging:
- Option 1: Click on the "Run and Debug" icon in the left sidebar and select C# and the launch configuration.
- Option 2: Right-click on the project file in the Explorer and Debug > Step into New Instance.

## VNC

Enable the VNC server on the Raspberry Pi.

```bash
sudo raspi-config
```

Go to Interface Options > VNC > Enable.
Would you like the VNC Server to be enabled? [Yes]

Install *RealVNC Viewer* on your local machine.

```powershell
winget install RealVNC.VNCViewer -i
```

Open *RealVNC Viewer* and add a new connection with the IP address of your Raspberry Pi.

To increase the screen resolution, go to Preferences > Screen Configuration > Layout > Screens > ... > Resolution.

## mDNS

- mDNS allows you to access your Raspberry Pi using a hostname instead of an IP address.
- mDNS works via **UDP port 5353**.
- Make sure that this port is open on your local machine with Windows Defender Firewall!

---
