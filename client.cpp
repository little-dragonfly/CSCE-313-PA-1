/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Caroline Burrow
	UIN: 633008883
	Date: 9/27/25
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int m = MAX_MESSAGE;
	bool new_chan = false;
	vector<FIFORequestChannel*> channels;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi (optarg);
				break;
			case 'c':
				new_chan = true;
				break;
		}
	}

	pid_t pid = fork();

	if(pid == -1) {
		return -1;
	}

	if(pid == 0) {
		char m_str[16];
		sprintf(m_str, "%d", m);
		char* args[] = {(char*)"./server", (char*)"-m", m_str, NULL};
		execvp(args[0], args);
		exit(1);
	}

    FIFORequestChannel* chan1 = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(chan1);

	FIFORequestChannel* chan2 = nullptr;

	if(new_chan) {
		//send new channel request to the sevrer
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		chan1->cwrite(&nc, sizeof(MESSAGE_TYPE));
		//create a variable to hold the name (char* or string)
		char* buf4 = new char[16];
		//cread the response from the server
		chan1->cread(buf4, 100);
		//call the FIFORequestChannel constructor with the name of the new server
		chan2 = new FIFORequestChannel(buf4, FIFORequestChannel::CLIENT_SIDE);
		//push new_chan onto channels
		channels.push_back(chan2);
	}

	FIFORequestChannel* chan = channels.back();
	
	if(p != -1 && t != -1 && e != -1 && filename.empty()) {
	// example data point request
		char buf[MAX_MESSAGE]; // 256
		datamsg x(p, t, e);
		
		memcpy(buf, &x, sizeof(datamsg));
		chan->cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan->cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	else if(p != -1 && filename.empty()) {
		//else, if p != -1, request 1000 data points
		ofstream s("received/x1.csv");
		//loop over 1st 1000 lines
		for(int i = 0; i < 1000; ++i) {
			double time = i * 0.004;
			//send request for ecg 1
			char buf1[MAX_MESSAGE];
			datamsg x1(p, time, 1);
			memcpy(buf1, &x1, sizeof(datamsg));
			chan->cwrite(buf1, sizeof(datamsg));
			double ecg1;
			chan->cread(&ecg1, sizeof(double));
			//send request ecg 2
			char buf2[MAX_MESSAGE];
			datamsg x2(p, time, 2);
			memcpy(buf2, &x2, sizeof(datamsg));
			chan->cwrite(buf2, sizeof(datamsg));
			double ecg2;
			chan->cread(&ecg2, sizeof(double));
			//write line to received/x1.csv
			s << time << "," << ecg1 << "," << ecg2 << endl;
			//eg: ./client -p 4
		}
		s.close();
	}

	if(!filename.empty()) {
		filemsg fm(0, 0); //how big is the file
		
		int len = sizeof(filemsg) + (filename.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), filename.c_str());
		chan->cwrite(buf2, len);  // I want the file length;

		int64_t filesize = 0;
		chan->cread(&filesize, sizeof(int64_t));
		cout << "File length is: " << filesize << " bytes" << endl;

		char* buf3 = new char[m];

		ofstream s("received/" + filename);
		int64_t offset = 0;
		int chunk = 0;

		while(offset < filesize) {
			if(filesize - offset < (int64_t)m) {
				chunk = filesize - offset;
			}
			else {
				chunk = m;
			}

			filemsg fm_chunk(offset, chunk);

			memcpy(buf2, &fm_chunk, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), filename.c_str());

			chan->cwrite(buf2, len);
			chan->cread(buf3, chunk);

			s.write(buf3, chunk);

			offset += m;
		}
		s.close();

		delete[] buf2;
		delete[] buf3;
	}

	//if necessary close and detlete the new channel

	// closing the channel    
    MESSAGE_TYPE mm = QUIT_MSG;
    chan->cwrite(&mm, sizeof(MESSAGE_TYPE));

	delete chan1;
	delete chan2;

	waitpid(pid, nullptr, 0);
}