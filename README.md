# Software-Defined-Routing
Implemented a simplified version of the Distance Vector routing protocol over simulated routers.


# Implementation


>> Programming environment

You will write C (or C++) code that compiles under the GCC (GNU Compiler Collection) environment. Furthermore, you should ensure that your code compiles and operates correctly on 5 dedicated servers. Your code should successfully compile using the version of gcc (for C code) or g++ (for C++ code) found on these servers and should function correctly when executed.


NOTE: You are NOT allowed to use any external (not present by default on the dedicated servers) libraries for any part of the programming assignment. Bundling of code (or part of it) from external libraries with your source will not be accepted either. Further, your implementation should NOT invoke any external binaries (e.g., ifconfig, nslookup, etc.).


>> Sockets

Use only the select() system call for handling multiple socket connections. Do not use multi-threading or fork-exec.
Use select() timeout to implement multiple timers. Any other implementation of multiple timers will not be accepted.

>> Running your program

Your program will take 1 command line parameter:

The parameter is the port number on which your process will listen for control messages from the controller (see section 6.3). In the rest of the document, this port is referred to as the <control port>.

E.g., if your executable is named router:

To run with control port number 4322
./router 4322

>> Dedicated Servers

For the purpose of this assignment, you should only use (for development and/or testing) the directory created for you on each of the 5 dedicated servers. Change the access permission to this directory so that only you are allowed to access the contents of the directory. This is to prevent others from getting access to your code.


# Detailed Description


Routers are one of the most important elements in any network, responsible for forwarding packets to the next-hop in the network and eventually to their correct destination. To be able to perform this critical task, routers rely on the forwarding table. The forwarding table itself is constructed using the distance vector routing protocol. A router’s operations are divided into two dimensions: Data Plane and Control Plane (Figure 2).


https://lh4.googleusercontent.com/CgO3R2UDZXNvtjIE4Jpf1FzEPfx-a5MSMgcp6gJVlDT4hJvsBhNtYc4UZPHwsECvzqYDcgM2hl6FdSpCW36hRMgMYyH88tlTBq-1llLbOH2e3Ps4ZmL50-6zUDzyASh21eDvJDT9

Graphics Source: http://blogs.salleurl.edu/data-center-solutions/2016/03/sdn-the-latest-trend-in-data-centers-e2016/


This assignment requires the implementation of a very basic, simplified router, able to perform the functions described below:


CONTROL PLANE
The distance vector protocol will be implemented to run on top of the servers (behaving as routers) on a pre-defined port number. In the rest of the document, this port is referred to as the <router port>. Note that this is different from the control port number passed as argument during startup. Your implementation will in fact emulate a router at the application layer and use UDP as the transport layer protocol to exchange routing messages. Further, it will not modify the actual kernel routing table on the servers, but rather maintain a separate one. Lastly, you only need to implement the basic algorithm: count to infinity, NOT poison reverse. Since we are limited to the five dedicated servers (mentioned in section 3.1), we will at most have a network consisting of five routers.

In addition to the router functionality, your implementation will listen for and respond to control messages (see section 6.4) from a controller (see section 6.3), that will itself run on one of the five dedicated servers. The router, when launched, should start listening for TCP connections on the control port number passed as argument and respond to control messages. The control messages could either require functions to be performed on the router, or require it to send a response back (over the established TCP session) to the controller, or both.

Note that the SDN architecture in this assignment is slightly different from the generalized forwarding architecture described in Figure 4.28. In Figure 4.28, the control plane functionality is implemented exclusively on a remote controller. In contrast, in this assignment the control plane functionality is split between the remote controller and the routers. Specifically, the routers exchange control (routing) messages with each other (using their <router port>) and build the forwarding tables locally instead of getting them from the remote controller. At the same time, the remote controller can send commands to the routers (on their <control port>) that change the routing decisions, e.g., by changing a link costs, bringing down a router, etc.


DATA PLANE
Once forwarding tables stabilize, data packets can be routed between a given pair of source and destination routers. On the receipt of a specific control message, a router should read and packetize a given file using a fixed packet structure (see section 7.1) and forward it to the next hop router by looking at the forwarding table. The router that receives this packet should then forward it further and so on, until the packet reaches its destination. All data plane packets will use TCP on a pre-specified port number. In the rest of the document, this port is referred to as the <data port>.


>> Control and Data Plane Dual Functionality

