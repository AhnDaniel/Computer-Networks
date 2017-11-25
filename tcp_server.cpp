/*
 * tcp_server.cpp
 *
 *  Created on: Apr 15, 2017
 *      Author: Daniel
 */

#include <sdkddkver.h>
#include <conio.h>
#include <stdio.h>

#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>

#define SCK_VERSION2 0x0202
#define _WINSOCK_DEPRECATED_NO_WARNINGS

using namespace std;

// function prototypes
void sendFileInfo(SOCKET s, int, int&);
void bytesSent(int&, int, int);			// Displays bytes sent, and byte intervals

int main(){
	// *****************************************LOOKED UP HOW TO OPEN A TCP SOCKET
	// Winsock Startup
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2,1);
	if(WSAStartup(DllVersion, &wsaData) != 0){
		MessageBoxA(NULL, "Winsock startup failed", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	SOCKADDR_IN addr;			// address that we will bind our listening socket to
	int addrlen = sizeof(addr);	// length of the address (required for accept call)
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // broadcast locally/this pc
	addr.sin_port = htons(10000); 		// port number (arbitrary)
	addr.sin_family = AF_INET;			// IPv4 Socket

	SOCKET sListen = socket(AF_INET, SOCK_STREAM, 0);	// create socket to listen for new connections
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));			// bind the address to the socket
	listen(sListen, SOMAXCONN);	// places sListen socket in a state in which it is listening for an incoming connection. NOTE: SOMAXCONN = Socket Outstanding Max Connections

	SOCKET newConnection; 	// Socket to hold the client's connection
	newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen);	// accept a new connection
	if(newConnection == 0){	// if fail to accept client
		cout << "Failed to accept the client's connection.\n";
	}
	else{	// If client connection successfully accepted
		cout << "Client connected!\n";

		string Message = "Welcome to the server!\n";
		char Welcome[256];
		strcpy(Welcome, Message.c_str());
		send(newConnection, Welcome, sizeof(Welcome), 0);	// parameters: (socket, char pointer to array, SIZE OF DATA IN BYTES, [];
	}
	// EVERYTHING AFTER THIS IS MY OWN CODE@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	int closeProgram = 1;

	// initialize variables
	string filename;
	ifstream inFile;

	char fileRequest[256];				// buffer to store name of file that Client requests
	const int invalidFile = -1;				// send this to client if file requested DOES NOT exist
	const int validFile = 1;					// send this to client if file requested DOES exist
	bool fileExists;

	int bufferSize;							// initialize buffer size
	int fileSize;

	int packNum;
	int offset;
	int remainingBytes;
	int ack[1];								// receives ack from client, knows the next sequence number to send (byte number)

	do{
		// this loops until client asks to open a valid file
		do{
			fileExists = false;
			bufferSize = 0;

			recv(newConnection, fileRequest, sizeof(fileRequest), 0);	// "blocks" until file request is received

			filename = fileRequest;		// convert character array into string to store the name of file
			cout << "\nIncoming message. Client has requested to access file: " << filename;

			// try to open the file
			inFile.open(filename, ios::binary | ios::ate);		//open in binary in order to send it

			// check if file exists
			if(!inFile.is_open()){
				cout << "\nUnable to open the file for reading. File does not exist.\n";
				send(newConnection, (char*)&invalidFile, sizeof(int), 0);
			}
			else{
				send(newConnection, (char*)&validFile, sizeof(int), 0);
				fileExists = true;
			}
		}while(fileExists == false);

		cout << "\nFile exists.\n"
				"Sending file info to client:\n\n";

		fileSize = inFile.tellg();				// size of file we wish to send (in bytes)
		inFile.seekg(0);							// return pointer back to start of file

		// send important file info:
		sendFileInfo(newConnection, fileSize, bufferSize);

		// start sending packets
		packNum = 1;
		offset = 0;
		char buffer[bufferSize];					// allocate buffer array in which to send packets

		// start sending packets
		while(offset < fileSize){		// this is the SLIDING WINDOW part

			remainingBytes = fileSize - offset;
			if(remainingBytes >= bufferSize){			// more bytes than we have room for in the buffer
				inFile.read(buffer, bufferSize);
				send(newConnection, buffer, bufferSize, 0);							// send buffer
				recv(newConnection, (char*)&ack, sizeof(int), 0);		// receive ACK to get the next sequence number

				// display bytes sent
				bytesSent(packNum, remainingBytes, bufferSize);

				offset += bufferSize;

				cout << "Expecting ACK: " << offset + 1 << endl << endl;

				// next sequence number expected from client should match the next sequence number ready to send
				while(ack[0] != offset + 1){		// while loop that checks for ACK from client
					cout << "Incorrect ACK received. Message was not delivered. Expecting seq. number " << offset+1 << endl << endl;
					send(newConnection, buffer, bufferSize, 0);							// send buffer
					recv(newConnection, (char*)&ack, sizeof(int), 0);		// receive ACK to get the next sequence number
				}
				cout << "ACK received. Client is requesting byte/sequence number: " << ack[0] << endl;
				cout << "Correct ACK received.\n";
			}
			else{										// less remaining than we have space for
				inFile.read(buffer, remainingBytes);
				int len = send(newConnection, buffer, remainingBytes, 0);
				recv(newConnection, (char*)&ack, sizeof(int), 0);		// receive ACK to get the next sequence number

				// display bytes sent
				bytesSent(packNum, remainingBytes, bufferSize);

				offset += remainingBytes;

				cout << "Expecting ACK: " << offset + 1 << endl << endl;

				// next sequence number expected from client should match the next sequence number ready to send
				while(ack[0] != offset + 1){		// while loop that checks for ACK from client
					cout << "Incorrect ACK received. Message was not delivered. Expecting seq. number " << offset+1 << endl << endl;
					send(newConnection, buffer, sizeof(buffer), 0);							// send buffer
					recv(newConnection, (char*)&ack, sizeof(int), 0);		// receive ACK to get the next sequence number
				}
				cout << "ACK received. Client is requesting byte/sequence number: " << ack[0] << endl;
				cout << "Correct ACK received.\n";
			}

			cout << "Total number of bytes sent so far: " << offset << endl << endl;
		}

		inFile.close();

		// check to see if client wants to request another file or just close connection
		recv(newConnection, (char*)&closeProgram, sizeof(int), 0);

	}while(closeProgram == 0);

	//this part of the program executes if the client wishes to close the socket
	// therefore, it only executes if closeProgram is equal to 1;

	cout << "Server will now close.\n";
	send(newConnection, (char*)&closeProgram, sizeof(int), 0); 	// sends a value of "1"
	// wait for ack for closing
	recv(newConnection, (char*)&closeProgram, sizeof(int), 0);	// this should receive a value of "1" for ACK

	// close socket
	closesocket(newConnection);
	return 0;
}

// send important file info to client
void sendFileInfo(SOCKET s, int fileSize, int &bufferSize){
	// START SENDING IMPORTANT MESSAGES TO THE CLIENT
	// Message 1: size of file (in bytes)
	cout << "\nSize of the file: " << fileSize << " bytes.\n";
	send(s, (char*)&fileSize, sizeof(int), 0);	// send the size of the file

	bufferSize = fileSize / 12;				// size of buffer to send in a single packet
											// allocate buffer size

	// Message 2: bufferSize
	cout << "Size of the buffer: " << bufferSize << endl << endl;
	send(s, (char*)&bufferSize, sizeof(int), 0);
}

// display bytes sent
void bytesSent(int &packet, int remaining, int bufferSize){
	cout << "Sent Packet number: " << packet << endl;

	if(remaining > bufferSize)
		cout << "Bytes sent: " << bufferSize << endl;
	else
		cout << "Bytes sent: " << remaining << endl;

	packet += 1;
}

