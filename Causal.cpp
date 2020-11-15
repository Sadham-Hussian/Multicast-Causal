/*
	1. Port number should be passed as command line argument
	2. Total no of process should be given in ProcInfo.txt file
*/

#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <list>

using namespace std;

// NewServer : A structure for all the required information of individual process is maintained
struct NewServer {
	int process_id;
	long socket_fd;
	long port;
	struct sockaddr_in new_serv_addr;
	struct hostent *new_server;
};

int vector_clock[10];  // Vector clock of the machine
struct NewServer p[10]; // Array of structure NewServer to handle information of process connected
pthread_mutex_t lock; // Mutex for synchronisation of threads
int total_process; // total process read from ProcInfo file
int my_process_id; // Process id of the current machine
int no_of_connections; // no_of_connections input
list<string> buffer_queue; // Queue for storing the buffer
int cnt = 1;

// Error : Function to print error message
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

// AcceptConnection : thread function for accepting new connection request
void *AcceptConnection(void *sockfd)				
{
	int i=0, n;
	char buffer[256];
	socklen_t clilen;
	long newsockfd[10];
	struct sockaddr_in cli_addr;
	clilen = sizeof(cli_addr);

	struct NewServer p1[10];

	while (cnt < total_process)							// Running the Accept loop till desired number of processes have connected
	{
		newsockfd[i] = accept((long)sockfd, (struct sockaddr *)&cli_addr, &clilen);

		bzero(buffer, 256);
		stringstream ss, ss1, ss2;
		ss << p[0].process_id;
		string tmpstr = ss.str();			// Converting Process ID from int to string or char Array
		strcpy(buffer, tmpstr.c_str());		// Now converting from string to const char * and copying to buffer

		n = send(newsockfd[i], buffer, strlen(buffer), 0); 		// Sending this machine's process ID to connected machine
		if (n < 0)
			error("ERROR writing to socket");

		bzero(buffer, 256);
		recv(newsockfd[i], buffer, 255, 0);		// Reading the process ID of the connected machine

		ss1 << buffer;
		string tmpstr1 = ss1.str();

		p1[i].process_id = atoi(tmpstr1.c_str());

		cout << "Connected to Machine ID <" << p1[i].process_id << ">, ";

		n = send(newsockfd[i], "ID received", 11, 0);
		if (n < 0)
			error("ERROR writing to socket");

		bzero(buffer, 256);
		recv(newsockfd[i], buffer, 255, 0);			// Reading the port number of connected machine

		ss2 << buffer;
		string tmpstr2 = ss2.str();

		p1[i].port = atoi(tmpstr2.c_str());			// Saving the port number of connected machine
		
		cout << "with port number <" << p1[i].port << ">" << endl;

		n = send(newsockfd[i], "Port received", 13, 0);
		if (n < 0)
			error("ERROR writing to socket");

		p1[i].socket_fd = newsockfd[i];			// Saving the Socket Descriptor of connected machine

		i++;				// counter for accepted connections
		cnt++;				// counter for total connections
	}

	for (int j=0; j<i; j++)		// Storing the accepted process details into main datastructure of all processes
	{
		p[no_of_connections] = p1[j];
		no_of_connections++;
	}
}

// CausalityCheck : Function to check if the received vector clock is causally related
//						to the local vector clock
int CausalityCheck(string tmpstr)
{
	int tmpArray[10];							// Array for temporarily storing received vector clock
	int index;									// Integer to store ID of sender process
	
	stringstream ss1(tmpstr);

	for(int i=0; i<total_process; i++)				// Getting the sender's vector clock from buffer into a temporary vector clock array
	{
		ss1 >> tmpArray[i];
	}

	ss1 >> index;

	int flag = 1;								// Flag to check Causal Ordering of events

	for(int i=0; i<total_process; i++)
	{
		if(i == (index - 1))
		{
			if(tmpArray[i] != (vector_clock[i] + 1))	// Check for Causal Ordering condition 1 violation
			{
				flag = 0;
				break;
			}
		}
		else
		{
			if(tmpArray[i] > vector_clock[i])	// Check for Causal Ordering condition 2 violation
			{
				flag = 0;
				break;
			}
		}
	}

	if(flag == 1)
	{
		return index;
	}
	else
	{
		return -1;
	}
}

