#include <cstdio>
#include <cassert>
#include <Windows.h>
#include <vector>
#include <set>
#include <map>
#include <tchar.h>

using namespace std;
#include "EncodedStream.h"
#include "RtmpPublisher.h"
map<int, RtmpPublisher*> publisher_map;

int streamHandlerExt(ULONG channel_id, void* context){
	static int count = 0;
	assert(count == 0);
	++count;
	printf("%d handler now!", count);
	DWORD len = 1024*1024;
	unsigned char* buf = (unsigned char*)malloc(len);
	int frame_type = 1;
	int ret;
	ret = ReadStreamData(EncodedStream::getStream(channel_id)->getHandle(), buf, &len, &frame_type);
	if (ret == 0 || ret == 1){
		printf("channel %d:\tframe_type = %d, len=%ld\n", channel_id, frame_type, (long)len); 

	}
	else{
		printf("return error, error code = %X\n", ret);
	}
	free(buf);
	--count;
	return 0;

}

void oriStreamHandler(UINT channel_id, void* context){
	static int count = 0;
	static unsigned long tick = GetTickCount();
	//assert(count == 0);
	//++count;
	//unsigned long t = GetTickCount();
	//printf("%05d:\tt:%08lu\t", count, t - tick);
	//tick = t;
	//printf("writing to yuv...");
	//EncodedStream::getStream(channel_id)->writePipeYuv(EncodedStream::getStream(channel_id)->getYuvBuf(),
	//	EncodedStream::getStream(channel_id)->getYuvBufSize());

	printf("encoding...");
	EncodedStream::getStream(channel_id)->getEncoder()->encode(EncodedStream::getStream(channel_id)->getYuvBuf());
	printf("sending stream...");
	map<int, RtmpPublisher*>::iterator publisher_it;
	if ((publisher_it = publisher_map.find(channel_id)) != publisher_map.end()){
		publisher_it->second->sendFrame(EncodedStream::getStream(channel_id)->getEncoder());
	}

	printf("\n");

	fflush(stdout);

	//--count;
}

int main(int argc, char** argv){
	set<int> channel_id_set;
	vector<EncodedStream*> streams;
	vector<STARTUPINFO*> si_vector;
	vector<PROCESS_INFORMATION*> pi_vector;
	
	EncodedStream::setOriHandler(oriStreamHandler);
	for (int i = 1; i < argc; ++i){
		channel_id_set.insert(atoi(argv[i]));
	}
	for (set<int>::iterator it = channel_id_set.begin(); it != channel_id_set.end(); ++it){
		streams.push_back(new EncodedStream(*it));
		char url[200];
		sprintf(url, "rtmp://127.0.0.1/oflaDemo/test%d", *it);
		publisher_map[*it] = new RtmpPublisher(url);
		//publisher_map[*it]->sendMetaData(EncodedStream::getStream(*it)->getEncoder());
		publisher_map[*it]->sendHeader(EncodedStream::getStream(*it)->getEncoder());
		printf("open stream %d\n", *it);

	}
	for (vector<EncodedStream*>::iterator it = streams.begin(); it != streams.end(); ++it){
		//(*it)->writePipeYuv(NULL, 0);
		(*it)->start();
		
	}

	//uncomment the following to use ffmpeg
	//for (vector<EncodedStream*>::iterator it = streams.begin(); it != streams.end(); ++it){
	//	(*it)->writePipeYuv(NULL, 0);
	//	(*it)->start();
	//	wchar_t name[100];
	//	wsprintf(name, TEXT("encode_publish.bat %d"), (*it)->getChannelId());
	//	printf("start to createProcess for channel %d\n", (*it)->getChannelId());
	//	STARTUPINFO *si = new STARTUPINFO;
	//	PROCESS_INFORMATION *pi = new PROCESS_INFORMATION;
	//	ZeroMemory( si, sizeof(STARTUPINFO));
	//	si->cb = sizeof(STARTUPINFO);
	//	ZeroMemory( pi, sizeof(PROCESS_INFORMATION));
	//	printf("start to createProcess for channel %d\n", (*it)->getChannelId());
	//	if( !CreateProcess( NULL,   // No module name (use command line)
	//		name,        // Command line
	//		NULL,           // Process handle not inheritable
	//		NULL,           // Thread handle not inheritable
	//		FALSE,          // Set handle inheritance to FALSE
	//		0,              // No creation flags
	//		NULL,           // Use parent's environment block
	//		NULL,           // Use parent's starting directory 
	//		si,            // Pointer to STARTUPINFO structure
	//		pi)           // Pointer to PROCESS_INFORMATION structure
	//		){
	//			printf( "CreateProcess failed (%d).\n", GetLastError() );
	//			return -1;
	//	}
	//	si_vector.push_back(si);
	//	pi_vector.push_back(pi);
	//}

	printf("capturing...encoding...publishing\n");

	getchar();

	for (vector<EncodedStream*>::iterator it = streams.begin(); it != streams.end(); ++it){
		(*it)->stop();
	}

	for (vector<PROCESS_INFORMATION*>::iterator it = pi_vector.begin(); it != pi_vector.end(); ++it){
		//WaitForSingleObject((*it)->hProcess, INFINITE);
		UINT ret = 0;
		TerminateProcess((*it)->hProcess, ret);
		CloseHandle((*it)->hProcess);
		delete (*it);
	}
	for (vector<STARTUPINFO*>::iterator it = si_vector.begin(); it != si_vector.end(); ++it){
		delete (*it);
	}


	for (vector<EncodedStream*>::iterator it = streams.begin(); it != streams.end(); ++it){
		delete (*it);
	}


	return 0;
}