When launched, your router application will listen for control messages on the control port. The very first control message received (of type INIT) will actually initialize the network. Apart from the list of neighbors and link-costs to them, it will contain the router and data port numbers. After the receipt of this initialization message, the application should start listening for routing updates/messages on the router port and data packets (that may require forwarding) on the data port. From this point onwards, the application will listen for messages/connections on the router (UDP), control (TCP), and data (TCP) ports simultaneously (using the select() system call) and respond to those messages (if required).


>> Packet byte-order

All packets will use the network (big-endian) byte order for all multi-byte fields.


>> Topology

After launching your application on the two or more CSE servers, the controller can be launched by supplying a topology file that it will use to build the network. The topology file contains an entire snapshot of the network. The controller, after reading the topology file, will send out the required INIT messages to each of the routers in the network.


The topology file is structured follows:


The 1st line contains a non-zero positive integer n, the number of routers in the network.

The next n lines contain the ID, IP address, control port, router port and data port number of each router in the network, formatted as:
<ID> <IP address> <control port> <router port> <data port>


Notes

Entries in a line are separated by a single space (‘ ‘) character
There is no space character after last entry in a line
Lines themselves are separated by a single newline (‘\n’) character

The next l lines contain the links existing between the routers and their cost, formatted as:
<router ID 1> <router ID 2> <cost>


Notes

Entries in a line are separated by a single space (‘ ‘) character
There is no space character after last entry in a line
Lines themselves are separated by a single newline (‘\n’) character
All links are bidirectional
All link costs are symmetrical

E.g., Consider the topology in Figure 1.

https://docs.google.com/drawings/d/sj2ty5Xe3vY3u-xtXhJ9aMQ/image?rev=1&h=223&w=347&ac=1

Figure 1: Example topology

Line #

Line entry

1

5

2

1 128.205.36.34 4091 3452 2344

3

2 128.205.35.46 4094 4562 2345

4

3 128.205.36.33 4096 8356 1635

5

4 128.205.36.35 7091 4573 1678

6

5 128.205.36.36 7864 3456 1946

7

1 2 7

8

9

4 5 9

1 4 2

10

11

12

3 4 5

3 2 4

3 5 11


Actual topology files will contain only the Line entry part (2nd column, see topology_example file included in the template). You can use your own topology files to test your code. However, we will use our topology files to test your program.


>> Numeric Types

All numeric fields (ID, cost, port numbers, control and response codes etc.), unless mentioned otherwise, should be represented/encoded as their unsigned versions.

INF (infinity) value: Largest unsigned integer that can be represented in 2 bytes.


# Control Plane

>> Routing Updates

Routing updates (distance vectors) should be sent periodically by a router to all its neighbors  (directly connected to the router) based on a time interval specified in the INIT control message, using the packet format described in section 6.2. All routing updates will be sent over UDP. Your application should NOT send routing updates to routers in the network that are not directly attached to it.

Routers can also be removed from the network as a result of receipt of certain control messages. When a router has been removed from the network, it should no longer send routing updates to its neighbors. Further, when a router does not receive distance vector updates from its neighbor for three consecutive update intervals, it assumes that the neighbor no longer exists in the network and makes the appropriate changes to its routing table (set link cost to this neighbor to infinity but do NOT remove it from the routing table). This information is propagated to other routers in the network with the exchange of routing updates. Please note that, although a router might be specified as a neighbor with a valid link cost in the topology file, the absence of three consecutive routing updates from that server will imply that it is no longer present in the network.

Also note that each router uses a different timer for each neighbor (to check for updates from that neighbor) which is set for first time when the router receives the first distance vector update message from that neighbor. E.g., if the update period is 3 sec and a router receives the first update from neighbor n1 at t=2s and from neighbor n2 at t=4s, then it will expect future updates from n1 at t=5, 8, 11, … and from n2 at t=7, 10, 13, … . Remember that you have to use select() to implement multiple timers.


>> Routing Update Packet Format

https://lh3.googleusercontent.com/XxoVuDYzaoCmASblSgF0FfKYry1ZV1tYSAgB1EYZhyoPJwZDzeiPo5BXeGu052NFoyo2DaH05lHNUId_xXK3iIdqh4kobifulWvAhG8scy5qJTaIYxV7A_WacBlFbAssJU78Yek0

Packet structure

