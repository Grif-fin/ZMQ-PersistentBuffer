ZMQ-PersistentBuffer
====================

Functionality:

  -PersistentBuffer executable supports ZMQ message passthrough with in bound TCP port on "tcp://127.0.0.1:4242" and out bound TCP port on "tcp://127.0.0.1:4343".
  -Caches messages upto maximum available memory on ram.
  -Caching to file (after memory buffer is exusted) is implemented but NOT glued in yet.

Compilers:
  - GCC 4.8

Dependencies: 
  - zmq
  - zmqpp
  - boost_atomic
  - pthread
  
Passed Tests:
  - [stream_generator](https://github.com/datasift/ds-recruitment/tree/develop/stream_generator) has been used to test the memory caching, passthrough of items/sec and latency.
  