// CheckBuffer : To check if there are messages in the buffer that obeys Causality
int CheckBuffer()
{
	int flag = 0;
	for(list<string> :: iterator s = buffer_queue.begin(); s != buffer_queue.end(); )
	{
		int result = CausalityCheck(*s);

		if(result > 0)
		{
			flag = 1;
			cout << "\n*****Message delivered from Queue for <Machine (" << result << ")>*****" << endl;

			vector_clock[result - 1]++;

			cout << "\n-----------------------------------------------" << endl;
			cout << "\tVector Clock: (";
			for(int i=0; i<(total_process - 1); i++)
			{
				cout << "[" << vector_clock[i] << "]" << ",";
			}
			cout << "[" << vector_clock[total_process - 1] << "]" << ")\n";

			cout << "==================================================" << endl;

			s = buffer_queue.erase(s);
			break;

		}
		else
		{
			s++;
		}
	}
	return flag;
}

// MulticastRecv : thread function for receiving multicast message from particular process
void *MulticastRecv(void *sockfd)
{
	char buffer[256];
	int n;
	long socket_fd = (long)sockfd;				// socket_fd for communicating with specific process

	while(1)
	{

		int tmpArray[10];						// Array for temporarily storing received vector clock from the connected machines which is multicasting
		int index;								// Integer to store ID of sender process

		bzero(buffer, 256);

		srand(time(0));
		sleep(rand()%5);

		int rc = recv(socket_fd, buffer, sizeof(buffer), 0);	// Receive message from sender with their vector clock

		if (rc < 0)
		{
			error("ERROR reading from socket");					// Printing error message if there's an error while receiving the message
		}
		else
		{
			stringstream ss;
			string tmpstr;
			ss.str("");
			ss << buffer;

			tmpstr = ss.str();

			int flag = CausalityCheck(tmpstr);

			if(flag > 0)					// If both conditions satisfied, message is delivered
			{
				cout << "\nMulticast Message Received from <Machine (" << flag << ")>" << endl;

				vector_clock[flag - 1]++;


				cout << "\n----------------------------------------------" << endl;

				cout << "\tVector Clock: (";
				for(int i=0; i<(total_process-1); i++)
				{
					cout << "[" << vector_clock[i] << "]" << ",";
				}
				cout << "[" << vector_clock[total_process - 1] << "]" << ")\n";

				cout << "===============================================" << endl;

				pthread_mutex_lock(&lock);
				if(!buffer_queue.empty())			// If the buffer has some pending messages, check if they satisfy causality now!
				{
					int res;
					do {
						res = CheckBuffer();
					} while(res == 1);				// If a message from buffer is delivered, check for the causality of remaining 
				}
				pthread_mutex_unlock(&lock);

			}
			else
			{

				if(!buffer_queue.empty())			// If a message a fails causal conditions, check if there are pending messages in buffer. Check if they satisfy causality now!
				{
					pthread_mutex_lock(&lock);
					int result, flag = 0;
					do {
						result = CheckBuffer();

						if(result == 1)				// A message from buffer is delivered!
						{
							if(flag == 0)
							{
								int recheck = CausalityCheck(tmpstr);	// If message from buffer is delivered. Re-check for causality of current message
								if (recheck > 0)		// If both conditions are satisfied, deliver the current message
								{
									flag = 1;

									cout << "\nMulticast Message Received from <Machine (" << recheck << ")>" << endl;

									vector_clock[recheck - 1]++;

									cout << "\n----------------------------------------------" << endl;

									cout << "\tVector Clock: (";
									for(int i=0; i<(total_process-1); i++)
									{
										cout << "[" << vector_clock[i] << "]" << ",";
									}
									cout << "[" << vector_clock[total_process - 1] << "]" << ")\n";

									cout << "===============================================" << endl;	
								}

							}
						}
					} while(result == 1);
					pthread_mutex_unlock(&lock);
					if(flag == 0)
					{
						cout << "\n##### Causality Violation! Message Buffered #####" << endl;
						pthread_mutex_lock(&lock);
						buffer_queue.push_back(tmpstr);				// When any condition is not satisfied, message is buffered
						pthread_mutex_unlock(&lock);
					}
				}
				else
				{
					cout << "\n##### Causality Violation! Message Buffered ####" << endl;

					pthread_mutex_lock(&lock);
					buffer_queue.push_back(tmpstr);					// When any condition is not satisfied, message is buffered
					pthread_mutex_unlock(&lock);
				}
			}
		}
	}
}