Number of update fields: Number of router entries that follow
Source Router Port: <router port> of the router sending this packet (src router)
Source Router IP Address:  <IP address> of the router sending this packet
Router(x) IP Address:  <IP address> of the xth router in the forwarding table of the src router
Router(x) Port: <router port> of the xth router in the forwarding table of the src router
Router(x) ID: <ID> of the xth router in the forwarding table of the src router
Cost(x): <cost> estimate of the path from the src router to the xth router in the routing table of the src router

Notes

Routers can be listed in any order inside the packet
Total number of entries (n) should be equal to the number of routers in the network, including an entry to self with cost 0.
        

>> Controller

The controller is a separate application that generates control messages for the router application. We will provide the controller application and it need not be implemented. It sends control messages to the routers and expects a response within a time-limit. The controller will connect to a router on the router’s control port and may send any number of control messages or even none after establishing the connection. Further, the controller can terminate the connection to a router any time and initiate a new one sometime in the future. The router should always be ready to accept a connection and listen for control messages on the control port. The controller itself is stateless and does not retain any information across runs.


>> Control Messages and Response

Control messages generated by the controller and responses generated by the routers will use the packet formats described below. These control messages can be generated by supplying the required command line arguments to the controller application provided with the template. The router application will need to parse the messages and generate response messages which will then be read by the controller. The response message should be sent after completing all the operations required for a command, unless otherwise stated.


Both the control messages and control-response messages will use the header format described below. The payload for the packets will be different for each control message and its response is listed separately for each control message below. The payload may very well be empty (0 bytes). If there is no description of the payload for a control message or its corresponding response, you can assume it to be empty. In such cases, you should still expect/respond with just the header of the packet.


>>> Control Message Header

https://lh4.googleusercontent.com/fudaTeuEECmKaPW-dKJe-7HYrpVL8BT22JuhLCI2Sw2MW8y-gaLDprNwWcwGGsqnRiyd8Uy-s6dpD8E5JpJ719_90KnU7yZkfiSbuvVjSGo-Munxx4BEAM_RfcDDhMuq11baeCVK

Packet structure

Destination Router IP Address:  <IP address> of the router to which the control message is addressed
Control Code: A 8-bit unsigned integer indicating the type of control message. The control code for each type of message is mentioned below next to name of the message
Response Time: Expected response time (in seconds) within which the controller expects a response from the router
Payload Length: Size of the payload in bytes, excluding the header fields

>>> Control-Response Message Header

https://lh6.googleusercontent.com/2Pwud-ybcihDTz-I0RVHv-eW7cc8Y2thlG4xVqsHq3sdGf_huDA8LvFT9VFyFCvnIXAuyWWsnzVFHImdCBvKxQTUZQOaYEeQ4v-TLgJgbYphLCqO3nPBXQkshcx62YqX-pC546Vv

Packet structure

Controller IP Address:  <IP address> of the controller
Control Code: A 8-bit unsigned integer indicating the type of control message to which the response was generated.
Response Code: 0x00 is treated as success. Any other value is error. Controller will discard a packet with an error code
Payload Length: Size of the payload in bytes, excluding the header fields

>>> Control/Control-Response Payloads


AUTHOR [Control Code: 0x00]
Control-Response Payload

I, <ubitname>, have read and understood the course academic integrity policy.


Notes

<ubitname> is your UBIT-Name in lower case
Both the <ubitname> and the academic integrity statement will be ASCII coded

Your submission will not be graded if the AUTHOR response is incorrect.


INIT [Control Code: 0x01]
Contains the data required to initialize a router. It lists the number of routers in the network, their IP address, router and data port numbers, and, the initial costs of the links to all routers. It also contains the periodic interval (in seconds) for the routing updates to be broadcast to all neighbors (routers directly connected).

Note that the link cost value to all the routers, except the neighbors, will be INF (infinity). Further, there will be an entry to self as well with cost set to 0.

The router, after receiving this message, should start broadcasting the routing updates and performing other operations required by the distance vector protocol, after one periodic interval.

 

Control Payload

https://lh6.googleusercontent.com/uqa5naM-1xqxeSptLCZ-EDO7kB-n_4rE6Y7g1XpIClHEg0vQJVboHtbolLmXywKHUopv5-LICBp4hDtGv8XDSg8q2DtxelJotdp6j5U-XtSXJ_Z76nyoYm8dz-KB7qbfu6zy78p9

