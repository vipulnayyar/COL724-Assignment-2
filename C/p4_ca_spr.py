# p4_ca_spr_level6.py
from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import MAIN_DISPATCHER, DEAD_DISPATCHER, set_ev_cls
from ryu.topology import event
from ryu.ofproto import ofproto_v1_3
from ryu.lib.packet import packet, ethernet, ether_types
from ryu.lib import hub
import networkx as nx
import subprocess
import csv
import time
import os


class CongestionAwareRouting(app_manager.RyuApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(CongestionAwareRouting, self).__init__(*args, **kwargs)
        self.datapaths = {}
        self.net = nx.Graph()          # Network graph
        self.prev_link_load = {}       # (src, dst) -> tx_bytes
        self.shortest_paths = {}       # dpid -> {dpid -> [path]}
        self.hosts = {}                # MAC -> dpid
        self.monitor_thread = hub.spawn(self._monitor)
        self.logger.info("Congestion-Aware Routing app initialized")
        self.log_file = "link_metrics.csv"
        if not os.path.exists(self.log_file):
            with open(self.log_file, "w", newline="") as f:
                writer = csv.writer(f)
                writer.writerow(["time", "src", "dst", "throughput_mbps", "utilization", "weight", "delay"])


    # ---------------- Datapath register/unregister ----------------
    @set_ev_cls(ofp_event.EventOFPStateChange, [MAIN_DISPATCHER, DEAD_DISPATCHER])
    def _state_change_handler(self, ev):
        dp = ev.datapath
        if ev.state == MAIN_DISPATCHER:
            if dp.id not in self.datapaths:
                self.datapaths[dp.id] = dp
                self.net.add_node(dp.id)
                self._add_table_miss_flow(dp)
                self.logger.info(f"Datapath registered: {dp.id}")
        elif ev.state == DEAD_DISPATCHER:
            if dp.id in self.datapaths:
                del self.datapaths[dp.id]
                if self.net.has_node(dp.id):
                    self.net.remove_node(dp.id)
                self.logger.info(f"Datapath unregistered: {dp.id}")

    # ---------------- Table-miss flow ----------------
    def _add_table_miss_flow(self, datapath):
        parser = datapath.ofproto_parser
        ofproto = datapath.ofproto
        match = parser.OFPMatch()
        actions = [parser.OFPActionOutput(ofproto.OFPP_FLOOD)]
        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS, actions)]
        mod = parser.OFPFlowMod(datapath=datapath, priority=0, match=match, instructions=inst)
        datapath.send_msg(mod)

    # ---------------- Topology events ----------------
    @set_ev_cls(event.EventSwitchEnter)
    def switch_enter_handler(self, ev):
        dp = ev.switch.dp
        self.net.add_node(dp.id)
        self.logger.info(f"Switch {dp.id} entered network.")
        self._print_topology()

    @set_ev_cls(event.EventLinkAdd)
    def link_add_handler(self, ev):
        src = ev.link.src
        dst = ev.link.dst
        self.net.add_edge(
            src.dpid, dst.dpid,
            port_src=src.port_no,
            port_dst=dst.port_no,
            weight=1.0
        )
        self.prev_link_load[(src.dpid, dst.dpid)] = 0
        self.logger.info(f"Link detected: {src.dpid}:{src.port_no} <-> {dst.dpid}:{dst.port_no}")
        self._print_topology()

    def _print_topology(self):
        self.logger.info(f"Current Switches: {list(self.net.nodes())}")
        links = [(u, v, d) for u, v, d in self.net.edges(data=True)]
        self.logger.info(f"Current Links: {links}")

    # ---------------- Monitor Loop ----------------
    def _monitor(self):
        while True:
            for dp in self.datapaths.values():
                self._request_stats(dp)
            hub.sleep(5)

    def _request_stats(self, dp):
        parser = dp.ofproto_parser
        ofproto = dp.ofproto
        req = parser.OFPPortStatsRequest(dp, 0, ofproto.OFPP_ANY)
        dp.send_msg(req)

    # ---------------- Helper: Read bandwidth and delay ----------------
    def get_link_params(self, intf):
        bw, delay = 1.0, 0.001  # Default Mbps, sec
        try:
            # Delay from qdisc
            out = subprocess.getoutput(f"tc qdisc show dev {intf}")
            for line in out.split('\n'):
                if 'delay' in line:
                    delay_str = line.split('delay')[1].split()[0]
                    delay = float(delay_str.replace('ms','')) / 1000
            # Bandwidth from class
            out = subprocess.getoutput(f"tc class show dev {intf}")
            for line in out.split('\n'):
                if 'rate' in line:
                    rate_str = line.split('rate')[1].split()[0]
                    bw = float(rate_str.replace('Mbit',''))
        except Exception as e:
            self.logger.info(f"Error reading tc for {intf}: {e}")
        return bw, delay

    # ---------------- Port stats reply ----------------
    @set_ev_cls(ofp_event.EventOFPPortStatsReply, MAIN_DISPATCHER)
    def port_stats_reply_handler(self, ev):
        dpid = ev.msg.datapath.id
        polling_interval = 5
        for stat in ev.msg.body:
            for neighbor in self.net.neighbors(dpid):
                edge = (dpid, neighbor)
                if self.net.has_edge(dpid, neighbor):
                    port_src = self.net[dpid][neighbor]['port_src']
                    if stat.port_no != port_src:
                        continue

                    # Utilization
                    curr_tx = stat.tx_bytes
                    prev_tx = self.prev_link_load.get(edge, 0)
                    delta_tx = curr_tx - prev_tx if curr_tx >= prev_tx else curr_tx
                    self.prev_link_load[edge] = curr_tx

                    # Bandwidth & Delay
                    intf = f's{dpid}-eth{port_src}'
                    bw, delay = self.get_link_params(intf)
                    
                    # throughput in Mbps over polling interval
                    throughput_mbps = (delta_tx * 8 / 1e6) / polling_interval
                    utilization = throughput_mbps / bw  # fraction of bw

                    # Weight calculation
                    alpha, beta, gamma = 1.0, 1.0, 1.0
                    weight = alpha * (1 / bw) + beta * delay + gamma * utilization
                    self.net[dpid][neighbor]['weight'] = weight

                    # ------------------- Logging -------------------
                    self.logger.info(
                        f"Link {dpid}->{neighbor} | "
                        f"prev_tx: {prev_tx} | curr_tx: {curr_tx} | delta_bytes: {delta_tx} | "
                        f"bandwidth: {bw} Mbps | delay: {delay}s | "
                        f"utilization: {utilization:.2f} | weight: {weight:.2f}"
                    )

                    # ---------------- CSV Logging ----------------
                

                    self.log_file = "link_data.csv"
                    
                    # Ensure header exists
                    if not os.path.exists(self.log_file):
                        with open(self.log_file, "w", newline="") as f:
                            writer = csv.writer(f)
                            writer.writerow(["time", "src", "dst", "throughput_mbps", "utilization", "weight", "delay", "bandwidth_mbps"])

                        self.logger.info("Created new link_metrics.csv with header")
                   
                    t = time.time()
                    with open(self.log_file, "a", newline="") as f:
                        writer = csv.writer(f)
                        writer.writerow([t, dpid, neighbor, throughput_mbps, utilization, weight, delay, bw])

        # Recompute paths
        self._update_shortest_paths()

    # ---------------- Shortest path ----------------
    def _update_shortest_paths(self):
        new_paths = {}
        for src in self.net.nodes():
            new_paths[src] = {}
            for dst in self.net.nodes():
                if src == dst: continue
                try:
                    path = nx.dijkstra_path(self.net, source=src, target=dst, weight='weight')
                except nx.NetworkXNoPath:
                    path = None
                new_paths[src][dst] = path

                prev_path = self.shortest_paths.get(src, {}).get(dst)
                if path != prev_path and path is not None:
                    self.logger.info(f"Path changed {src}->{dst}: {path}")
                    if src in self.hosts and dst in self.hosts:
                        self.install_path(path, src, dst)

        self.shortest_paths = new_paths

    # ---------------- Flow installation ----------------
    def install_path(self, path, src_mac, dst_mac):
        for i in range(len(path)-1):
            dp = self.datapaths[path[i]]
            next_dp = path[i+1]
            out_port = self.net[path[i]][next_dp]['port_src']
            parser = dp.ofproto_parser
            ofproto = dp.ofproto

            match = parser.OFPMatch(eth_src=src_mac, eth_dst=dst_mac)
            actions = [parser.OFPActionOutput(out_port)]
            inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS, actions)]
            mod = parser.OFPFlowMod(
                datapath=dp,
                priority=100,
                match=match,
                instructions=inst,
                idle_timeout=10,
                hard_timeout=30
            )
            dp.send_msg(mod)

    # ---------------- Packet-In handler ----------------
    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def _packet_in_handler(self, ev):
        msg = ev.msg
        dp = msg.datapath
        in_port = msg.match['in_port']

        pkt = packet.Packet(msg.data)
        eth = pkt.get_protocol(ethernet.ethernet)
        if eth.ethertype == ether_types.ETH_TYPE_LLDP:
            return

        # Learn hosts
        if eth.src not in self.hosts:
            self.hosts[eth.src] = dp.id
            self.logger.info(f"Discovered host {eth.src} at switch {dp.id} port {in_port}")

        if eth.dst in self.hosts:
            src_sw = self.hosts[eth.src]
            dst_sw = self.hosts[eth.dst]
            if src_sw in self.net.nodes() and dst_sw in self.net.nodes():
                try:
                    path = nx.dijkstra_path(self.net, source=src_sw, target=dst_sw, weight='weight')
                    self.install_path(path, eth.src, eth.dst)
                except nx.NetworkXNoPath:
                    self.logger.info(f"No path from {src_sw} to {dst_sw}")
        else:
            parser = dp.ofproto_parser
            ofproto = dp.ofproto
            actions = [parser.OFPActionOutput(ofproto.OFPP_FLOOD)]
            out = parser.OFPPacketOut(
                datapath=dp,
                buffer_id=msg.buffer_id,
                in_port=in_port,
                actions=actions,
                data=msg.data if msg.buffer_id == ofproto.OFP_NO_BUFFER else None
            )
            dp.send_msg(out)