// MulticastSend : thread function for sending multicast message
void *MulticastSend(void *arg)
{
	char buffer[256];
	int n;

	while(1)
	{
		int ans;
		cout << "Press 1 to multicast";
		cin >> ans;

		if(ans == 1)
		{
			int k = 0;

			while(k < 3)
			{
				k++;
				vector_clock[my_process_id - 1]++;

				for(int i=0; i<total_process; i++)
				{
					if(i == (my_process_id - 1))
					{
						continue;
					}
					else
					{
						bzero(buffer, 256);
						stringstream ss;
						string tmpstr;

						ss.str("");
						ss << vector_clock[0];

						tmpstr = ss.str();							// Converting Process ID from int to string or char array
						strcpy(buffer, tmpstr.c_str());

						strcat(buffer, " ");

						for(int j=1; j<total_process - 1; j++)
						{
							ss.str("");
							ss << vector_clock[j];

							tmpstr = ss.str();						// Converting Process ID from int to string or char array
							strcat(buffer, tmpstr.c_str());			// Now converting from string to const char*

							strcat(buffer, " ");
						}

						ss.str("");
						ss << my_process_id;

						tmpstr = ss.str();
						strcat(buffer, tmpstr.c_str());

						n = send(p[i].socket_fd, buffer, sizeof(buffer), 0);
						if (n < 0)
							error("ERROR writing to socket");
					}
				}

				srand(time(0));
				sleep(rand()%5);
		
				cout << "\nMulticast Message Sent from <Machine (" << my_process_id << ")>" << endl;
				cout << "\n---------------------------------------------" << endl;
				cout << "\tVector Clock: (";
				for(int i = 0; i < (total_process - 1); i++)
				{
					cout << "[" << vector_clock[i] << "]" << ",";
				}
				cout << "[" << vector_clock[total_process - 1] << "]" << ")\n";
	
				cout << "=============================================" << endl;

			}
		}
	}
}