Packet structure

Number of Routers: Number of routers in the network
Updates Periodic Interval: Time (in seconds) to be used between two successive routing update packet broadcast
Router(x) ID: <ID> of the xth router
Router(x) Port-1: <router port> of the xth router
Router(x) Port-2: <data port> of the xth router
Cost(x): <cost> of the link to the xth router
Router(x) IP Address:  <IP address> of the xth router

ROUTING-TABLE [Control Code: 0x02]
The controller uses this to request the current routing/forwarding table from a given router. The table sent as a response should contain an entry for each router in the network (including self) consisting of the next hop router ID (on the least cost path to that router) and the cost of the path to it.


Control-Response Payload

https://lh6.googleusercontent.com/imu6eCqL5tq7XorYVQysc2cCrYWex6m4bD_2YBNtLequtASNO-XFAlSWD54Yh0CT9JhrxdqKTlzPPldtf4liTK9lfgOs9AeHJ__fv69B1K8F7wEdCwXf-pFDVcN_BAq2QZnEru_k

Packet structure

Router(x) ID: <ID> of the xth router
Router(x) Next-Hop ID: <ID> of the next-hop router in the shortest path to the xth router
Cost(x): <cost> of the path to the xth router

Notes

The number of entries should be equal to the number (n) of routers mentioned in the INIT control message
One of the entries should be to self with a cost of 0 and next hop the router itself.
If the cost of path to a router is INF (infinity), the next hop router ID would also be INF in such a case

If you do not implement the ROUTING-TABLE response correctly, most automated tests will fail.


UPDATE [Control Code: 0x03]
The controller uses this to change/update the link cost between between the router receiving this message and a neighboring router. Since in our network, links are symmetric, this control message will always be sent in pairs to both the routers involved in a link. Note that if the cost of a link changes to INF via the UPDATE command, the two neighbors should still send periodic routing updates to each other and they should not assume that the link has broken (in fact, the cost might change later to another value via another UPDATE command). This is in contrast to the case in 6.1 when a router discovers that a neighbor is down (after missing three routing updates from that neighbor); in that case, the router also stops sending updates to its neighbor.


Control Payload

https://lh6.googleusercontent.com/CoN7ZeHYCUbmhgDWAX5ILkLifyrUUUWcXVcXcEz28Nj2B9KS7nYHTN7s2M27reB0WCB7y9kgiO-EBjvBmyBmu_Uc9OU_lf-F9VhM2Yt69W2Y11l20nS6GnAbAAmqJjlCPOUXAC-S

Packet structure

Router ID: <ID> of the router to which the link cost is to be updated
Cost: New link <cost> to the router

CRASH [Control Code: 0x04]
The controller uses this to simulate a crash (unexpected failure) on a router. On receiving this message, the router should exit immediately. Note that you should not send messages to any routers in the network regarding the crash. They will discover this and update their routing tables, in due time, on their own. Note that in this case you have to send the response message before exiting. Once a router exits, it will not be respawned.


SENDFILE [Control Code: 0x05]
The controller uses this to initiate a file transfer from the router receiving this message to any other router in the network. See section 7 for details on how to packetize and route the packets.

The file to be sent will reside in the same folder as the binary executable of the router application. The receiving router should store the file in the same folder as the executable, with the name file-<Transfer ID>. Your implementation should be able to transfer both text and binary files. You can assume the maximum file size to be 10 MiB. In this case, you have to send the response message after the packet with the FIN bit set (last packet) has been sent to the next hop.


Control Payload

https://lh4.googleusercontent.com/sOjrmnNZ29MrB_9CLKfA5tyTxRrOelNm279DVygfRqsSpxdc-dN6leMngDVnYWDNdlvXfqUcIWQqc67WFBWKUYjgJ0NJOgl7cW1g3stJmrdj_6AwNBd9FcJ3L_BMwlHWKhZPea8o

Packet structure

Destination Router IP Address: <IP address> of the router to which the file is to be sent
Init. TTL: Initial TTL value to be used in data packets (see section 7)
Transfer ID: Unique number identifying the transfer/flow
Init. Seq. Number: Initial Sequence number to used in data packets (see section 7)
Filename: Name of the file (ASCII encoded) to be sent

