/*
 * Buffer.h
 *
 *  Created on: May 31, 2014
 *      Author: vagrant
 */

#include <zmqpp/zmqpp.hpp>
#include <string>
#include <iostream>
#include <unistd.h>
#include <boost/lockfree/queue.hpp>
#include <atomic>
#include <stdio.h>

#include "FileReaderWriter.h"

#ifndef BUFFER_H_
#define BUFFER_H_

class Buffer {

public:
	Buffer();
	virtual ~Buffer();

	void init(zmqpp::endpoint_t const& pullEndpoint, zmqpp::endpoint_t const& pushEndpoint);

private:
	bool _passedReadSeek;
	size_t _sizeOfMesagesOnHeap;
	std::atomic<int> _lockFreeQueueSize;

	zmqpp::socket *_pullSocket;
	zmqpp::socket *_pushSocket;

	pthread_t _pullThread;
	pthread_t _pushThread;

	boost::lockfree::queue<zmqpp::message *, boost::lockfree::fixed_sized<false>> _lockFreeQueue;

	const size_t _totalAvailableMemory;

	FileReaderWriter _cacheFile;
	fpos_t _readPosition;
	fpos_t _writePosition;
	fpos_t _endDataPosition;

	size_t MAX_SIZE_OF_CACHE_FILE;

	pthread_mutex_t _lock;

private:

	static void* initPullSocketStatic(void* thisPointer);
	static void* initPushSocketStatic(void* thisPointer);
    void initPullSocket();
	void initPushSocket();
	void pullMessages();
	void pushMessages();
	void cacheMessageToFile(zmqpp::message* message);
	zmqpp::message* popCachedMessageFromFile();

    zmqpp::endpoint_t _PullEndpoint;
    zmqpp::endpoint_t _PushEndpoint;

    zmqpp::poller _poller;
};

inline size_t getTotalSystemMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

#endif /* BUFFER_H_ */
