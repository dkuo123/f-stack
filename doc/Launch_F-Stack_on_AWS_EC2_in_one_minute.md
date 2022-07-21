# Launch F-Stack on AWS EC2 in one minute

  If you have a Redhat7.3 EC2 instance，and then execute the following cmds, you will get the F-Stack server in one minute 

    sudo -i
    yum install -y git gcc openssl-devel kernel-devel-$(uname -r) bc numactl-devel mkdir make net-tools vim pciutils iproute pcre-devel zlib-devel elfutils-libelf-devel meson

    mkdir /data/f-stack
    git clone https://github.com/F-Stack/f-stack.git /data/f-stack

    # Compile DPDK
    cd /data/f-stack/dpdk
    meson -Denable_kmods=true build
    ninja -C build
    ninja -C build install

    # set hugepage	
    echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
    mkdir /mnt/huge
    mount -t hugetlbfs nodev /mnt/huge

    # close ASLR; it is necessary in multiple process
    echo 0 > /proc/sys/kernel/randomize_va_space

    # insmod ko
    sudo modprobe uio
    sudo modprobe hwmon
    sudo insmod build/kernel/linux/igb_uio/igb_uio.ko
    sudo insmod build/kernel/linux/kni/rte_kni.ko carrier=on

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
          3: eth1: <BROADCAST,MULTICAST> mtu 9001 qdisc mq state DOWN group default qlen 1000
              link/ether 06:0c:8b:7c:8c:e1 brd ff:ff:ff:ff:ff:ff
              inet 10.50.12.75/24 brd 10.50.12.255 scope global dynamic eth1
                 valid_lft 582sec preferred_lft 582sec
        
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
            #Amazon Linux AMI 2017.03
            #export myaddr=`ifconfig eth0 | grep "inet addr" | awk -F ' '  '{print $2}' |  awk -F ':' '{print $2}'`
            #export mymask=`ifconfig eth0 | grep "Mask" | awk -F ' ' '{print $4}' |  awk -F ':' '{print $2}'`
            #export mybc=`ifconfig eth0 | grep "Bcast" | awk -F ' ' '{print $3}' |  awk -F ':' '{print $2}'`
            #export myhw=`ifconfig eth0 | grep "HWaddr" | awk -F ' ' '{print $5}'`
            #export mygw=`route -n | grep 0.0.0.0 | grep eth0 | grep UG | awk -F ' ' '{print $2}'`

    sed "s/addr=192.168.1.2/addr=${myaddr}/" -i /data/f-stack/config.ini
    sed "s/netmask=255.255.255.0/netmask=${mymask}/" -i /data/f-stack/config.ini
    sed "s/broadcast=192.168.1.255/broadcast=${mybc}/" -i /data/f-stack/config.ini
    sed "s/gateway=192.168.1.1/gateway=${mygw}/" -i /data/f-stack/config.ini

    # enable kni
    sed "s/#\[kni\]/\[kni\]/" -i /data/f-stack/config.ini
    sed "s/#enable=1/enable=1/" -i /data/f-stack/config.ini
    sed "s/#method=reject/method=reject/" -i /data/f-stack/config.ini
    sed "s/#tcp_port=80/tcp_port=80/" -i /data/f-stack/config.ini
    sed "s/#vlanstrip=1/vlanstrip=1/" -i /data/f-stack/config.ini

    # Upgrade pkg-config while version < 0.28
    cd /data/
    wget https://pkg-config.freedesktop.org/releases/pkg-config-0.29.2.tar.gz
    tar xzvf pkg-config-0.29.2.tar.gz
    cd pkg-config-0.29.2
    ./configure --with-internal-glib
    make
    make install
    mv /usr/bin/pkg-config /usr/bin/pkg-config.bak
    ln -s /usr/local/bin/pkg-config /usr/bin/pkg-config

    # Compile F-Stack lib
    export FF_PATH=/home/archy/f-stack
    export PKG_CONFIG_PATH=/usr/lib64/pkgconfig:/usr/local/lib64/pkgconfig:/usr/lib/pkgconfig
    cd /data/f-stack/lib
    make

    # Compile Nginx
    cd ../app/nginx-1.16.1
    ./configure --prefix=/usr/local/nginx_fstack --with-ff_module
    make
    make install   ============> installed to /usr/local/nginx_fstack

    # offload NIC（if there is only one NIC，the follow commands must run in a script）
    (base) [archy@TA-TKY-C-07 dpdk]$ usertools/dpdk-devbind.py -s

    Network devices using kernel driver
    ===================================
    0000:00:05.0 'Elastic Network Adapter (ENA) ec20' if=eth0 drv=ena unused=igb_uio *Active*
    0000:00:06.0 'Elastic Network Adapter (ENA) ec20' if=eth1 drv=ena unused=igb_uio *Active*

        sudo ifconfig eth1 down
    (base) [archy@TA-TKY-C-07 dpdk]$ usertools/dpdk-devbind.py -s

    Network devices using kernel driver
    ===================================
    0000:00:05.0 'Elastic Network Adapter (ENA) ec20' if=eth0 drv=ena unused=igb_uio *Active*
    0000:00:06.0 'Elastic Network Adapter (ENA) ec20' if=eth1 drv=ena unused=igb_uio

        sudo usertools/dpdk-devbind.py --bind=igb_uio eth1
    (base) [archy@TA-TKY-C-07 dpdk]$ usertools/dpdk-devbind.py -s

    Network devices using DPDK-compatible driver
    ============================================
    0000:00:06.0 'Elastic Network Adapter (ENA) ec20' drv=igb_uio unused=ena            ============ no more if on eth1?

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
    cp /data/f-stack/config.ini /usr/local/nginx_fstack/conf/f-stack.conf

    # start Nginx
    sudo /usr/local/nginx_fstack/sbin/nginx

    # start kni
    sleep 10
    sudo ifconfig veth0 ${myaddr}  netmask ${mymask}  broadcast ${mybc} hw ether ${myhw} -------> SIOCSIFHWADDR: Operation not supported
    sudo route add -net 0.0.0.0 gw ${mygw} dev veth0 -------> Timeout, server ta-tky-c-07 not responding. then machine dead, can't be login again
    sudo echo 1 > /sys/class/net/veth0/carrier # if `carrier=on` not set while `insmod rte_kni.ko`.
