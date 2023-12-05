Networks Part B

## Implementing TCP Functionality using UDP

This code represents a client-server system that simulates TCP-like behavior over UDP. Its main functionalities involve organizing data into smaller fragments and handling retransmissions:

**Data Sequencing:** The sender divides the text into smaller chunks, each assigned a unique identifier, and communicates the total number of chunks to the receiver. The receiver reassembles these fragments based on their sequence numbers to reconstruct the original text.

**Retransmission:** Upon receiving a data chunk, the receiver acknowledges it with a sequence number. If no acknowledgment is received within 0.1 seconds, the sender resends the same chunk, without waiting for acknowledgments related to previous chunks.

The client connects to the UDP Server as it did in the basic code.
However, here we use non blocking mode to send the
The user enters the input string.
The input string is broken into multiple data segments.
A struct Sache is used which holds a unique id for each data segment and we also store the time of the moment when the Sache was created.

### Server Side Code (Receiver of data, Sends acknowledgement)

- Creates a UDP Socket.
- Receives chunk size and number of chunks from the client (sender).
- Receives chunks from the client, processes its sequence number, and sends back an acknowledgment.
- Stores the chunk data into an array, in order.
- After receiving all chunks, data is aggregated to reconstruct the original string.

### Client Side Code (Sends data)

- Creates a UDP Socket.
- Uses non blocking mode.
- Sends this data to the server (receiver).
- Maintains a sent array.
- Loops through the chunks, constructs packets, and sends them to the server.
- Checks for acknowledgments for 0.05 seconds.
  - If ACK is received, it is marked off in the sent array, and the next chunk is sent.
  - If ACK is not received, the loop goes back to the .
- To handle retransmissions, at the start of every loop, a linked list of chunks whose acknowledgments are yet to be received is checked to see if a timeout has occurred.
  - If so, then the particular chunk is retransmitted.
  - Else, the next chunk is sent as usual.

## Difference with Actual TCP

1. While our attempt involves mimicking some aspects of TCP using UDP, such as incorporating acknowledgments for data transmission assurance, it's important to note that TCP is a considerably more intricate protocol with robust mechanisms for reliable data delivery.
2. Notably, the code provided lacks the inclusion of essential features like flow control and congestion control, which are pivotal elements of TCP. These mechanisms are designed to enhance data transmission efficiency and maintain network stability.
3. In the implemented code, sequence numbers are manually assigned to individual data chunks to achieve data sequencing. In contrast, TCP dynamically manages sequence numbers, ensuring the orderly and dependable transfer of data.

## Incorporating Flow Control

To address flow control, we can introduce a sliding window mechanism. The sliding window enables the sender to manage the volume of unacknowledged data sent at any given time, preventing the receiver or the network from becoming overwhelmed. We'll also update the receiver to accommodate this sliding window.

1. On the sender's end, we can enhance the packet structure by including information about the window size.
2. This information can then be conveyed to the receiver, enabling it to monitor data transmission and dynamically adjust its window size as needed.

This approach ensures effective flow control, preventing the sender from inundating the receiver with excessive unacknowledged data simultaneously.

Refer to this [link](https://www.geeksforgeeks.org/window-sliding-technique/) for more on Sliding Window:

## Summary

The client-server system emulates TCP-like behavior over UDP by implementing data sequencing and retransmissions.

## References

- [UDP](https://www.javatpoint.com/udp-protocol)
- [TCP](https://www.javatpoint.com/tcp)
