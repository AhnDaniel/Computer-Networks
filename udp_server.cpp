/*
 * udp_server.cpp
 *
 *  Created on: Apr 21, 2017
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
void sendFileInfo(SOCKET, int, int, SOCKADDR_IN&, int);
void bytesSent(int&, int, int);

int main(){
	// *****************************************LOOKED UP HOW TO OPEN A UDP SOCKET
	// Winsock Startup
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2,1);
	if(WSAStartup(DllVersion, &wsaData) != 0){
		MessageBoxA(NULL, "Winsock startup failed", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	SOCKADDR_IN addr;			// address that we will bind our listening socket to
	SOCKADDR_IN des_addr;		// source address from where the message came from

	int deslen = sizeof(des_addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // broadcast locally/this pc
	addr.sin_port = htons(12065); 		// port number (arbitrary)
	addr.sin_family = AF_INET;			// IPv4 Socket

	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);	// DGRAM for UDP
	bind(s, (SOCKADDR*)&addr, sizeof(addr));			// bind the address to the socket
	// EVERYTHING AFTER THIS IS MY OWN CODE@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	// initialize variables
	int bufferSize = 1849;
	char buffer[bufferSize];

	string filename;
	ifstream inFile;
	char fileRequest[256];				// buffer to store name of file that Client requests
	const int invalidFile = -1;				// send this to client if file requested DOES NOT exist
	const int validFile = 1;					// send this to client if file requested DOES exist

	bool fileExists;

	int fileSize;
	int offset;
	int remaining;

	int packNum;

	while(1){	// this will always be true in this program, because it is a connection-less socket
		fileExists = false;			// reset existence of file when client requests a new file
		offset = 0;					// reset offset when client requests a new file

		do{
			recvfrom(s, fileRequest, sizeof(fileRequest), 0, (SOCKADDR*)&des_addr, &deslen);

			filename = fileRequest;
			cout << "Incoming message. A client has requested to access file: " << filename;

			// try to open the file
			inFile.open(filename, ios::binary | ios::ate);		//open in binary in order to send it
			// check if file exists
			if(!inFile.is_open()){
				cout << "\nUnable to open the file for reading. File does not exist.\n\n";
				sendto(s, (char*)&invalidFile, sizeof(int), 0, (SOCKADDR*)&des_addr, deslen);
				inFile.close();
			}
			else{
				cout << "\nFile exists.\n\n";
				sendto(s, (char*)&validFile, sizeof(int), 0, (SOCKADDR*)&des_addr, deslen);
				fileExists = true;
			}
		}while(fileExists == false);

		// start sending packets
		// msg 1: filesize
		fileSize = inFile.tellg();
		inFile.seekg(0);

		cout << "Now sending packets.\n\n";
		sendFileInfo(s, fileSize, bufferSize, des_addr, deslen);	// send important file info

		int length;
		// start sending packets		// NO NEED TO WAIT FOR ACK BECAUSE THIS IS UDP
		packNum = 1;
		while(offset < fileSize){		// this is the SLIDING WINDOW part
			remaining = fileSize - offset;
			if(remaining > bufferSize){			// more bytes than we have room for in the buffer
				inFile.read(buffer, bufferSize);
				sendto(s, buffer, bufferSize, 0, (SOCKADDR*)&des_addr, deslen);

				// display bytes sent
				bytesSent(packNum, remaining, bufferSize);

				offset += bufferSize;
			}
			else{										// less remaining than we have space for
				inFile.read(buffer, remaining);
				sendto(s, buffer, remaining, 0, (SOCKADDR*)&des_addr, deslen);

				cout << "bufferSize: " << bufferSize;
				bytesSent(packNum, remaining, bufferSize);

				offset += remaining;
			}


			cout << "Total number of bytes sent so far: " << offset << endl << endl;
		}// finish sending packets
		inFile.close();

		cout << "Finished sending packets.\n\n";
	}

	return 0;
}

// send important file info to client
void sendFileInfo(SOCKET s, int fileSize, int bufferSize, SOCKADDR_IN &des_addr, int deslen){
	// START SENDING IMPORTANT MESSAGES TO THE CLIENT
	// Message 1: size of file (in bytes)
	cout << "\nSize of the file: " << fileSize << " bytes.\n";
	sendto(s, (char*)&fileSize, sizeof(int), 0, (SOCKADDR*)&des_addr, deslen);	// send the size of the file

	// Message 2: bufferSize
	cout << "Size of the buffer: " << bufferSize << endl << endl;
	sendto(s, (char*)&bufferSize, sizeof(int), 0, (SOCKADDR*)&des_addr, deslen);
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

