# fsw-linux-homework

The programs have been writen in C11 standard, and they are compatible with x86_64 Linux systems.
Only the standard C library, and standard POSIX tools have been used. No third party libraries have been required.

No docker container for setting up the compilation environment has been created.
Distro Ubuntu-20.04 has been used for the development. It is only assumed that package "build-essential" is installed, which include basic tools for building: make, gcc, libc6-dev, etc

Just the functionalities as described in the provided task descriptions have been developed.
If additional features, mainly for debugging, have been included in intermediate commits, all them have been removed from the final version of the programs.

No dynamic memory allocation has been used in the code of any program.

A Makefile to compile both programs is provided: use `make client1` and `make client2`

Before the execution of any program (`./client1` or `./client2`), the provided server application docker image must be loaded and run. The logs printed just after running the server show the server IP address ('0.0.0.0') and confirm the TCP data ports (4001, 4002, 4003), and the UDP control port (4000).

```
root@LAPTOP-ALEX:~/projects/fsw-linux-homework# docker load < fsw-linux-homework.tar.gz
Loaded image: fsw-linux-homework:latest
root@LAPTOP-ALEX:~/projects/fsw-linux-homework# docker run -p 4000:4000/udp -p 4001:4001 -p 4002:4002 -p 4003:4003 fsw-linux-homework &
[1] 2443
root@LAPTOP-ALEX:~/projects/fsw-linux-homework# Version: 2024-03-13-179a8b2
Starting servers...
Control channel listening on 0.0.0.0:4000
Server listening on 0.0.0.0:4002
Server listening on 0.0.0.0:4003
Server listening on 0.0.0.0:4001
```

## Task 1

- No third party JSON library has been used. The output JSON structure is quite simple, and it has been manually constructed.
- For accurate timing of 100ms intervals, a fixed absolute schedule has been chosen. The main thread is waken up at absolute time points separated 100ms. No interval sleep is used.
- This program creates 3 POSIX pthreads to manage concurrently each of 3 TCP data streams from server (out1, out2, out3). Each thread creates a SOCK_STREAM socket, connects to a server TCP port, and enters into a loop that continuously receives values from the server and holds the last one from each server data stream.
- In the main program thread, there is a loop that is waken up every 100ms to get the last values received from each server data stream, and resets them to value '--'. If no value is received from the server during last 100ms interval, the value '--' is got. The 3 values from the server data streams, together with the current timestamp, are formated according to the provided JSON object and printed to the stdout.
- To read and write the variables containing the last values received from each server data stream, 2 mutexes have been used: one thread mutex to protect concurrent read/write access to each value, and one global mutex to asure that all 3 values are read at the same time to construct the JSON object, without being updated until all them have been read (design assumption).
- In the first tests that I have performed to check the format of the values received from the server within each TCP data stream, I have found that the values are received as a sequence of chars that forms a decimal number with just one decimal place, maybe preceded by minus ('-') sign. Moreover, each number ends with the "end of line" chars ("\r\n"), which I remove to construct the output JSON object.
- A sample of the stdout for the execution of program `client1` is:
```
{"timestamp": 1765113592633, "out1": "5.0", "out2": "--", "out3": "5.0"}
{"timestamp": 1765113592733, "out1": "4.7", "out2": "2.0", "out3": "--"}
{"timestamp": 1765113592833, "out1": "4.0", "out2": "--", "out3": "--"}
{"timestamp": 1765113592933, "out1": "2.9", "out2": "1.0", "out3": "--"}
{"timestamp": 1765113593033, "out1": "1.1", "out2": "--", "out3": "--"}
{"timestamp": 1765113593133, "out1": "-0.5", "out2": "-0.0", "out3": "0.0"}
{"timestamp": 1765113593233, "out1": "-1.8", "out2": "--", "out3": "--"}
{"timestamp": 1765113593333, "out1": "-2.9", "out2": "-1.0", "out3": "--"}
{"timestamp": 1765113593433, "out1": "-3.6", "out2": "--", "out3": "--"}
{"timestamp": 1765113593533, "out1": "-4.8", "out2": "-2.0", "out3": "--"}
{"timestamp": 1765113593633, "out1": "-5.0", "out2": "--", "out3": "0.0"}
{"timestamp": 1765113593733, "out1": "-4.6", "out2": "-3.0", "out3": "--"}
{"timestamp": 1765113593833, "out1": "-3.8", "out2": "--", "out3": "--"}
{"timestamp": 1765113593933, "out1": "-2.5", "out2": "-4.0", "out3": "--"}
{"timestamp": 1765113594033, "out1": "-1.2", "out2": "--", "out3": "--"}
{"timestamp": 1765113594133, "out1": "0.2", "out2": "-5.0", "out3": "5.0"}
```

# What are the frequencies, amplitues and shapes you see on the server outputs?

- Assumption: If the server doesn't send an update for a value, the last received value remains valid.

- In order to answer these questions, I have used 2 approaches: first one is the visual inspection of the stdout for the execution of program `client1` (which are the peak and bottom values, and what are the time differences between them); the second one is the use of a simple python script that receives as an argument a data.txt file with the output for an execution of program `client1` and plots 3 graphs, one for each value (x axis is the timestamp). This script is called `plot_server_data.py` and it is in folder /tools. 

Use `python3 plot_server_data.py data_client1.txt` to run the script with sample data file `data_client1.txt`. Output graph is `graph_clien1.png`.

With this information, I can conclude that:
- out1: Amplitude [-5.0, 5.0], Frequency 1/(20*100ms) = 0.5Hz, Sinusoid waveform
- out2: Amplitude [-5.0, 5.0], Frequency 1/(40*100ms) = 0.25Hz, Triangular waveform
- out3: Amplitude [0.0, 5.0], No periodic binary signal, Squared waveform. The narrowest pulse width (bit time) I have seen is 1sec.


