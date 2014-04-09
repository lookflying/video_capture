#include "StreamManager.h"
#include <windows.h> 
#include <cstdlib>
#include <cstring>
using namespace std;


StreamManager::StreamManager(void)
{
	m_interval = 0;
}

StreamManager::~StreamManager(void)
{
	
}


long StreamManager::parseTimeStamp(struct tm* ti, int show_minute){
	long timestamp = 0;
	timestamp = ((ti->tm_year + 1900) % 100) * 100000000 + (ti->tm_mon + 1) * 1000000 + (ti->tm_mday) * 10000 + ti->tm_hour * 100;
	if (show_minute != 0){
		timestamp += ti->tm_min;
	}
	return timestamp;
}

long StreamManager::parseTimeStamp(struct tm* ti){
	long timestamp = 0;
	timestamp = ((ti->tm_year + 1900) %100) * 100000000 + (ti->tm_mon + 1) * 1000000 + (ti->tm_mday) * 10000 + ti->tm_hour * 100;
	if (m_interval != 0){
		timestamp += ti->tm_min;
	}
	return timestamp;
}

long StreamManager::getCurrentTimeStamp(){
	time_t t;
	struct tm * ti;
	time(&t);
	ti = localtime(&t);
	return parseTimeStamp(ti);
}

long StreamManager::getCurrentRecordFileTimeStamp(){
	time_t t;
	struct tm * ti;
	time(&t);
	ti = localtime(&t);
	return parseTimeStamp(ti, m_record_file_interval);
}

long StreamManager::getNextSwitchStamp(){
	time_t t;
	struct tm * ti;
	time(&t);
	ti = localtime(&t);
	if (m_interval == 0){
		ti->tm_hour = ti->tm_hour + 1;
	}else{
		ti->tm_min = ti->tm_min + m_interval;
	}
	t = mktime(ti);
	ti = localtime(&t);
	return parseTimeStamp(ti);
}

long StreamManager::getNextHourSharpStamp(){
	time_t t;
	struct tm * ti;
	time(&t);
	ti = localtime(&t);
	ti->tm_hour = ti->tm_hour + 1;
	t = mktime(ti);
	ti = localtime(&t);
	return parseTimeStamp(ti, 0);
}



void StreamManager::wakeUp(UINT channel_id, unsigned char * yuv_ptr){
	m_channel_busy[channel_id] = true;
	memcpy(m_yuv_buf[channel_id], yuv_ptr, WIDTH * HEIGHT * 3 / 2);
	m_channel_busy[channel_id] = false;
	SetEvent(m_publish_event[channel_id]);
	SetEvent(m_live_publish_event[channel_id]);
}
bool StreamManager::channelIsBusy(UINT channel_id){
	return m_channel_busy[channel_id];
}
void StreamManager::publish(UINT channel_id){
	while(m_working){
		WaitForSingleObject(m_publish_event[channel_id], -1);
		if(!m_working){
			return;
		}
		m_channel_busy[channel_id] = true;
		map<int, RtmpPublisher*>::iterator publisher_it;
		publisher_it = m_channel_publisher.find(channel_id);
		long cur_ts = getCurrentTimeStamp();
		if (cur_ts >=  m_next_channel_timestamp[channel_id]){
			char url[1024];				
			if (publisher_it != m_channel_publisher.end()){
				delete publisher_it->second;	
			}
			sprintf(url, "%s/channel%u_%ld", m_url.c_str(), channel_id, getCurrentRecordFileTimeStamp());
			printf("%s\n", url);
			m_channel_publisher[channel_id] = new RtmpPublisher(url);
			EncodedStream::getStream(channel_id)->renewEncoder();
			m_channel_publisher[channel_id]->sendHeader(EncodedStream::getStream(channel_id)->getEncoder());
			m_cur_channel_timestamp[channel_id] = cur_ts;
			m_next_channel_timestamp[channel_id] = getNextSwitchStamp();
		}
		else if(cur_ts >= m_next_hour_sharp_timestamp[channel_id]){
			char url[1024];
			if (publisher_it != m_channel_publisher.end()){
				delete publisher_it->second;
			}
			sprintf(url, "%s/channel%u_%ld", m_url.c_str(), channel_id, getCurrentRecordFileTimeStamp());
			printf("%s\n", url);
			m_channel_publisher[channel_id] = new RtmpPublisher(url);
			EncodedStream::getStream(channel_id)->renewEncoder();
			m_channel_publisher[channel_id]->sendHeader(EncodedStream::getStream(channel_id)->getEncoder());
			m_cur_channel_timestamp[channel_id] = cur_ts;
			m_next_hour_sharp_timestamp[channel_id] = getNextHourSharpStamp();
		}
		else{
			EncodedStream::getStream(channel_id)->getEncoder()->encode(m_yuv_buf[channel_id]);
			publisher_it->second->sendFrame(EncodedStream::getStream(channel_id)->getEncoder());
		}
		m_channel_busy[channel_id] = false;
	}
}

