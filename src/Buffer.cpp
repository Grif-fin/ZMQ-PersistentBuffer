/*
 * Buffer.cpp
 *
 *  Created on: May 31, 2014
 *      Author: vagrant
 */

#include "Buffer.h"
#include <pthread.h>

/*
 * Notice: All the 'cout' function calls have been commented out to get the maximum speed efficiency
 */

Buffer::Buffer():
	_pullSocket(nullptr),
	_pushSocket(nullptr),
	_PullEndpoint(""),
	_PushEndpoint(""),
	_lockFreeQueue(10000), // The lockFree queue is dynamic size (after reaches the specified initial size)
	_sizeOfMesagesOnHeap(0),
	_lockFreeQueueSize(0),
	_pullThread(0),
	_pushThread(0),
	_totalAvailableMemory(getTotalSystemMemory()),
	_cacheFile("cachedMessagesFile.cache"),
	MAX_SIZE_OF_CACHE_FILE(0),
	_passedReadSeek(true)
{
	MAX_SIZE_OF_CACHE_FILE = getTotalSystemMemory()*2;

	bool ret = _cacheFile.OpenFile();
	if(!ret)
	{
		std::cout << "Failed to create a temp file" << std::endl;
	}
	_readPosition = _cacheFile.GetCurrentSeek();
	_writePosition = _cacheFile.GetCurrentSeek();
	_endDataPosition = _cacheFile.GetCurrentSeek();

	pthread_mutex_init ( &_lock, NULL);
}

Buffer::~Buffer() {
	std::string endpoint;

	_pullSocket->get(zmqpp::socket_option::last_endpoint,endpoint);
	_pullSocket->unbind(endpoint);

	_pushSocket->get(zmqpp::socket_option::last_endpoint,endpoint);
	_pushSocket->unbind(endpoint);

	_pullSocket->close();
	_pushSocket->close();

	delete _pullSocket;
	delete _pushSocket;

	pthread_mutex_destroy(&_lock);
}

void Buffer::init(zmqpp::endpoint_t const& pullEndpoint, zmqpp::endpoint_t const& pushEndpoint)
{
	// Default local sockets
	std::string s_localHostPull = "tcp://*:4242";
	std::string s_localHostPush = "tcp://127.0.0.1:4343";

	zmqpp::endpoint_t pullPort = pullEndpoint;
	if(pullPort.empty())
	{
		pullPort = s_localHostPull;
	}

	zmqpp::endpoint_t pushPort = pushEndpoint;
	if(pushPort.empty())
	{
		pushPort = s_localHostPush;
	}

	_PullEndpoint = pullPort;
    _PushEndpoint = pushPort;

	int retThreadPull = pthread_create(&_pullThread, NULL, &initPullSocketStatic, static_cast<void*>(this));
	int retThreadPush = pthread_create(&_pushThread, NULL, &initPushSocketStatic, static_cast<void*>(this));

	if(retThreadPull != 0 || retThreadPush != 0)
	{
		printf("Internal error\n");
		return;
	}

	pthread_join(_pullThread, NULL);
	pthread_join(_pushThread, NULL);
}

void* Buffer::initPullSocketStatic(void* thisPointer)
{
    reinterpret_cast<Buffer*>(thisPointer)->initPullSocket();
    return nullptr;
}

void Buffer::initPullSocket()
{
	// Initialize the 0MQ contexts
	zmqpp::context context;

	// Generate a pull socket
	zmqpp::socket_type type = zmqpp::socket_type::pull;
	_pullSocket = new zmqpp::socket(context, type);

	// Bind to the socket
	//std::cout << "Binding to " << _PullEndpoint << "..." << std::endl;

	try
	{
	  //socket.disconnect(endpoint);
		_pullSocket->bind(_PullEndpoint);
	}
	catch(zmqpp::exception &e)
	{
		std::cout << "Exception has been thrown: " << e.what() << std::endl;
	}

	pullMessages();
}

void Buffer::pullMessages()
{
	if(_pullSocket == nullptr)
	  return;

	// receive the message
	//std::cout << "Receiving message..." << std::endl;
	bool keepPulling = true;

	while(keepPulling)
	{
	  zmqpp::message* message = new zmqpp::message();

	  // decompose the message
	  _pullSocket->receive(*message);

	  size_t messageSize = 0;
	  size_t numOfParts = message->parts();
	  for(unsigned int i = 0; i < numOfParts; ++i)
		  messageSize += message->size(i);

	  _sizeOfMesagesOnHeap += messageSize;

	  if(_sizeOfMesagesOnHeap >= _totalAvailableMemory)
	  {
		  // cache it in the file
	  }

	  if(message->size(0) > 0 && message->parts() > 0)
	  {
		  //cacheMessageToFile(message);

		  bool ret = _lockFreeQueue.push(message);
		  if(ret)
			  ++_lockFreeQueueSize;
		  else
		  {
			  //std::cout << "Internal Erro: Failed to put to internal queue message... terminating the thread" << std::endl;
			  return;
		  }
	  }
	  else
	  {
		  //std::cout << "Received empty message... discarding" << std::endl;
	  }

	  //std::cout << "Received message with total size of: " << messageSize << std::endl;
	}
}

