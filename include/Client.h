/*
  CinderStreamClient.h
  CinderStreamClient

  Created by Vladimir Gusev on 5/19/12.
  Copyright (c) 2012 onewaytheater.us

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#pragma once

#include "cinder/Cinder.h"
#include "cinder/ConcurrentCircularBuffer.h"
#include "asio/asio.hpp"

template <class T>
class CinderStreamClient {
public:

	CinderStreamClient(std::string host, std::string service, ci::ConcurrentCircularBuffer<T*>& queueToServer):
		mIOService(), mHost(host), mService(service), mQueue(queueToServer)
	{
	}

	void setup(std::string* status, std::size_t dataSize)
	{
		mStatus = status;
		mDataSize = dataSize;
		mData.reset(new T[mDataSize]);
	}

	void run()
	{
		asio::ip::tcp::resolver resolver(mIOService);
		std::array<T, 65536> temp_buffer;
		size_t len, iSize;
		asio::ip::tcp::resolver::query query(asio::ip::tcp::v4(), mHost, mService);

		while (true)
		{
			try
			{
				asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
				asio::ip::tcp::resolver::iterator end;

				asio::ip::tcp::socket socket(mIOService);
				asio::error_code error = asio::error::host_not_found;

				while (error && endpoint_iterator != end)
				{
					socket.close();
					socket.connect(*endpoint_iterator++, error);
				}
				if (error)
					throw asio::system_error(error);

				iSize = 0;
				for (;;)
				{
					len = socket.read_some(asio::buffer(temp_buffer), error) / sizeof(T);

					//memcpy(mData, &temp_buffer[0], temp_buffer.size());
					//copy( temp_buffer.begin(), temp_buffer.begin(), mData);
					for (int i = 0; i < len; i++) {
						mData[i + iSize] = temp_buffer[i];
					}

					iSize += len;

					if (error == asio::error::eof)
						break; // Connection closed cleanly by peer.
					else if (error)
						throw asio::system_error(error); // Some other error.
				}
				mQueue.tryPushFront(mData.get());
				(*mStatus).assign("Capturing");
			}
			catch (std::exception& e)
			{
				(*mStatus).assign(e.what(), strlen(e.what()));
			}
		}
	}
private:
	asio::io_service mIOService;
	ci::ConcurrentCircularBuffer<T*>& mQueue;
	std::string mService;
	std::string mHost;
	std::string* mStatus;
	std::size_t mDataSize;
	std::unique_ptr<T[]> mData;
};