void StreamManager::cleanPublisher(UINT channel_id){
	m_next_channel_timestamp[channel_id] = getCurrentTimeStamp();
	m_next_hour_sharp_timestamp[channel_id] = getNextHourSharpStamp();

}
void StreamManager::livePublish(UINT channel_id){
	while(m_working){
		WaitForSingleObject(m_live_publish_event[channel_id], -1);
		if(!m_working){
			return;
		}
		map<int, RtmpPublisher*>::iterator publisher_it;
		publisher_it = m_channel_live_publisher.find(channel_id);
		if (publisher_it == m_channel_live_publisher.end()){
			char url[1024];
			sprintf(url, "%s/channel%u_live", m_url.c_str(), channel_id);
			printf("%s\n", url);
			m_channel_live_publisher[channel_id] = new RtmpPublisher(url, 1);
			m_live_encoder[channel_id] = new Encoder(WIDTH, HEIGHT,  FPS, 4);
			m_channel_live_publisher[channel_id]->sendHeader(m_live_encoder[channel_id]);
		}else{
			m_live_encoder[channel_id]->encode(m_yuv_buf[channel_id]);
			m_channel_live_publisher[channel_id]->sendFrame(m_live_encoder[channel_id]);
		}
	}
}
void StreamManager::cleanLivePublisher(UINT channel_id){
	map<int, Encoder*>::iterator it;
	it = m_live_encoder.find(channel_id);
	if (it != m_live_encoder.end()){
		delete it->second;
		m_live_encoder.erase(it);
	}
}

typedef struct publish_param{
	StreamManager* ptr;
	UINT id;
} publish_param_t;

DWORD WINAPI doPublish(LPVOID pM){
	publish_param_t* param = (publish_param_t*) pM;
	
	while (param->ptr->isWorking()){
		try{
			param->ptr->publish(param->id);
		}
		catch (exception e){
			printf("publish %ld crashed\n", param->id);
			param->ptr->cleanPublisher(param->id);
		}
	}
	free(param);
	return 0;
}

DWORD WINAPI doLivePublish(LPVOID pM){
	publish_param_t* param = (publish_param_t*) pM;
	while (param->ptr->isWorking()){
		try{
			param->ptr->livePublish(param->id);
		}
		catch(exception e){
			printf("live publish %ld crashed\n", param->id);
			param->ptr->cleanLivePublisher(param->id);
		}
	}
	free(param);
	return 0;
}
void StreamManager::start(string url, int interval, int file_interval, set<int> id_set){
	m_interval = interval;
	m_channel_id_set = id_set;
	m_url = url;
	long cur_ts = getCurrentTimeStamp();
	long next_ts = getNextSwitchStamp();
	m_working = true;
	for (set<int>::iterator it = m_channel_id_set.begin(); it != m_channel_id_set.end(); ++it){
		m_streams.push_back(new EncodedStream(*it));
		m_publish_event[*it] = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_cur_channel_timestamp[*it] = cur_ts;
		m_next_channel_timestamp[*it] = cur_ts;//make publish method create a new publish immediately
		m_next_hour_sharp_timestamp[*it] = getNextHourSharpStamp();
		m_yuv_buf[*it] = new unsigned char[WIDTH * HEIGHT * 3 / 2];
		m_channel_busy[*it] = false;
		publish_param_t * param = new publish_param_t;
		param->ptr = this;
		param->id = *it;
		m_publish_thread[*it] = CreateThread(NULL, 0, doPublish, param, 0, NULL);
		publish_param_t * live_param = new publish_param_t;
		live_param->ptr = this;
		live_param->id = *it;
		m_live_publish_thread[*it] = CreateThread(NULL, 0, doLivePublish, live_param, 0, NULL);

	}
	for (vector<EncodedStream*>::iterator it = m_streams.begin(); it != m_streams.end(); ++it){
		(*it)->start();
	}
}

bool StreamManager::isWorking(){
	return m_working;
}


void StreamManager::stop(){
	m_working = false;
	for (vector<EncodedStream*>::iterator it = m_streams.begin(); it != m_streams.end(); ++it){
		(*it)->stop();
	}
	for (map<int, void*>::iterator it = m_publish_event.begin(); it != m_publish_event.end(); ++it){
		SetEvent(it->second);
	}
	for (map<int, HANDLE>::iterator it = m_publish_thread.begin(); it != m_publish_thread.end(); ++it){
		WaitForSingleObject(it->second, -1);	
	}
	for (map<int, RtmpPublisher*>::iterator it = m_channel_publisher.begin(); it != m_channel_publisher.end(); ++it){
		delete it->second;
	}
	m_channel_publisher.clear();

	for (map<int, unsigned char*>::iterator it = m_yuv_buf.begin(); it != m_yuv_buf.end(); ++it){
		delete it->second;
	}
	
	m_yuv_buf.clear();
	for (map<int, void*>::iterator it = m_live_publish_event.begin(); it != m_live_publish_event.end(); ++it){
		SetEvent(it->second);
	}
	for (map<int, HANDLE>::iterator it = m_live_publish_thread.begin(); it != m_live_publish_thread.end(); ++it){
		WaitForSingleObject(it->second, -1);	
	}
	for (map<int, Encoder*>::iterator it = m_live_encoder.begin(); it != m_live_encoder.end(); ++it){
		delete it->second;
	}
	m_live_encoder.clear();

	for (vector<EncodedStream*>::iterator it = m_streams.begin(); it != m_streams.end(); ++it){
		delete (*it);
	}

	m_streams.clear();
}


