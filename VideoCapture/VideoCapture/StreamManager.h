#pragma once
#include <ctime>
#include <string>
#include <set>
#include <map>
#include <vector>
#include "EncodedStream.h"
#include "RtmpPublisher.h"
class StreamManager
{
public:
	StreamManager(void);
	~StreamManager(void);
	void start(std::string url, int interval, int file_interval, std::set<int> id_set);
	void stop();
	void publish(UINT channel_id);
	void livePublish(UINT channel_id);
	bool channelIsBusy(UINT channel_id);
	void wakeUp(UINT channel_id, unsigned char * yuv_ptr);
	bool isWorking();
	void cleanLivePublisher(UINT channel_id);
	void cleanPublisher(UINT channel_id);
private:

	std::set<int> m_channel_id_set;
	std::vector<EncodedStream*> m_streams;
	std::string m_url;
	int m_interval;
	int m_record_file_interval;//0 for hour, other for actual time interval

	bool m_working;
	long m_current_timestamp;
	long m_next_timestamp;
	long parseTimeStamp(struct tm* timeinfo);
	long parseTimeStamp(struct tm* timeinfo, int show_minute);
	long getCurrentTimeStamp();
	long getCurrentRecordFileTimeStamp();
	long getNextSwitchStamp();
	long getNextHourSharpStamp();
	int m_publisher_map_id;
	

	//use semaphore
	std::map<int, void*> m_publish_event;
	std::map<int, void*> m_live_publish_event;
	std::map<int, RtmpPublisher*> m_channel_publisher;
	std::map<int, RtmpPublisher*> m_channel_live_publisher;
	std::map<int, Encoder*> m_live_encoder;
	std::map<int, long> m_cur_channel_timestamp;
	std::map<int, long> m_next_channel_timestamp;
	std::map<int, long> m_next_hour_sharp_timestamp;
	std::map<int, HANDLE> m_publish_thread;
	std::map<int, HANDLE> m_live_publish_thread;
	std::map<int, bool> m_channel_busy;
	std::map<int, unsigned char*> m_yuv_buf;
};
