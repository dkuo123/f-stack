# Launch F-Stack on AWS EC2 in one minute

  If you have a Redhat7.3 EC2 instance，and then execute the following cmds, you will get the F-Stack server in one minute 

    #sudo -i
    #sudo su -
    yum install -y git gcc openssl-devel kernel-devel-$(uname -r) bc numactl-devel mkdir make net-tools vim pciutils iproute pcre-devel zlib-devel elfutils-libelf-devel vim

    git clone https://github.com/F-Stack/f-stack.git

    # Compile DPDK
    cd ~/f-stack
    git checkout master
    cd dpdk
    make config T=x86_64-native-linuxapp-gcc
    make

    # set hugepage, skip as my AWS already has /dev/hugepages with 4096 pages.
    echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
    mkdir /mnt/huge
    mount -t hugetlbfs nodev /mnt/huge

    # close ASLR; it is necessary in multiple process
    echo 0 > /proc/sys/kernel/randomize_va_space

    # insmod ko
    sudo modprobe uio
    sudo modprobe hwmon
    sudo insmod build/kmod/igb_uio.ko
    sudo insmod build/kmod/rte_kni.ko carrier=on

    # set ip address
    #redhat7.3
    export myaddr=`ifconfig eth1 | grep "inet" | grep -v ":" | awk -F ' '  '{print $2}'`
    export mymask=`ifconfig eth1 | grep "netmask" | awk -F ' ' '{print $4}'`
    export mybc=`ifconfig eth1 | grep "broadcast" | awk -F ' ' '{print $6}'`
    export myhw=`ifconfig eth1 | grep "ether" | awk -F ' ' '{print $2}'`
    export mygw=`route -n | grep 0.0.0.0 | grep eth1 | grep UG | awk -F ' ' '{print $2}'`
    
      ==== doesn't have inet addr  #Amazon Linux AMI 2017.03
      #export myaddr=`ifconfig eth1 | grep "inet addr" | awk -F ' '  '{print $2}' |  awk -F ':' '{print $2}'`
      #export mymask=`ifconfig eth1 | grep "Mask" | awk -F ' ' '{print $4}' |  awk -F ':' '{print $2}'`
      #export mybc=`ifconfig eth1 | grep "Bcast" | awk -F ' ' '{print $3}' |  awk -F ':' '{print $2}'`
      #export myhw=`ifconfig eth1 | grep "HWaddr" | awk -F ' ' '{print $5}'`
      #export mygw=`route -n | grep 0.0.0.0 | grep eth1 | grep UG | awk -F ' ' '{print $2}'

    sed "s/addr=192.168.1.2/addr=${myaddr}/" -i ~/f-stack/config.ini
    sed "s/netmask=255.255.255.0/netmask=${mymask}/" -i ~/f-stack/config.ini
    sed "s/broadcast=192.168.1.255/broadcast=${mybc}/" -i ~/f-stack/config.ini
    sed "s/gateway=192.168.1.1/gateway=${mygw}/" -i ~/config.ini

    ====== use eth1 instead, leave eth0 for other normal app, use sudo reboot to recover all the machine settings from AWS.
    [root@TA-TKY-C-07 f-stack]# ifconfig
    eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 9001
            inet 10.50.12.133  netmask 255.255.255.0  broadcast 10.50.12.255
            ether 06:ee:25:e5:e1:2b  txqueuelen 1000  (Ethernet)
            RX packets 2031848  bytes 2280930590 (2.1 GiB)
            RX errors 0  dropped 0  overruns 0  frame 0
            TX packets 1004053  bytes 334497678 (319.0 MiB)
            TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

    eth1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 9001
            inet 10.50.12.75  netmask 255.255.255.0  broadcast 10.50.12.255
            ether 06:0c:8b:7c:8c:e1  txqueuelen 1000  (Ethernet)
            RX packets 2947  bytes 138960 (135.7 KiB)
            RX errors 0  dropped 0  overruns 0  frame 0
            TX packets 103  bytes 35226 (34.4 KiB)
            TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
     ip addr:
        3: eth1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 9001 qdisc mq state UP group default qlen 1000
            link/ether 06:0c:8b:7c:8c:e1 brd ff:ff:ff:ff:ff:ff
            inet 10.50.12.75/24 brd 10.50.12.255 scope global dynamic eth1
            valid_lft 2865sec preferred_lft 2865sec

    so replace it as:
    export myaddr=10.50.12.75
    export mymask=255.255.255.0
    export mybc=10.50.12.255
    export myhw=06:0c:8b:7c:8c:e1
    export mygw=`route -n | grep 0.0.0.0 | grep eth1 | grep UG | awk -F ' ' '{print $2}'`    ======== same as eth0, it's 10.50.12.1
    
      # enable kni === don't activate kni because dual NIC cards and I'm using eth1
      sed "s/#\[kni\]/\[kni\]/" -i /data/f-stack/config.ini
      sed "s/#enable=1/enable=1/" -i /data/f-stack/config.ini
      sed "s/#method=reject/method=reject/" -i /data/f-stack/config.ini
      sed "s/#tcp_port=80/tcp_port=80/" -i /data/f-stack/config.ini
      sed "s/#vlanstrip=1/vlanstrip=1/" -i /data/f-stack/config.ini

    # Compile F-Stack lib
    export FF_PATH=~/f-stack
    export FF_DPDK=~/f-stack/dpdk/build
    cd ~/f-stack/lib
    make
    # have to hack fixing some compile issue when using gcc10
    ../../freebsd/kern/kern_linker.c, add #pragma GCC diagnostic ignored "-Wformat-overflow"
    ../../freebsd/libkern/zlib.c, add #pragma GCC diagnostic ignored "-Wnonnull"
    sudo make install  ----- install to /usr/local/ include and lib directory. /etc/f-stack.conf = config.ini
    
    # Compile Nginx
    cd ../app/nginx-1.16.1
    ./configure --prefix=/usr/local/nginx_fstack --with-ff_module
    make
    sudo make install  # to directory /usr/local/nginx_fstack

    # offload NIC（if there is only one NIC，the follow commands must run in a script）
    sudo ifconfig eth1 down
    sudo ~/f-stack/dpdk/usertools/dpdk-devbind.py --bind=igb_uio eth1

    ~/f-stack/dpdk/usertools/dpdk-devbind.py -s
          Network devices using DPDK-compatible driver
          ============================================
          0000:00:06.0 'Elastic Network Adapter (ENA) ec20' drv=igb_uio unused=ena

          Network devices using kernel driver
          ===================================
          0000:00:05.0 'Elastic Network Adapter (ENA) ec20' if=eth0 drv=ena unused=igb_uio *Active*

    # run test server
     cd example
     sudo ./ff_epoll_server --conf ../config.ini --proc-type=primary --proc-id=0

    # copy config.ini to $NGX_PREFIX/conf/f-stack.conf
    sudo cp ~/f-stack/config.ini /usr/local/nginx_fstack/conf/f-stack.conf
    # test setup using hello world web app:
    $ sudo ./example/helloworld --conf config.ini --proc-type=primary --proc-id=0
    # on another box, do:
      [dev-guo](~ )$  curl --request GET      --url http://10.50.12.75:80
      <!DOCTYPE html>
      <html>
      <head>
      <title>Welcome to F-Stack!</title>
      <style>
          body {
              width: 35em;
              margin: 0 auto;
              font-family: Tahoma, Verdana, Arial, sans-serif;
          }
      </style>
      </head>
      <body>
      <h1>Welcome to F-Stack!</h1>

      <p>For online documentation and support please refer to
      <a href="http://F-Stack.org/">F-Stack.org</a>.<br/>

      <p><em>Thank you for using F-Stack.</em></p>
      </body>
      </html>
     
    # ==== skip the following two steps
    # start Nginx
    sudo /usr/local/nginx_fstack/sbin/nginx

    # start kni --- skip it due to using eth1
    sleep 10
    ifconfig veth0 ${myaddr}  netmask ${mymask}  broadcast ${mybc} hw ether ${myhw}
    route add -net 0.0.0.0 gw ${mygw} dev veth0
    echo 1 > /sys/class/net/veth0/carrier # if `carrier=on` not set while `insmod rte_kni.ko`.