void Buffer::cacheMessageToFile(zmqpp::message* message)
{
	pthread_mutex_lock(&_lock);

	size_t messageSize = 0;
	size_t numOfParts = message->parts();
	for(unsigned int i = 0; i < numOfParts; ++i)
	  messageSize += message->size(i);

	messageSize += sizeof(size_t) * numOfParts;

	_cacheFile.SetCurrentSeek(_readPosition);
	size_t bytesUptoRead = _cacheFile.BytesUptoCurrentSeek();

	_cacheFile.SetCurrentSeek(_writePosition);
	if(_cacheFile.BytesUptoCurrentSeek() + messageSize > MAX_SIZE_OF_CACHE_FILE)
	{
		_endDataPosition = _writePosition;
		_cacheFile.SeekToBegin();
		_writePosition = _cacheFile.GetCurrentSeek();
		_passedReadSeek = false;
	}

	if( !_passedReadSeek || (_passedReadSeek && (_cacheFile.BytesUptoCurrentSeek() + messageSize < bytesUptoRead || _cacheFile.GetCurrentFileSize() == 0)))
	{
		// Write the size of the message
		_cacheFile.WriteBinaryData(&numOfParts, sizeof(size_t));

		for(unsigned int i=0; i< message->parts() ; ++i)
		{
			// Write the size of the part
			size_t sizeOfPart = message->size(i);
			bool ret = _cacheFile.WriteBinaryData(&sizeOfPart, sizeof(size_t));
			if(!ret)
			{
				//std::cout << "Failed to write to cachedMessages file" << std::endl;
				pthread_mutex_unlock(&_lock);
				return;
			}

			// Write the data of the part
			void const * rawPart = message->raw_data(i);

			ret = _cacheFile.WriteBinaryData(rawPart, message->size(i));
			if(!ret)
			{
				//std::cout << "Failed to write to cachedMessages file" << std::endl;
				pthread_mutex_unlock(&_lock);
				return;
			}
		}

		_writePosition = _cacheFile.GetCurrentSeek();
	}
	pthread_mutex_unlock(&_lock);
}

zmqpp::message* Buffer::popCachedMessageFromFile()
{
	pthread_mutex_lock(&_lock);

	zmqpp::message* message = nullptr;

	_cacheFile.SetCurrentSeek(_readPosition);

	if(_readPosition.__pos != _writePosition.__pos)
	{
		message = new zmqpp::message();
		size_t numOfParts = 0;
		_cacheFile.ReadBinaryData(reinterpret_cast<unsigned char*>(&numOfParts), sizeof(size_t));

		size_t sizeOfPart = 0;
		for(size_t i = 0; i< numOfParts; ++i)
		{
			sizeOfPart = 0;
			_cacheFile.ReadBinaryData(reinterpret_cast<unsigned char*>(&sizeOfPart), sizeof(size_t));

			unsigned char rawPart[sizeOfPart];

			_cacheFile.ReadBinaryData(rawPart, sizeOfPart);
			message->add(rawPart, sizeOfPart);
		}

		_readPosition = _cacheFile.GetCurrentSeek();
		if(_readPosition.__pos != _writePosition.__pos)
		{
			_passedReadSeek = true;
			_cacheFile.SeekToBegin();
			_readPosition = _cacheFile.GetCurrentSeek();
		}
	}
	pthread_mutex_unlock(&_lock);
	return message;
}

void* Buffer::initPushSocketStatic(void* thisPointer)
{
    reinterpret_cast<Buffer*>(thisPointer)->initPushSocket();
    return nullptr;
}

void Buffer::initPushSocket()
{
	// initialize the 0MQ context
	zmqpp::context context;

	// generate a push socket
	zmqpp::socket_type type = zmqpp::socket_type::push;
	_pushSocket = new zmqpp::socket(context, type);

	// open the connection
	//std::cout << "Opening connection to " << _PushEndpoint << "..." << std::endl;

	try
	{
		_pushSocket->bind(_PushEndpoint);
	}
	catch(zmqpp::exception &e)
	{
		std::cout << "Exception has been thrown: " << e.what() << std::endl;
	}

	pushMessages();
}

void Buffer::pushMessages()
{
	if(_pushSocket == nullptr)
		return;

	// send a message
	zmqpp::message* messageToSend = nullptr;
	bool keepPushing = true;

	while(keepPushing)
	{
		//messageToSend = popCachedMessageFromFile();
		if(_lockFreeQueue.pop(messageToSend))//messageToSend != nullptr)
		{
			_lockFreeQueueSize--;
			bool ret = false;
			for(int i = 0; !ret && i < 10  ; ++i)	// retry 10 times
			{
				ret = _pushSocket->send(*messageToSend, true);

				if(ret)
				{
					size_t messageSize = 0;
					size_t numOfParts = messageToSend->parts();
					for(unsigned int i = 0; i < numOfParts; ++i)
					  messageSize += messageToSend->size(i);

					_sizeOfMesagesOnHeap -= messageSize;

					//std::cout << "Message with " << messageToSend->parts() << "parts sent successfully" << std::endl;
				}
				//else
					//std::cout << "Message sent failed... retrying" << std::endl;*/
			}
			if(!ret)
			{
				//std::cout << "Message sent failed after 10 retries!" << std::endl;
			}

			delete messageToSend;
		}
		else
		{
			//std::cout << "Out bound port is starving" << std::endl;
		}
	}
}
