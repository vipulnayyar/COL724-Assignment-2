#!/usr/bin/python3

from mininet.net import Mininet
from mininet.node import RemoteController, OVSKernelSwitch
from mininet.link import TCLink
from mininet.topo import Topo
import time
import csv

class CustomTopo(Topo):
    def build(self):
        # Switches
        s1 = self.addSwitch('s1', protocols='OpenFlow13')
        s2 = self.addSwitch('s2', protocols='OpenFlow13')
        s3 = self.addSwitch('s3', protocols='OpenFlow13')
        s4 = self.addSwitch('s4', protocols='OpenFlow13')
        s5 = self.addSwitch('s5', protocols='OpenFlow13')
        s6 = self.addSwitch('s6', protocols='OpenFlow13')
        s7 = self.addSwitch('s7', protocols='OpenFlow13')
        s8 = self.addSwitch('s8', protocols='OpenFlow13')

        # Hosts
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        h3 = self.addHost('h3')
        h4 = self.addHost('h4')
        h5 = self.addHost('h5')
        h6 = self.addHost('h6')
        h7 = self.addHost('h7')
        h8 = self.addHost('h8')

        # Host to switch links
        self.addLink(h1, s1)
        self.addLink(h2, s2)
        self.addLink(h3, s3)
        self.addLink(h4, s4)
        self.addLink(h5, s5)
        self.addLink(h6, s6)
        self.addLink(h7, s7)
        self.addLink(h8, s8)

        # Switch to switch links with bw and delay
        self.addLink(s1, s2, bw=1.5, delay='20ms', cls=TCLink)
        self.addLink(s2, s3, bw=2, delay='20ms', cls=TCLink)
        self.addLink(s3, s4, bw=1, delay='15ms', cls=TCLink)
        self.addLink(s4, s8, bw=1.5, delay='20ms', cls=TCLink)
        self.addLink(s1, s5, bw=2, delay='10ms', cls=TCLink)
        self.addLink(s5, s6, bw=1.5, delay='15ms', cls=TCLink)
        self.addLink(s6, s7, bw=2, delay='10ms', cls=TCLink)
        self.addLink(s7, s8, bw=2, delay='20ms', cls=TCLink)

def run():
    topo = CustomTopo()
    c0 = RemoteController('c0', ip='127.0.0.1', port=7000)
    net = Mininet(topo=topo, link=TCLink, switch=OVSKernelSwitch, controller=c0)
    net.start()

    print("*** Testing connectivity ***")
    net.pingAll()

    hosts = {f'h{i}': net.get(f'h{i}') for i in range(1, 9)}
    switches = {f's{i}': net.get(f's{i}') for i in range(1, 9)}

    # Start iperf servers on all hosts
    iperf_servers = [h.popen('iperf -s -p 5001 &') for h in hosts.values()]

    # Define links to congest (path 1)
    links_to_congest = [
        ('h1', 'h2', 's1', 's2'),
        ('h2', 'h3', 's2', 's3'),
        ('h3', 'h4', 's3', 's4'),
        ('h4', 'h8', 's4', 's8')
    ]

    # Initialize congestion log
    congestion_log = []

    # --- Congest all links of path 1 together ---
    print("\n*** Congesting entire path 1 together ***")
    for level in [5, 8, 12]:
        start_time = time.time()
        print(f"\n*** Congestion level: {level} streams per link ***")
        
        procs = []
        # Start iperf clients for all links in parallel
        for src_name, dst_name, _, _ in links_to_congest:
            src = hosts[src_name]
            dst = hosts[dst_name]
            cmd = f'iperf -c {dst.IP()} -t 5 -P {level} &'
            print(f"Running: {src_name} -> {dst_name}: {cmd}")
            procs.append(src.popen(cmd))

        # Wait for all iperf processes to finish
        for p in procs:
            p.wait()

        end_time = time.time()

        # Log congestion event for all links
        for src_name, dst_name, _, _ in links_to_congest:
            congestion_log.append({
                'link': f"{src_name}-{dst_name}",
                'level': level,
                'start': start_time,
                'end': end_time
            })

        # Optional: short baseline after each level
        for src_name, dst_name, _, _ in links_to_congest:
            src = hosts[src_name]
            dst = hosts[dst_name]
            baseline = src.cmd(f'iperf -c {dst.IP()} -t 2 -P 1')
            print(f"Baseline for {src_name}->{dst_name}:")
            print(baseline)

        time.sleep(2)

    # Stop iperf servers
    for server in iperf_servers:
        server.terminate()

    # Save congestion log to CSV
    with open('congestion_log.csv', 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['link','level','start','end'])
        writer.writeheader()
        writer.writerows(congestion_log)

    print("*** Stopping network ***")
    net.stop()

if __name__ == '__main__':
    run()