int main(int argc, char *argv[])
{
	int choice, n;
	long port;

	long sockfd, port_no, newsockfd[10];
	socklen_t clilen;
	char buffer[256];

	struct sockaddr_in server_address, client_address;

	pthread_t multi_cast_send, multi_cast_recv, new_connections; // threads for handling client requests
	
	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}

	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		cout << "mutex init has failed" << endl;
	}

	ifstream myfile("ProcInfo.txt");		// taking no of processes from file

	string line;

	if (myfile.is_open())
	{
		getline(myfile, line);	// reading file line by line.
		istringstream ss(line);
		ss >> total_process;	// converting the no of process(machines in the distributed system) into int from string
		myfile.close();
	}
	else
	{
		cout << "Unable to open file ProcInfo";
	}


	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
		error("ERROR opening socket");

	bzero((char *) &server_address, sizeof(server_address));   // bzero fills the structure sockaddr_in server_address all values to null
	port_no = atoi(argv[1]);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port_no);	// htons converts from host byte order to network byte order

	int reuse = 1;

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)	// To reuse socket address in case of crashes and failures
		perror("setsockopt(SO_REUSEADDR) failed");

	#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
		perror("setsockopt(SO_REUSEPORT) failed");
	#endif

	if(bind(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
		error("ERROR on binding");

	listen(sockfd, 10);

	cout << "What is your process ID? ";
	cin >> my_process_id;

	p[0].process_id = my_process_id;
	p[0].port = port_no;
	p[0].socket_fd = sockfd;

	int rc = pthread_create(&new_connections, NULL, AcceptConnection, (void*)sockfd);

	cout << "--------------------------------------" << endl;
	cout << "Establishing Connections " << endl;
	cout << "======================================" << endl;

	cout << "Do you wish to connect to any machine? (yes = 1 / no = 2) ";
	cin >> choice;
	cout << endl;

	if (choice == 1)
	{
		cout << "Enter the number of machines to connect: ";
		cin >> no_of_connections;

		for (int i=1; i<=no_of_connections; i++)
		{
			cout << "Enter the port number of Machine to connect: ";
			cin >> p[i].port;

			p[i].socket_fd = socket(AF_INET, SOCK_STREAM, 0);

			if(p[i].socket_fd < 0)
				error("ERROR opening socket");

			p[i].new_server = gethostbyname("localhost");

			bzero((char *) &p[i].new_serv_addr, sizeof(p[i].new_serv_addr));
			p[i].new_serv_addr.sin_family = AF_INET;
			bcopy((char *)p[i].new_server->h_addr, (char *)&p[i].new_serv_addr.sin_addr.s_addr, p[i].new_server->h_length);
			p[i].new_serv_addr.sin_port = htons(p[i].port);


			if (connect(p[i].socket_fd, (struct sockaddr *) &p[i].new_serv_addr, sizeof(p[i].new_serv_addr)) < 0)
				error("ERROR connecting");

			bzero(buffer, 256);
			recv(p[i].socket_fd, buffer, sizeof(buffer), 0);	// Reading the machine ID of connected machines 

			stringstream ss, ss1, ss2;
			ss << buffer;
			string tmpstr = ss.str();

			p[i].process_id = atoi(tmpstr.c_str());

			cout << "Connected to machine ID: " << p[i].process_id << endl;

			bzero(buffer, 256);

			ss1 << p[0].process_id;
			string tmpstr1 = ss1.str();				// Converting Process ID from int to string
			strcpy(buffer, tmpstr1.c_str());		// Converting from string to const char *

			n = send(p[i].socket_fd, buffer, strlen(buffer), 0);	// Sending the current machine's process id to connected machine
			if (n < 0)
				error("ERROR writing to socket");

			bzero(buffer, 256);
			n = recv(p[i].socket_fd, buffer, 255, 0);

			bzero(buffer, 256);

			ss2 << p[0].port;
			string tmpstr2 = ss2.str();				// Converting Port number from long to string
			strcpy(buffer, tmpstr2.c_str());		// Now converting from string to const char *

			n = send(p[i].socket_fd, buffer, strlen(buffer), 0); 		// Sending the current machine's port number to connected machine
			if (n < 0)
				error("ERROR writing to socket");

			bzero(buffer, 256);
			n = recv(p[i].socket_fd, buffer, 255, 0);
			
			cnt++;
		}
		no_of_connections++;	
	}

	while (cnt < total_process)
	{
		continue;
	}

	cout << "---------------------------------------" << endl;
	cout << "Connections Established" << endl;
	cout << "=======================================" << endl;
	cout << "Data of connected machines: " << endl;
	cout << "\tID\tPort" << endl;

//------------------------------Connections Completed----------------------------------

	for (int i=0; i<(total_process-1); i++)								// Sorting the processes according to their ID values
	{
		for(int j=0; j<(total_process-i-1); j++)
		{
			if (p[j].process_id > p[j+1].process_id)
			{
				swap(p[j], p[j+1]);
			}
		}
	}

	for (int i=0; i<total_process; i++)									// Printing the process IDs and respective port numbers
	{
		cout << "\t" << p[i].process_id << "\t" << p[i].port << "\n";
	}

	cout << "\n===================================" << endl;

	for (int i=0; i<total_process; i++)									// Spawning Multicast Receive Threads
	{
		if (i == (my_process_id - 1))
		{
			continue;
		}
		else
		{
			pthread_create(&multi_cast_recv, NULL, MulticastRecv, (void *)p[i].socket_fd);
		}
	}

	pthread_create(&multi_cast_send, NULL, MulticastSend, NULL);		// Spawning Multicast Send thread

	while(1)
	{
		continue;
	}

	pthread_mutex_destroy(&lock);				// Code never reaches here, but still following good practice to destroy mutexes

	return 0;
}	
