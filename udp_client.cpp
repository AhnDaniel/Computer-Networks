/*
 * udp_client.cpp
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
#include <sstream>
#include <string>
#include <string.h>

#define SCK_VERSION2 0x0202
#define _WINSOCK_DEPRECATED_NO_WARNINGS

using namespace std;

// function prototypes
void recvFileInfo(SOCKET s, int &fileSize, int &bufferSize, SOCKADDR_IN&, int&);
void bytesRecvd(int&, int, int);
int menu();

int main(){
	// *****************************************LOOKED UP HOW TO OPEN A UDP SOCKET
	// Winsock Startup
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2,1);
	if(WSAStartup(DllVersion, &wsaData) != 0){
		MessageBoxA(NULL, "Winsock startup failed", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	SOCKADDR_IN addr;						// address that we will bind our listening socket to
	SOCKADDR_IN server_addr;				// source address from where the message came from
	int servlen = sizeof(server_addr);		// length of the source address

	// manually set server port number/ip address
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 	// broadcast locally/this pc
	server_addr.sin_port = htons(12065); 					// port number (arbitrary)
	server_addr.sin_family = AF_INET;						// IPv4 Socket

	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);	// DGRAM for UDP
	bind(s, (SOCKADDR*)&addr, sizeof(addr));			// bind the address to the socket
	// EVERYTHING AFTER THIS IS MY OWN CODE@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	// initialize variables
	// variables regarding files
	string filename;
	ofstream outFile;
	bool fileExists;

	// variables regarding reponse, whether it's from file validation or user input
	int close = 1;						// see if client wants to close their socket
	int fileExistence[] = {0};
	int response;

	char fileReq[256];

	int packNum;
	int fileSize;
	int bytesWritten;
	int bufferSize;
	int length;

do{
	do{
		fileExists = false;
		length = 0;

		cout << "What file would you like to request from the server via UDP? ";
		cin >> filename;

		// send the file name over to the server to see if file exists
		strcpy(fileReq, filename.c_str());
		sendto(s, fileReq, sizeof(fileReq), 0, (SOCKADDR*)&server_addr, servlen);

		cout << "File request sent.";

		response = recvfrom(s, (char*)&fileExistence, sizeof(int), 0, (SOCKADDR*)&server_addr, &servlen);

		if(response <= 0)
			cout << "There is no server to request a file from.\n\n";
		else{
			if(fileExistence[0] == 1){
				cout << "The file exists.\n\n";
				fileExists = true;
				outFile.open(filename, ios::binary | ios::ate);
			}
			else{
				cout << "The server does not contain the file or it is unable to open the file.\n";
				cout << "Try again.\n\n";
			}
		}
	}while(fileExists == false);			// check to see if file exists, won't be able to request until a server makes a socket

	// check if we can open a file to write to.
	if(!outFile.is_open()){
		cout << "Unable to open \"" << filename << "\" for writing.\n";
		exit(-1);
	}

	cout << "Now receiving packets.\n";
	recvFileInfo(s, fileSize, bufferSize, server_addr, servlen);			// receive file info

	char buffer[bufferSize];

	// start receiving packets
	packNum = 1;						// restart the counting of packet number
	bytesWritten = 0;					// restart the counting of written bits whenever the client requests a new file

	while(bytesWritten < fileSize){		// once BytesWritten == fileSize, the client knows that all of the file has been sent
		length = recvfrom(s, (char*)&buffer, bufferSize, 0, (SOCKADDR*)&server_addr, &servlen);

		outFile.write(buffer, length);
		bytesWritten += length;

		bytesRecvd(packNum, length, bytesWritten);		// display bytes received/written
	}// finish receiving packets

	outFile.close();

	cout << "\nFinished receiving packets.\n";

	// check if user wishes to request another file
	close = menu();

}while(close == 1);

	cout << "\n\nClosing the client socket.\n";

	closesocket(s);
	return 0;

}

// receive important file info from server
void recvFileInfo(SOCKET s, int &fileSize, int &bufferSize, SOCKADDR_IN &server_addr, int &servlen){
	// START RECEIVING THE IMPORTANT MESSAGES FROM THE SERVER
	// Message 1: receive size of file from server
	recvfrom(s, (char*)&fileSize, sizeof(int), 0, (SOCKADDR*)&server_addr, &servlen);
	cout << "\nSize of file: " << fileSize << endl;

	// Message 2: bufferSize
	recvfrom(s, (char*)&bufferSize, sizeof(int), 0, (SOCKADDR*)&server_addr, &servlen);
	cout << "Size of the buffer: " << bufferSize << endl;
}

// displays the received packet number, bytes received, and bytes written
void bytesRecvd(int &packNum, int amountOnBuffer, int bytesWritten){
	cout << "\nReceived packet number: " << packNum << endl
		 << "Bytes received: " << amountOnBuffer << endl
		 << "Bytes Written: " << bytesWritten << endl;

	packNum += 1;
}

// menu function to see if user/client wants to request another file or close the socket
int menu(){
	string input;	// user input
	double userInput = 0;

	bool correctInput = false;
	cin.ignore();

	// input error checking
	/*
	 * The conditions for proper user input are:
	 * 1. The input MUST be either 1 or 2
	 * 2. The input CAN be in double form, such as 1.0 or 2.0
	 */
	while(correctInput == false){
		cout << "\nWould you like to request another file or close the connection?\n"
				"1. Request another file\n"
				"2. Close connection socket\n\n";

		getline(cin, input);

		stringstream myStream(input);
		if((myStream >> userInput) && (userInput == 1 || userInput == 2)){
			correctInput = true;
		}
		else
			cout << "\n\nInvalid option. Please enter \"1\" or \"2\".\n";
	}

	return static_cast<int>(userInput);
}

