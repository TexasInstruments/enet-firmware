import vlab
import ethernet
from ethernet.EthernetFrame import EthernetFrame

# Load the J7ES platform and the testbench (the testbench contains all of the
# off chip components; in our case, the two Ethernet nodes)
vlab.load(u'keystone_scripts.j7es_ccs', args=['--testbench=testbench_switch_logic_9g.py'])
#vlab.load(u"keystone_scripts.j7es_ccs", args=['--testbench=testbench_9g_new.py', "--no_c66_cluster", "--no_c7x_cluster"]  + __args__)
# Get hold of the Ethernet nodes so we can work with them
#port0_node = vlab.get_instance('port1_node').obj
port1_node = vlab.get_instance('port1_node').obj
port2_node = vlab.get_instance('port2_node').obj
port3_node = vlab.get_instance('port3_node').obj
port4_node = vlab.get_instance('port4_node').obj
port5_node = vlab.get_instance('port5_node').obj
port6_node = vlab.get_instance('port6_node').obj
port7_node = vlab.get_instance('port7_node').obj
port8_node = vlab.get_instance('port8_node').obj


# Activate the two nodes (otherwise they won't do anything)
#port0_node.activate()
port1_node.activate()
port2_node.activate()
port3_node.activate()
port4_node.activate()
port5_node.activate()
port6_node.activate()
port7_node.activate()
port8_node.activate()

# Enable the ALE (this should be done by the image running on the core, but,
# for simplicity, we use the VLAB API to do it here)
# # vlab.write_register('platform.cpsw9.CPSW_NU_ALE_ALE_CONTROL',  0x80000000)
# # vlab.run_until(vlab.trigger.time(1, 'ns'))

# Enable port forwarding for the port that tx_node is connected to (again,
# for simplicity, we use the VLAB API to do this)
# # vlab.write_register('platform.cpsw9.CPSW_NU_ALE_I0_ALE_PORTCTL0[1]', 0x3)
# # vlab.run_until(vlab.trigger.time(1, 'ns'))

# Create an Ethernet frame for one of the nodes to transmit (we only care about
# the MAC addresses, so we don't bother to set the payload)
# tx_frame = EthernetFrame(200)
# tx_frame.SetDestAddress(0xabc123)
# tx_frame.SetSourceAddress(0x456def)


