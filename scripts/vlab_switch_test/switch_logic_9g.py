import vlab
import ethernet
from ethernet.EthernetFrame import EthernetFrame
import threading
import random
import time

# Get hold of the Ethernet nodes so we can work with them
#port0_node = vlab.get_instance('port0_node').obj
port1_node = vlab.get_instance('port1_node').obj
port2_node = vlab.get_instance('port2_node').obj
port3_node = vlab.get_instance('port3_node').obj
port4_node = vlab.get_instance('port4_node').obj
port5_node = vlab.get_instance('port5_node').obj
port6_node = vlab.get_instance('port6_node').obj
port7_node = vlab.get_instance('port7_node').obj
port8_node = vlab.get_instance('port8_node').obj

# MAC address
mac_addr  = [ 0x020000000000, 0x040000000000, 0x060000000000, 0x080000000000, 0x0A0000000000, 0x0C0000000000, 0x0E0000000000, 0x100000000000 ]
host_addr = 0xf4f4f4f4f4f4

#Transmit & Receive status
tx_status       = True
receive_flushed = True

print 'tx_status:', tx_status
print 'receive_flushed:', receive_flushed
#Thread lock
lock = threading.Lock()

def create_frame(src=None, dest=None):
    # Create an Ethernet frame for one of the nodes to transmit (we only care about
    # the MAC addresses, so we don't bother to set the payload)
    tx_frame  = EthernetFrame(200)
    tx_frame.SetSourceAddress(mac_addr[src])
    tx_frame.SetDestAddress(mac_addr[dest])
         
    return tx_frame

def transmit_frame(src_mac_idx, tx_frame):
    #Port 1 Transmit
     if src_mac_idx == 0:
         port1_node.transmit_frame(tx_frame)
     #Port 2 Transmit
     elif src_mac_idx == 1:
         port2_node.transmit_frame(tx_frame)
     #Port 3 Transmit
     elif src_mac_idx == 2:
         port3_node.transmit_frame(tx_frame)
     #Port 4 Transmit
     elif src_mac_idx == 3:
         port4_node.transmit_frame(tx_frame)
     #Port 5 Transmit
     elif src_mac_idx == 4:
         port5_node.transmit_frame(tx_frame)
     #Port 6 Transmit
     elif src_mac_idx == 5:
         port6_node.transmit_frame(tx_frame)
     #Port 7 Transmit
     elif src_mac_idx == 6:
         port7_node.transmit_frame(tx_frame)
     #Port 8 Transmit
     elif src_mac_idx == 7:
         port8_node.transmit_frame(tx_frame)

def transmit():
    global receive_flushed
    global tx_status

    while receive_flushed:
        continue

    lock.acquire()
    print 'receive flush done'
    lock.release()
    for i in range(20):
         src_mac_idx = random.randrange(7)
         dest_mac_idx = random.randrange(7)
         if src_mac_idx != dest_mac_idx:
             tx_frame = create_frame(src_mac_idx, dest_mac_idx)
             time.sleep(1)
             transmit_frame(src_mac_idx, tx_frame)
             lock.acquire()
             print 'Port'+str(src_mac_idx+1)+' -> Port'+str(dest_mac_idx+1)+': '+str(tx_frame)
             lock.release()

    #lock.acquire()
    print "Transmit Done"
    #lock.acquire()
    tx_status = False

def receive():
    
    global receive_flushed
    global tx_status
    rx_last_loop  = False

    while True:
        #print "In receive Loop"
        #time.sleep(0.5)
        rx_status = False
        if port1_node.has_received_new_frame():
            rx_frame = port1_node.get_latest_received_EthernetFrame()
            lock.acquire()
            print 'Port1 received:', rx_frame, ':-)'
            lock.release()
            rx_status = True
    
        if port2_node.has_received_new_frame():
            rx_frame = port2_node.get_latest_received_EthernetFrame()
            lock.acquire()
            print 'Port2 received:', rx_frame, ':-)'
            lock.release()
            rx_status = True
    
        if port3_node.has_received_new_frame():
            rx_frame = port3_node.get_latest_received_EthernetFrame()
            lock.acquire()
            print 'Port3 received:', rx_frame, ':-)'
            lock.release()
            rx_status = True
    
        if port4_node.has_received_new_frame():
            time.sleep(.1)
            rx_frame = port4_node.get_latest_received_EthernetFrame()
            lock.acquire()
            print 'Port4 received:', rx_frame, ':-)'
            lock.release()
            rx_status = True
    
        if port5_node.has_received_new_frame():
            rx_frame = port5_node.get_latest_received_EthernetFrame()
            lock.acquire()
            print 'Port5 received:', rx_frame, ':-)'
            lock.release()
            rx_status = True
    
        if port6_node.has_received_new_frame():
            rx_frame = port6_node.get_latest_received_EthernetFrame()
            lock.acquire()
            print 'Port6 received:', rx_frame, ':-)'
            lock.release()
            rx_status = True
    
        if port7_node.has_received_new_frame():
            rx_frame = port7_node.get_latest_received_EthernetFrame()
            lock.acquire()
            print 'Port7 received:', rx_frame, ':-)'
            lock.release()
            rx_status = True
    
        if port8_node.has_received_new_frame():
            rx_frame = port8_node.get_latest_received_EthernetFrame()
            lock.acquire()
            print 'Port8 received:', rx_frame, ':-)'
            lock.release()
            rx_status = True
        
        receive_flushed = False
        
        if tx_status == False:
            rx_last_loop = True
            tx_status = True
            time.sleep(1)
            continue
            
        if rx_last_loop == True and rx_status == False:
            lock.acquire()
            print "No frame to receive"
            lock.release()
            break

    lock.acquire()        
    print "Receive Done"
    lock.release()

# creating thread 
tx = threading.Thread(target=transmit, args=()) 
rx = threading.Thread(target=receive, args=()) 

# starting Transmit thread
tx.start() 
# starting Receive thread
rx.start()

# wait until transmit thread is completely executed 
tx.join()
# wait until receive thread is completely executed 
rx.join()

print "Test Done"
