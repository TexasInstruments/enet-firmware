import vlab
import ethernet
from ethernet.EthernetFrame import EthernetFrame
ethernetnode_component = vlab.component('EthernetNode', module='ethernet.EthernetNode')

# Add Nine Ethernet nodes
#port0_node = vlab.instantiate(ethernetnode_component, 'port0_node')
port1_node = vlab.instantiate(ethernetnode_component, 'port1_node')
port2_node = vlab.instantiate(ethernetnode_component, 'port2_node')
port3_node = vlab.instantiate(ethernetnode_component, 'port3_node')
port4_node = vlab.instantiate(ethernetnode_component, 'port4_node')
port5_node = vlab.instantiate(ethernetnode_component, 'port5_node')
port6_node = vlab.instantiate(ethernetnode_component, 'port6_node')
port7_node = vlab.instantiate(ethernetnode_component, 'port7_node')
port8_node = vlab.instantiate(ethernetnode_component, 'port8_node')

# Connect the Ethernet nodes to the CPSW ports in the Keystone platform
#vlab.connect((port0_node, 'TX'), ('platform', 'CPSW9_SGMII_RX_0'), kind='buffer')
#vlab.connect((port0_node, 'RX'), ('platform', 'CPSW9_SGMII_TX_0'), kind='buffer')
vlab.connect((port1_node, 'TX'), ('platform', 'CPSW9_SGMII_RX_1'), kind='buffer')
vlab.connect((port1_node, 'RX'), ('platform', 'CPSW9_SGMII_TX_1'), kind='buffer')
vlab.connect((port2_node, 'TX'), ('platform', 'CPSW9_SGMII_RX_2'), kind='buffer')
vlab.connect((port2_node, 'RX'), ('platform', 'CPSW9_SGMII_TX_2'), kind='buffer')
vlab.connect((port3_node, 'TX'), ('platform', 'CPSW9_SGMII_RX_3'), kind='buffer')
vlab.connect((port3_node, 'RX'), ('platform', 'CPSW9_SGMII_TX_3'), kind='buffer')
vlab.connect((port4_node, 'TX'), ('platform', 'CPSW9_SGMII_RX_4'), kind='buffer')
vlab.connect((port4_node, 'RX'), ('platform', 'CPSW9_SGMII_TX_4'), kind='buffer')
vlab.connect((port5_node, 'TX'), ('platform', 'CPSW9_SGMII_RX_5'), kind='buffer')
vlab.connect((port5_node, 'RX'), ('platform', 'CPSW9_SGMII_TX_5'), kind='buffer')
vlab.connect((port6_node, 'TX'), ('platform', 'CPSW9_SGMII_RX_6'), kind='buffer')
vlab.connect((port6_node, 'RX'), ('platform', 'CPSW9_SGMII_TX_6'), kind='buffer')
vlab.connect((port7_node, 'TX'), ('platform', 'CPSW9_SGMII_RX_7'), kind='buffer')
vlab.connect((port7_node, 'RX'), ('platform', 'CPSW9_SGMII_TX_7'), kind='buffer')
vlab.connect((port8_node, 'TX'), ('platform', 'CPSW9_SGMII_RX_8'), kind='buffer')
vlab.connect((port8_node, 'RX'), ('platform', 'CPSW9_SGMII_TX_8'), kind='buffer')

