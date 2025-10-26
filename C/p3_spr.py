
from ryu.base import app_manager
from ryu.controller import ofp_event
from ryu.controller.handler import MAIN_DISPATCHER, CONFIG_DISPATCHER
from ryu.controller.handler import set_ev_cls
from ryu.ofproto import ofproto_v1_3
from ryu.topology import api
from ryu.topology import event
import networkx
from ryu.lib.packet import packet, ethernet
from ryu.lib.packet import lldp, cfm
from ryu.lib.packet import packet, ethernet
from ryu.lib.packet import lldp, ether_types
from ryu.ofproto.ether import ETH_TYPE_LLDP
from ryu.ofproto.ether import ETH_TYPE_CFM
import subprocess



class SPRController(app_manager.RyuApp):

    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    SWITCH_INFO = {}
    HOST_INFO = {}

    nw_graph = networkx.Graph()


    def log_topology_to_file(self):

        with open('topology.txt', 'w') as f:

            f.write("Hosts:\n")
            # Read all hosts from nw_graph and write to file
            hosts = sorted([node for node in self.nw_graph.nodes() if self.nw_graph.nodes[node]['type'] == 'host'])
            for host in hosts:
                f.write("Host " + str(host) + "\n")

            f.write("Switches:\n")
            # Read all switches from nw_graph and write to file
            switches = [node for node in self.nw_graph.nodes() if self.nw_graph.nodes[node]['type'] == 'switch']
            for switch in switches:
                f.write("Switch " + str(switch) + "\n")

            f.write("Links:\n")
            # Read all links from nw_graph and write to file
            links = [edge for edge in self.nw_graph.edges()]
            for link in links:
                f.write(str(self.nw_graph.edges[link]['desc']) + " : " + str(link) + "  Delay: " + str(self.nw_graph.edges[link]['delay']) + " Bw: " + str(self.nw_graph.edges[link]['bw']) + " Weight: " + str(self.nw_graph.edges[link]['weight']) + "\n")


            # Log shortest paths between each host pair
            for i in range(len(hosts)):
                for j in range(i+1, len(hosts)):
                    src = hosts[i]
                    dst = hosts[j]
                    
                    shortest_path = networkx.shortest_path(self.nw_graph, src, dst, weight='weight')
                    f.write("\n****************************************\n")
                    f.write("Shortest path from Host interface " + str(src) + " to Host interface " + str(dst) + ":\n\n")

                    for node in shortest_path:
                        f.write(" -> ")
                        
                        if node in hosts:
                            f.write("(Host Interface)" + str(node))
                        else:
                            if node in switches:
                                f.write("(Switch)" + str(node))
                            else:
                                f.write("(Switch Interface)" + str(node))

                        
                    f.write("\n")


            
    def _display_graph(self):
        print("Network graph: ", self.nw_graph.nodes(data=True))
        print("Network graph edges: ", self.nw_graph.edges(data=True))
        self.log_topology_to_file()

    def __init__(self, *args, **kwargs):
        super(SPRController, self).__init__(*args, **kwargs)

        print("Starting SPRController")

        #print(api.get_all_switch(self))


    def _get_delay(self, interface):
        cmd = "tc qdisc show dev " + interface + " | grep delay | awk '{print $9}'"
        result = subprocess.check_output(cmd, shell=True)
        return float(result.strip()[:-2])


    def _get_bw(self, interface):
        cmd = "tc class show dev " + interface + " | grep rate | awk '{print $12}'"
        result = subprocess.check_output(cmd, shell=True)
        return float(result.strip()[:-4])


    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        match = parser.OFPMatch()
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER,
                                          ofproto.OFPCML_NO_BUFFER)]
        self.add_flow(datapath, 1, match, actions)

    def add_flow(self, datapath, priority, match, actions, buffer_id=None):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS,
                                             actions)]
        if buffer_id:
            mod = parser.OFPFlowMod(datapath=datapath, buffer_id=buffer_id,
                                    priority=priority, match=match,
                                    instructions=inst)
        else:
            mod = parser.OFPFlowMod(datapath=datapath, priority=priority,
                                    match=match, instructions=inst)
        datapath.send_msg(mod)

    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def packet_in_handler(self, ev):
        msg = ev.msg
        dp = msg.datapath
        ofp = dp.ofproto
        ofp_parser = dp.ofproto_parser

        pkt = packet.Packet(msg.data)
        eth = pkt.get_protocol(ethernet.ethernet)
        
        # ignore lldp and cfm packets
        if eth.ethertype in (ETH_TYPE_LLDP, ETH_TYPE_CFM):
            return

        dest = eth.dst
        source = eth.src


        forwarding_port = ofp.OFPP_FLOOD

        if dest in self.nw_graph.nodes():
            print("Packet in from switch:", dp.id, "port:", msg.match['in_port'], "dest:", dest, "source:", source)

            path = networkx.shortest_path(self.nw_graph, dp.id, dest, weight='weight')   
            print("Path: ", path)

            #next =path[path.index(dpid)+1]
            if len(path) >= 2:
                print("Path: ", self.nw_graph.nodes[path[1]])
                forwarding_port = int(self.nw_graph.nodes[path[1]]['port_id'])
                print("Forwarding port: ", forwarding_port)

        
        actions = [ofp_parser.OFPActionOutput(forwarding_port)]


        if forwarding_port != ofp.OFPP_FLOOD:
            match = ofp_parser.OFPMatch(in_port=msg.match['in_port'], eth_dst=dest, eth_src=source)
            
            if msg.buffer_id != ofp.OFP_NO_BUFFER:
                self.add_flow(dp, 1, match, actions, msg.buffer_id)
                return
            else:
                self.add_flow(dp, 1, match, actions)


        data = None
        if msg.buffer_id == ofp.OFP_NO_BUFFER:
            data = msg.data

        out = ofp_parser.OFPPacketOut(
            datapath=dp, buffer_id=msg.buffer_id, in_port=msg.match['in_port'],
            actions=actions, data=data)
        
        dp.send_msg(out)




    @set_ev_cls(event.EventHostAdd)
    def _event_host_add_handler(self, ev):
        msg = ev.host.to_dict()
        print("Host added: ", msg)

        if msg['mac'] not in self.nw_graph.nodes():
            self.nw_graph.add_node(msg['mac'], type='host')
            self.nw_graph.add_edge(msg['mac'], msg['port']['hw_addr'], type='host-to-switch-port', weight=0, delay=0, bw=0, desc = "Link from host to switch " + str(int(msg['port']['dpid'])))

        self._display_graph()


    @set_ev_cls(event.EventSwitchEnter)
    def _event_switch_enter_handler(self, ev):
        msg = ev.switch.to_dict()
        print("Switch entered: ", msg)

        self.nw_graph.add_node(int(msg['dpid']), type='switch')

        for port in msg['ports']:
            self.nw_graph.add_node(port['hw_addr'], type='switch-port', port_id = port['port_no'], switch_id = int(msg['dpid']))

            self.nw_graph.add_edge(int(msg['dpid']), port['hw_addr'], type='switch-to-switch-port', weight=0, delay=0, bw=0, desc = "Link from switch " + str(int(msg['dpid'])) + " to its own port " + str(int(port['port_no'])))


    @set_ev_cls(event.EventSwitchLeave)
    def _event_switch_leave_handler(self, ev):
        msg = ev.switch.to_dict()
        print("Switch left: ", msg)

    @set_ev_cls(event.EventLinkAdd)
    def _event_link_add_handler(self, ev):
                
        msg = ev.link.to_dict()
        print("Link added: ", msg)

        # Get Link bw and delay to calculate weight


        delay = float(self._get_delay(msg['src']['name']))/1000
        bw = float(self._get_bw(msg['src']['name']))

        # RTT = delay * 2 
        bandwidth_delay_product = float( 2 * delay * bw)

        weight = float(1 / bandwidth_delay_product)

        self.nw_graph.add_edge(msg['src']['hw_addr'], msg['dst']['hw_addr'], type='switch-port-to-switch-port', weight=weight, delay=delay, bw=bw, desc = "Link from switch " + str(int(msg['src']['dpid'])) + " to switch " + str(int(msg['dst']['dpid'])))



        self._display_graph()


    @set_ev_cls(event.EventPortAdd)
    def _event_port_add_handler(self, ev):
        msg = ev.port.to_dict()
        print("Port added: ", msg)

    @set_ev_cls(event.EventPortModify)
    def _event_port_modify_handler(self, ev):
        msg = ev.port.to_dict()
        print("Port modified: ", msg)