SENDFILE-STATS [Control Code: 0x06]
The controller uses this to get statistics from the routers involved in a file transfer about the routed data packets. To be able to respond correctly to this control message, each router should store the TTL and Sequence number of each data packet that it sends/routes. These statistics need to be maintained per Transfer ID, on all routers including the source and destination router involved in a transfer.

Unless you are attempting BONUS, there will be only one active data transfer/flow at any given time, in the network.


Control Payload

https://lh5.googleusercontent.com/vw5B_Nf-jG0_Dt5ELm12PrDhAkp424FDG5XSZnktbWz9wOF1gMJ3eDP7TJX7ww7hyZnigiGVcDbbUvkMXdjd1vv-ji4Ps6iuK024GTzQ6Wk7cmyZ5kPPr8B0nAPUhzQBqyyFnqx0

Packet structure

Transfer ID: <ID> of the transfer/flow for which the statistics are requested

Control-Response Payload

https://lh3.googleusercontent.com/f0v_H3xMiUo3WEisIWFIgsmoXO2Tmw8WyDH2rjaYwb31qpQPPPTulu1Bgln0dVUFhqa4I8kewypTgQVNl3MXxuRDCdWT4E35bCoOPSWl4uZBipPWNopRrr02nPv5l2iqHNeiTSLW

Packet structure

Transfer ID: <ID> of the flow for which the statistics are provided
TTL: TTL value of routed packets after the decrement operation (see section 7)
Seq. Number(x): Sequence Number of the xth packet routed/sent

LAST-DATA-PACKET [Control Code: 0x07]
The controller uses this to get a copy of the last data packet that was sent/routed/received through the router. As a response to this control message, the router will put the last sent/routed/received packet as payload inside the control-response packet and send it to the controller over the control channel (connection established involving the control port). Note that in order to respond to this control message correctly, each router will always keep a copy of the last packet that it sends/routes which can then be sent to the controller when requested.


PENULTIMATE-DATA-PACKET [Control Code: 0x08]
The controller uses this to get a copy of the second last data packet that was sent/routed/received through the router. As a response to this control message, the router will put the second last sent/routed/received packet as payload inside the control-response packet and send it to the controller over the control channel (connection established involving the control port). Note that in order to respond to this control message correctly, each router will always keep a copy of the second last packet that it sends/routes which can then be sent to the controller when requested.

#  Data Plane

In a typical deployed network, routers will never generate data packets of their own but only route packets generated by end-hosts. However, for this assignment, routers can indeed generate data packets (on receipt of SENDFILE control message), in addition to routing them.

On receiving the SENDFILE control message, the sending (source) router will need to read the file from the disk and packetize it using the packet format described in section 7.1. The destination (sink) router will need to reconstruct the file back from the received data packets and write it to disk.

The SENDFILE control message along with the <Filename> of the file to be transferred, specifies a TTL and an initial sequence number value <seq>. The source router will put the TTL value in each packet generated. The first packet for a given transfer will have sequence number <seq>, the second one <seq>+1 and so on.

All data plane packets will be sent over TCP connections on the <data port> specified in the INIT control message. The routing procedure consists of decrementing the TTL value (by 1) of the data packet received, looking up the next hop router for the destination mentioned in the packet in the forwarding table and forwarding it. The same procedure is repeated at each router, until the packet reaches its destination (the sink router). If a packet’s TTL reaches 0 after the decrement operation, it should be dropped and not routed further.

Other than the TTL, a router should not modify any other fields in the packet in any way.


Notes

The source router should NOT decrement the TTL
The destination router should decrement the TTL and drop the packet if the TTL reaches 0
LAST-DATA-PACKET and PENULTIMATE-DATA-PACKET refer to packets with TTL>0


>> Data Packet Format

https://lh3.googleusercontent.com/Vlyw_idfA0fQDoyxlO46o734xIXaF-VykVOJa4xIJ6WtKup1IXL10ujdBbrfO4P6EbM_RA5vFKsqZiHtlnGPWwuin9rOZFFsYncijGesFx90xPf5tLKy6kHuMo0GFuJ6cQtD3UPS

Packet structure

Destination Router IP Address: <IP address> of the router to which the packet is destined
Transfer ID: Unique number identifying the transfer/flow
TTL: TTL value
FIN: This bit is set (1) for the last data packet of a given transfer/flow/file sent from the source, unset (0) otherwise
Sequence Number: Sequence number

Notes

File sizes used will always be a multiple of 1024 bytes
