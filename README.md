# Multicast-Causal

# Table of Contents
- [Introduction](#introduction)
- [Causal Ordering](#causal-ordering)
- [Implementation](#implementation)
    - [Assumption](#assumption)
- [How to run](#how-to-run)
- [Author](#author)

# Introduction
**Multicast-Causal** creates an architecture in which all the machines behave as a client and server simultaneously to implement the Multicast message ordering mechanism using Causal ordering.

# Causal Ordering

In causal ordering, on receiving a message from a process, a causality check is done on message. If the message satisfies the conditions of causality, it is delivered to the application, otherwise it is buffered till it satisfies those conditions.

# Implementation

- All the machines are connected to each other and each process or machine knows about the total number of machines in the system. 

- Every process maintains a vector clock in which the logical clock values of all corresponding processes is maintained.

- All the processes multicast messages to all the other processes in the system and send their vector clocks along with the message. 

- While sending a message, the process increments the vector clock value corresponding to its index by one and sends the vector with message.

    ## Assumption
    *It is assumed that any process in this system will send multicast message to all the other processes in the system i.e. for every process, the group for multicasting any message contains all the other processes in the system.*

# How to run
There are some specific considerations which needs to be taken care of while running any of these programs.

- There is a file in the respective directory of this assignment called “ProcInfo.txt”. 

- The total number of processes/machines to be executed for processing the program is taken from this file. 

- You may update this file with the desired number before starting to run the program.(eg. 4).

To execute program, use the following command:

    './causal <port number>'

Now, as you run the program. Each process takes some information from the user:

    1) What is your process ID?

    2) Do you wish to connect to any machine? (yes = 1 / no = 2)

    3) Enter the number of machines to connect:

    4) Enter the port number of Machine to connect:

The process is explained with an example of 4 processes: 

Let’s assume that the ProcInfo.txt contains the number ‘4’. So there are total 4 processes in our system.

- You need to give all the processes, a unique process ID (eg. 1, 2, 3, 4)

    - Now for process 1, say yes to connect to any machine. Give the number of machines as (n - 1). i.e 3 in our case.

    - For process 2, say yes to connect to any machine. Give the number of machines as (n - 2). i.e 2 in our case.

    - For process 3, say yes to connect to any machine. Give the number of machines as (n - 3). i.e 1 in our case.

    - For process 4, say no to connect to any machine as the number of machines is (n - 4). i.e 0 in ourcase.

- Now, follow the same order to connect to all the processes:

    - First, all the processes connect to the last i.e. 4th process. Give the port number of process 4 to process 1, then to process 2 and then to process 3. At this point, Process 4 is connected to all other processes.

    - Now, all remaining processes connect to the second last i.e. 3rd process. Give the port number of process 3 to process 1 and then to process 2. At this point, Process 4 is also connected to all other processes.

    - Finally, the remaining process connects to third last i.e. 2nd process. Give the port number of process 2 to process 1. At this point, all the processes are connected to all other processes.

    - Now all connections are completed. Follow the above procedure for any number of processes.
    
    - At any process, enter 1 to multicast messages 3 times.

# Author

**Sadham Hussian M**

**Harish Kumar A**