## Task 2

- This programs has a timing interval of 20ms instead of 100ms.

- The control protocol specification defines the read and write messages structure and their fields. We know the values for field 'operation' (`1` - read, `2` - write). But no info is provided about the fields 'object', 'property' and 'value'.

    * Each read message consists of three fields: operation, object, and property.
    * Each write message consists of four fields: operation, object, property, and value.

- This program creates a SOCK_DGRAM socket, builds a command buffer according to the message specification, and sends the buffer to the server UDP port. Reception feedback from the server is writen to the standard output.
- So, to get some info about the message fields 'object', 'property' and 'value', a first step that I have done is sending a burst of read control messages to the server, and analyze the server output:

    * First burst: fixing 'property' value to 0x0000, and iterate 'object' from 0x0000 to 0xFFFF. Server ouput is:

        ...
        No such object: 0
        1.0: no such property
        2.0: no such property
        3.0: no such property
        No such object: 4
        ...

    Only 'object' values 0x0001, 0x0002 and 0x0003 are acknowledged to be valid. We infere they correspond to TCP data streams out1, out2 and out3.

    * Second burst: fixing 'object' value to 0x0001 (out1), and iterate 'property' from 0x0000 to 0xFFFF. Server ouput is:

        ...
        1.13: no such property
        1.14: enabled=1
        1.15: no such property
        ...
        1.169: no such property
        1.170: amplitude=5000
        1.171: no such property
        ...
        1.254: no such property
        1.255: frequency=500
        ...

    We discover that: 
        - 'property' 14 (0x000E) is 'enabled'. Current value is 1 (On?)
        - 'property' 170 (0x00AA) is 'amplitude'. Current value is 5000. It matches out1 amplitude 5.0 (x1000)
        - 'property' 255 (0x00FF) is 'frequency'. Current value is 500. It matches out1 frequency 0.5Hz (500mHz)

- Once I know the missing details of the control protocol, what I have done in program `client2` is detecting when TCP data stream out3 "becomes greater than or equal 3.0" or "becomes lower than 3.0" to send a write control message to the server to change the behaviour of out1, according to the task specification.

    * When the value on the output 3 of the server becomes greater than or equal 3.0:
        * Set the frequency of server output 1 to 1Hz -> Write object 0x0001, property 0x00FF, value 1000
        * Set the amplitude of server output 1 to 8000 -> Write object 0x0001, property 0x00AA, value 8000

    * When the value on the output 3 becomes lower than 3.0:
        * Set the frequency of server output 1 to 2Hz -> Write object 0x0001, property 0x00FF, value 2000
        * Set the amplitude of server output 1 to 4000 -> Write object 0x0001, property 0x00AA, value 4000

- The script `plot_server_data.py` in folder /tools has also been used to analyze a sample data file `data_client2.txt` with the output of an execution of program `client2`: Use `python3 plot_server_data.py data_client2.txt` to run it. Output graph is `graph_clien2.png`.

- A sample of the stdout for the execution of program `client2` is:
```
{"timestamp": 1765226988748, "out1": "1.0", "out2": "--", "out3": "--"}
{"timestamp": 1765226988768, "out1": "--", "out2": "--", "out3": "5.0"}
ok,frequency=1000
ok,amplitude=8000
{"timestamp": 1765226988788, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226988808, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226988828, "out1": "-2.9", "out2": "--", "out3": "--"}
{"timestamp": 1765226988848, "out1": "-3.4", "out2": "--", "out3": "--"}
{"timestamp": 1765226988868, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226988888, "out1": "-5.5", "out2": "--", "out3": "--"}
{"timestamp": 1765226988908, "out1": "-6.2", "out2": "--", "out3": "--"}
{"timestamp": 1765226988928, "out1": "-6.5", "out2": "--", "out3": "--"}
{"timestamp": 1765226988948, "out1": "-7.0", "out2": "--", "out3": "--"}
{"timestamp": 1765226988968, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226988988, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226989008, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226989028, "out1": "-8.0", "out2": "--", "out3": "--"}
{"timestamp": 1765226989048, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226989068, "out1": "-7.6", "out2": "-4.0", "out3": "--"}
{"timestamp": 1765226989088, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226989108, "out1": "-6.8", "out2": "--", "out3": "--"}
{"timestamp": 1765226989128, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226989148, "out1": "-5.5", "out2": "--", "out3": "--"}
{"timestamp": 1765226989168, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226989188, "out1": "-3.9", "out2": "--", "out3": "--"}
{"timestamp": 1765226989208, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226989228, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226989248, "out1": "--", "out2": "--", "out3": "--"}
{"timestamp": 1765226989268, "out1": "-0.0", "out2": "-5.0", "out3": "0.0"}
ok,frequency=2000
ok,amplitude=4000
{"timestamp": 1765226989288, "out1": "--", "out2": "--", "out3": "--"}
```

# How do you know the solution is correct?

Even though I don't know the exact behavior of the server, I have reasonable assurance that my programs `client1` and `client2` are working correctly because:

1. TCP connections are established and maintained without unexpected disconnections. The TCP channels are active and stable.

2. There are no blocking issues on the reception. My main thread always wakes up at regular intervals and displays consistent timestamps and numerical data to the standard output as a JSON object.

3. Control protocol's read and write messages cause a consistent change in the server's signals behavior, together with the corresponding feedback on the server's standard output.

To be absolutely certain that my programs are working correctly, I should use a test server that generates well-known patterns, and check that my program receives and prints them exactly as they are. Moreover, it would also be necessary to check for boundary conditions, such as: zero bytes received, server shutdowns, very large messages, very fast messages, non-printable values, etc.
