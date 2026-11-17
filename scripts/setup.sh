#!/bin/bash
blue='\e[0;34m'
green='\e[0;32m'
yellow='\e[1;33m'
red='\e[0;31m'
NC='\e[0m' #No Color

echo -e "${blue} STARTING JETSON CONFIGURATION ${NC}"

# Submodules isolados para evitar erro caso o script rode fora do repositório
if git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
    git submodule init
    git submodule update
else
    echo -e "${yellow}Aviso: Executando fora de um repositório Git. Pulando submodules.${NC}"
fi

# Correção da sintaxe do APT (&&)
sudo apt update && sudo apt upgrade -y
sudo apt install python3-pip -y

if [ -z "$(pip show jetson-stats)" ]; then
    sudo pip3 install -U jetson-stats
fi 

if [ -z "$(pip show ultralytics)" ]; then
    pip install ultralytics[export]
    if [ -n "$(pip show ultralytics)" ]; then
        # Reinício automático comentado para não interromper o resto do script
        echo -e "${yellow}Ultralytics instalado. Um reboot será necessário ao final.${NC}"
    fi
fi 

# Repositórios do CUDA 12.4 (Ubuntu 22.04) foram removidos aqui, pois o Jetpack 7 já traz o CUDA nativamente.

sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get install -y python3-pip libjpeg-dev libpng-dev libtiff-dev

# ATENÇÃO: O link do PyTorch v61 (Jetpack 6.1) foi removido. 
# Você precisará instalar o wheel correto do PyTorch para Jetpack 7.2.1 após o script.

pip install "numpy<2.0" 

# Instalação do Torchvision compatível
git clone https://github.com/pytorch/vision.git torchvision
cd torchvision
git checkout v0.20.0
sudo python3 setup.py install
cd ..
# pip uninstall torchvision==0.21.0

pip install onnx2tf==1.26.3
pip install onnx_graphsurgeon
pip install sng4onnx
pip install tflite_support

pip install paho-mqtt
sudo apt install -y mosquitto
sudo apt install -y mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
# sudo vim /etc/mosquitto/mosquitto.conf -> para ouvir o 1883 e dar allow em anonymous_users
crontab -e # para iniciar a inteface com o omnicontroler -> @reboot bash $HOME/TCC/startup.sh > $HOME/log.txt 2>&1

#################################################################
#                       _                                       #
#                      / \   _ __  _ __  ___                    #
#                     / _ \ | '_ \| '_ \/ __|                   #
#                    / ___ \| |_) | |_) \__ \                   #
#                   /_/   \_\ .__/| .__/|___/                   #
#                           |_|   |_|                           #
#################################################################

# Chromium Browser
if [ -z "$(which chromium-browser)" ]; then
    echo -e "${blue}Instaling Chromium Browser${NC}"
    sudo apt install chromium-browser -y
    
    if [ -z "$(which chromium-browser)" ]; then
        echo -e "${green}Chromium Browser Installed Successfully${NC}"
    else
        echo -e "${red}ERROR: Chromium Browser Instalation${NC}"
    fi
else 
    echo -e "${green}Chromium Browser Already Installed${NC}"
fi

# VS Code
if [ -z "$(which code)" ]; then
    echo -e "${blue}Instaling VS Code${NC}"
    wget -O code.deb "https://code.visualstudio.com/sha/download?build=stable&os=linux-deb-arm64"
    sudo dpkg -i code.deb
    sudo apt --fix-broken install -y
    rm -rf code.deb

    if [ -z "$(which code )" ]; then
        echo -e "${red}ERROR: VS Code Instalation${NC}"
    else
        echo -e "${green}VS Code Installed Successfully${NC}"
    fi
else 
    echo -e "${green}VS Code Already Installed${NC}"
fi

#################################################################
#                                _                              #
#                        _______| |__                           #
#                       |_  / __| '_ \                          #
#                        / /\__ \ | | |                         #
#                       /___|___/_| |_|                         #
#                                                               #
#################################################################

