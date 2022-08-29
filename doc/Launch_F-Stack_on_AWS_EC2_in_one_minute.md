# Launch F-Stack on AWS EC2 in one minute

  If you have a Redhat7.3 EC2 instance，and then execute the following cmds, you will get the F-Stack server in one minute 

    sudo -i
    yum install -y git gcc openssl-devel kernel-devel-$(uname -r) bc numactl-devel mkdir make net-tools vim pciutils iproute pcre-devel zlib-devel elfutils-libelf-devel meson

    git clone https://github.com/F-Stack/f-stack.git f-stack-dev

    # Compile DPDK
    cd f-stack-dev/dpdk
    meson -Denable_kmods=true build
    ninja -C build
    sudo ninja -C build install
      --- Installing subdir /home/archy/f-stack-dev/dpdk/examples to /usr/local/share/dpdk/examples
      --- all librte_*.a and .so libs to  /usr/local/lib64 & /usr/local/lib64/dpdk
      --- a bunch of dpdk-* to  /usr/local/bin
      --- Installing kernel/linux/kni/rte_kni.ko to /lib/modules/5.10.130-118.517.amzn2.x86_64/extra/dpdk
      --- Installing kernel/linux/igb_uio/igb_uio.ko to /lib/modules/5.10.130-118.517.amzn2.x86_64/extra/dpdk
      --- include files to /usr/local/include & /usr/local/include/generic
      --- cmake package file libdpdk-libs.pc & libdpdk.pc to /usr/local/lib64/pkgconfig
      --- a lot of .so files to /usr/local/lib64/librte*.so and /usr/local/lib64/dpdk/*
      
      
       
    # set hugepage	--- AWS has it already with 4096 in  /dev/hugepages/
    # echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
    # mkdir /mnt/huge
    # mount -t hugetlbfs nodev /mnt/huge

    # close ASLR; it is necessary in multiple process
    echo 0 > /proc/sys/kernel/randomize_va_space

    # insmod ko
    sudo modprobe uio
    sudo modprobe hwmon
    sudo insmod build/kernel/linux/igb_uio/igb_uio.ko
    sudo insmod build/kernel/linux/kni/rte_kni.ko carrier=on
    # if old version is installed, like the ones from master build, sudo rmmod igb_uio.ko, sudo rmmod rte_kni.ko

    # set ip address
    
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


    #redhat7.3 =========== use eth1 instead of eth0, otherwise, the machine will not accessable once "sudo ifconfig eth0 down"
    export myaddr=`ifconfig eth1 | grep "inet" | grep -v ":" | awk -F ' '  '{print $2}'`
    export mymask=`ifconfig eth1 | grep "netmask" | awk -F ' ' '{print $4}'`
    export mybc=`ifconfig eth1 | grep "broadcast" | awk -F ' ' '{print $6}'`
    export myhw=`ifconfig eth1 | grep "ether" | awk -F ' ' '{print $2}'`
    export mygw=`route -n | grep 0.0.0.0 | grep eth0 | grep UG | awk -F ' ' '{print $2}'`
    cd ~/f-stack-dev
    sed "s/addr=192.168.1.2/addr=${myaddr}/" -i config.ini
    sed "s/netmask=255.255.255.0/netmask=${mymask}/" -i config.ini
    sed "s/broadcast=192.168.1.255/broadcast=${mybc}/" -i config.ini
    sed "s/gateway=192.168.1.1/gateway=${mygw}/" -i config.ini
    sed "s/pkt_tx_delay=100/pkt_tx_delay=0/" -i config.ini

    # enable kni ---- skip it, as it's using eth1?
    sed "s/#\[kni\]/\[kni\]/" -i config.ini
    sed "s/#enable=1/enable=1/" -i config.ini
    sed "s/#method=reject/method=reject/" -i config.ini
    sed "s/#tcp_port=80/tcp_port=80/" -i config.ini
    sed "s/#vlanstrip=1/vlanstrip=1/" -i config.ini

    # Upgrade pkg-config while version < 0.28
    cd ~
    wget https://pkg-config.freedesktop.org/releases/pkg-config-0.29.2.tar.gz
    tar xzvf pkg-config-0.29.2.tar.gz
    cd pkg-config-0.29.2
    ./configure --with-internal-glib
    make
    make install
    mv /usr/bin/pkg-config /usr/bin/pkg-config.bak
    ln -s /usr/local/bin/pkg-config /usr/bin/pkg-config

    # Compile F-Stack lib
    export FF_PATH=/home/archy/f-stack-dev
    export FF_DPDK=$FF_PATH/dpdk/build
    export PKG_CONFIG_PATH=/usr/lib64/pkgconfig:/usr/local/lib64/pkgconfig:/usr/lib/pkgconfig
    cd ~/f-stack-dev/lib
    make
    sudo make install  ------ include and lib are in /usr/local/include and /usr/local/lib  
                               /usr/local/bin/ff_start
                                cp -f /home/archy/f-stack/lib/../config.ini /etc/f-stack.conf  --- customized config.ini file using eth1

    # Compile Nginx
    cd ../app/nginx-1.16.1
    ./configure --prefix=/usr/local/nginx_fstack --with-ff_module
    make
    make install   ============> installed to /usr/local/nginx_fstack

    # offload NIC（if there is only one NIC，the follow commands must run in a script）
    (base) [archy@TA-TKY-C-07 dpdk]$ ~/f-stack-dev/dpdk/usertools/dpdk-devbind.py -s

    Network devices using kernel driver
    ===================================
    0000:00:05.0 'Elastic Network Adapter (ENA) ec20' if=eth0 drv=ena unused=igb_uio *Active*
    0000:00:06.0 'Elastic Network Adapter (ENA) ec20' if=eth1 drv=ena unused=igb_uio *Active*

        sudo ifconfig eth1 down
    (base) [archy@TA-TKY-C-07 dpdk]$ ~/f-stack-dev/dpdk/usertools/dpdk-devbind.py -s

    Network devices using kernel driver
    ===================================
    0000:00:05.0 'Elastic Network Adapter (ENA) ec20' if=eth0 drv=ena unused=igb_uio *Active*
    0000:00:06.0 'Elastic Network Adapter (ENA) ec20' if=eth1 drv=ena unused=igb_uio

        sudo ~/f-stack-dev/dpdk/usertools/dpdk-devbind.py --bind=igb_uio eth1
        or use 0000:00:06.0 if eth1 doesn't work anymore
    (base) [archy@TA-TKY-C-07 dpdk]$ usertools/dpdk-devbind.py -s

    Network devices using DPDK-compatible driver
    ============================================
    0000:00:06.0 'Elastic Network Adapter (ENA) ec20' drv=igb_uio unused=ena            ============ no more if on eth1? using igb_uio driver now.

    Network devices using kernel driver
    ===================================
    0000:00:05.0 'Elastic Network Adapter (ENA) ec20' if=eth0 drv=ena unused=igb_uio *Active*

    (base) [archy@TA-TKY-C-07 dpdk]$ ifconfig 
    eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 9001
            inet 10.50.12.133  netmask 255.255.255.0  broadcast 10.50.12.255
            ether 06:ee:25:e5:e1:2b  txqueuelen 1000  (Ethernet)
            RX packets 2006  bytes 330758 (323.0 KiB)
            RX errors 0  dropped 0  overruns 0  frame 0
            TX packets 1844  bytes 441048 (430.7 KiB)
            TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

    lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
            inet 127.0.0.1  netmask 255.0.0.0
            loop  txqueuelen 1000  (Local Loopback)
            RX packets 350  bytes 53732 (52.4 KiB)
            RX errors 0  dropped 0  overruns 0  frame 0
            TX packets 350  bytes 53732 (52.4 KiB)
            TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0


    # copy config.ini to $NGX_PREFIX/conf/f-stack.conf
    sudo cp ~/f-stack-dev/config.ini /usr/local/nginx_fstack/conf/f-stack.conf

    # starting test program:
    sudo ./example/helloworld --conf config.ini --proc-type=primary --proc-id=0
    #test it on another box, or in browser: http://10.50.12.75/
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

    # start Nginx
    sudo /usr/local/nginx_fstack/sbin/nginx

    # start kni  ---- skip it, as it's using eth1?
    sleep 10
    sudo ifconfig veth1 ${myaddr}  netmask ${mymask}  broadcast ${mybc} hw ether ${myhw} -------> SIOCSIFHWADDR: Operation not supported
    sudo route add -net 0.0.0.0 gw ${mygw} dev veth1 -------> Timeout, server ta-tky-c-07 not responding. then machine dead, can't be login again
    sudo echo 1 > /sys/class/net/veth1/carrier # if `carrier=on` not set while `insmod rte_kni.ko`.
