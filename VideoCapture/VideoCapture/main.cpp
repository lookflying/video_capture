#include <cstdio>
#include <cassert>
#include <Windows.h>
#include <vector>
#include <set>
#include <tchar.h>

using namespace std;
#include "EncodedStream.h"

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
	assert(count == 0);
	++count;
	printf("writing to yuv...");
	EncodedStream::getStream(channel_id)->writePipeYuv(EncodedStream::getStream(channel_id)->getYuvBuf(),
		EncodedStream::getStream(channel_id)->getYuvBufSize());
	printf("encoding...");
	printf("sending stream...");
	printf("\n");


	--count;
}

int main(int argc, char** argv){
	set<int> channel_id_set;
	vector<EncodedStream*> streams;

	EncodedStream::setOriHandler(oriStreamHandler);
	for (int i = 1; i < argc; ++i){
		channel_id_set.insert(atoi(argv[i]));
	}
	for (set<int>::iterator it = channel_id_set.begin(); it != channel_id_set.end(); ++it){
		streams.push_back(new EncodedStream(*it));
		printf("open stream %d\n", *it);
	}
	for (vector<EncodedStream*>::iterator it = streams.begin(); it != streams.end(); ++it){
		(*it)->start();

	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	




	getchar();

	for (vector<EncodedStream*>::iterator it = streams.begin(); it != streams.end(); ++it){
		(*it)->stop();
	}

	for (vector<EncodedStream*>::iterator it = streams.begin(); it != streams.end(); ++it){
		delete (*it);
	}

	return 0;
}

