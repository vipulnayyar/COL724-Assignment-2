#!/usr/bin/python3

from mininet.net import Mininet
from mininet.node import RemoteController, OVSKernelSwitch
from mininet.link import TCLink
from mininet.topo import Topo
from mininet.cli import CLI
import time

class CustomTopo(Topo):
    def build(self):
        # Switches
        s1 = self.addSwitch('s1', protocols='OpenFlow13')
        s2 = self.addSwitch('s2', protocols='OpenFlow13')
        s3 = self.addSwitch('s3', protocols='OpenFlow13')
        s4 = self.addSwitch('s4', protocols='OpenFlow13')

        # Hosts
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        h3 = self.addHost('h3')
        h4 = self.addHost('h4')

        # Host to switch links
        self.addLink(h1, s1)
        self.addLink(h2, s2)
        self.addLink(h3, s3)
        self.addLink(h4, s4)

        # Switch to switch links with initial bw and delay
        self.addLink(s1, s2, bw=2, delay='20ms', cls=TCLink)
        self.addLink(s2, s3, bw=2, delay='10ms', cls=TCLink)
        self.addLink(s3, s4, bw=1, delay='20ms', cls=TCLink)
        self.addLink(s4, s1, bw=1, delay='15ms', cls=TCLink)

def run():
    topo = CustomTopo()
    c0 = RemoteController('c0', ip='127.0.0.1', port=7000)
    net = Mininet(topo=topo, link=TCLink, switch=OVSKernelSwitch, controller=c0)
    net.start()

    print("*** Testing connectivity ***")
    net.pingAll()

    hosts = {
        'h1': net.get('h1'),
        'h2': net.get('h2'),
        'h3': net.get('h3'),
        'h4': net.get('h4')
    }

    switches = {
        's1': net.get('s1'),
        's2': net.get('s2'),
        's3': net.get('s3'),
        's4': net.get('s4')
    }

    #hosts = [net.get('h1'), net.get('h2'), net.get('h3'), net.get('h4')]

    # Start iperf servers on all hosts
    iperf_servers = []
    for h in hosts.values():
        iperf_servers.append(h.popen('iperf -s -p 5001 &'))

    #links_to_congest = [
     #   ('h1', 'h2'), ('h2', 'h3'), ('h3', 'h4'), ('h4', 'h1')
    #]

    # Map each host pair to the switch link to congest
    links_to_congest = [
        ('h1', 'h2', 's1', 's2'),
        ('h2', 'h3', 's2', 's3'),
        ('h3', 'h4', 's3', 's4'),
        ('h4', 'h1', 's4', 's1')
    ]

    # Initialize congestion log
    congestion_log = []

    # Dynamic congestion for each link
    for src_name, dst_name, src_sw, dst_sw in links_to_congest:
        src = hosts[src_name]
        dst = hosts[dst_name]

        # Get the link between the switches
        link = net.linksBetween(switches[src_sw], switches[dst_sw])[0]  # <-- use switches
        bw = getattr(link, 'bw', None)
        delay = getattr(link, 'delay', None)

        print(f"\n*** Congesting link {src_name} -> {dst_name} ***")
        for level in [2, 5, 8]:  # Number of parallel iperf streams
            start_time = time.time()
            print(f"  - Congestion level: {level} streams")
            cmd = f'iperf -c {dst.IP()} -t 5 -P {level}'
            print(f"Running: {cmd}")
            result = src.cmd(cmd)
            print(result)
            end_time = time.time()

            # Log the congestion event
            congestion_log.append({
                'link': f"{src_name}-{dst_name}",
                'level': level,
                'start': start_time,
                'end': end_time
            })

            time.sleep(1)

        print(f"*** Finished congestion on {src_name} -> {dst_name}, restoring normal traffic ***")
        # Optional: short baseline throughput after congestion
        baseline = src.cmd(f'iperf -c {dst.IP()} -t 5 -P 1')
        print("Baseline after congestion:")
        print(baseline)
        time.sleep(2)

    # Stop iperf servers
    for server in iperf_servers:
        server.terminate()


    # Save congestion log to CSV
    import csv
    with open('congestion_log.csv', 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['link','level','start','end'])
        writer.writeheader()
        writer.writerows(congestion_log)

    print("*** Stopping network ***")
    net.stop()

if __name__ == '__main__':
    run()