# Correção da sintaxe do APT (&&)
sudo apt update && sudo apt upgrade -y
sudo apt install zsh curl -y


# Instalacao da fonte para usar no terminal
if [ -z "$(ls -a ~/.local/share | grep fonts)" ]; then
    echo -e "${blue}Instaling Nerd Fonts${NC}"
    wget -P ~/.local/share/fonts https://github.com/ryanoasis/nerd-fonts/releases/download/v3.0.2/JetBrainsMono.zip \
    && cd ~/.local/share/fonts \
    && unzip JetBrainsMono.zip \
    && rm JetBrainsMono.zip \
    && fc-cache -fv
fi

# Caso já exista não instala o Oh my zsh
if [ -n "$(ls -a ~/ | grep .oh-my-zsh)" ]; then
    echo "OH-MY-ZSH Já está instalado"
else
    echo -e "${blue}Instaling Oh My Zsh${NC}"
    wget https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh
    # Comentar a linha de inicialização automática para não travar o resto da instalação
    sed -i '/exec zsh -l/s/^/#/' install.sh
    sh install.sh
    rm -f install.sh
fi

# Configuração de plugins do zsh
git clone https://github.com/zsh-users/zsh-autosuggestions.git ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions
sudo apt install zoxide -y

# Definindo o zsh como padrão
chsh -s $(which zsh)

#################################################################
#                    ____   ___  ____ ____                      #
#                   |  _ \ / _ \/ ___|___ \                     #
#                   | |_) | | | \___ \ __) |                    #
#                   |  _ <| |_| |___) / __/                     #
#                   |_| \_\\___/|____/_____|                    #
#                                                               #
#################################################################
if [ -z "$(ls -a /opt | grep ros)" ]; then
    echo -e "${blue}Instaling ROS2 Jazzy${NC}"

    # Instalar o ROS Jazzy
    sudo apt install software-properties-common -y
    sudo add-apt-repository universe -y
    sudo apt update && sudo apt install curl -y
    sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null
    
    sudo apt update && sudo apt upgrade -y
    sudo apt install ros-jazzy-desktop -y
    sudo apt install ros-dev-tools -y
    sudo apt install python3-rosdep -y

    # Colcon Setup
    sudo apt install python3-colcon-common-extensions -y

    # Intalar Onshape to Robot
    pip install onshape-to-robot

    # Source
    if [[ "$SHELL" == *"zsh"* ]]; then
        source /opt/ros/jazzy/setup.zsh
    else
        source /opt/ros/jazzy/setup.bash
    fi
    
    # Instalando dependencias do ROS2
    sudo rosdep init
    rosdep update
    rosdep install --from-paths src --ignore-src -r -y

    # Caminho corrigido para o RPLidar de acordo com o workspace CareBot
    source ~/CareBot/software/src/carebot_navigation/rplidar_ros/scripts/create_udev_rules.sh

    echo -e "${green}ROS Installed Successfully${NC}"
fi

#################################################################
#                               _       _                       #
#                 ___  ___ _ __(_)_ __ | |_ ___                 #
#                / __|/ __| '__| | '_ \| __/ __|                #
#                \__ \ (__| |  | | |_) | |_\__ \                #
#                |___/\___|_|  |_| .__/ \__|___/                #
#                                |_|                            #
#################################################################
if ! grep -q 'source ~/CareBot/software/install/setup.bash' ~/.bashrc; then
    echo 'source ~/CareBot/software/install/setup.bash' >> ~/.bashrc
fi

if [ -n "$(ls -a ~/ | grep .zshrc)" ]; then
    if ! grep -q 'source ~/CareBot/software/install/setup.zsh' ~/.zshrc; then
        echo 'source ~/CareBot/software/install/setup.zsh' >> ~/.zshrc
    fi
fi

echo -e "${green} SETUP FINALIZADO. REINICIE A PLACA. ${NC}"