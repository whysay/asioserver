//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 111
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
using boost::asio::ip::tcp;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;


typedef struct _TCP_PACKET_H
{
	int PacketSize;
	short Cmd;
} TCP_PACKET_H;

typedef struct _TMP_PACKET
{
	TCP_PACKET_H Header;
	TCHAR strdata[1024];
} TMP_PACKET;


class session
	: public std::enable_shared_from_this<session>
{
public:
	session(tcp::socket socket)
		: socket_(std::move(socket))
	{
	}

	void start()
	{
		do_read();
	}

private:
	void do_read()
	{
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(data_, max_length),
			[this, self](boost::system::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				TMP_PACKET* packet = (TMP_PACKET*)(data_);
				Document d;
				d.Parse(packet->strdata);
				Value& ss = d["stars"];
				ss.SetInt(ss.GetInt() + 1);
				OutputDebugString("recv packet");

				Document sendJson;
				sendJson.SetObject();
				Document::AllocatorType& allocator = sendJson.GetAllocator();
				sendJson.AddMember("cmd", 1, allocator);
				sendJson.AddMember("ret", 1, allocator);

				// Convert JSON document to string
				rapidjson::StringBuffer strbuf;
				rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
				sendJson.Accept(writer);
				
				static int totalcnt = 0;
				totalcnt++;

				TCP_PACKET_H hdr;
				hdr.PacketSize = strbuf.GetSize()+ 1 + sizeof(TCP_PACKET_H);
				memcpy_s(_sendData, sizeof(TCP_PACKET_H), (void*)&hdr, sizeof(TCP_PACKET_H));
				std::cout << strbuf.GetString() << strbuf.GetSize() << " total:"<< totalcnt << std::endl;
				//std::cout << strbuf.GetSize() << std::endl;
				sprintf_s(_sendData + sizeof(TCP_PACKET_H), strbuf.GetSize()+1, "%s",strbuf.GetString());


				
				
				// 오류코드 없으면, 패킷데이터 처리..
				// 헤더검사 등의 예외처리 필요..
				//TMP_PACKET sendPkt;
				//sendPkt.Header.PacketSize = strbuf.GetSize()+;
				do_write(hdr.PacketSize);

				do_read();

			}
		});
	}

	void do_write(std::size_t length)
	{
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(_sendData, length),
			[this, self](boost::system::error_code ec, std::size_t /*length*/)
		{
			if (!ec)
			{
				//do_read();
			}
		});
	}

	tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
	char _sendData[max_length];
};

class server
{
public:
	server(boost::asio::io_service& io_service, short port)
		: acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
		socket_(io_service)
	{
		do_accept();
	}

private:
	void do_accept()
	{
		acceptor_.async_accept(socket_,
			[this](boost::system::error_code ec)
		{
			if (!ec)
			{
				std::make_shared<session>(std::move(socket_))->start();
			}

			do_accept();
		});
	}

	tcp::acceptor acceptor_;
	tcp::socket socket_;
};

int main(int argc, char* argv[])
{
	try
	{
		//if (argc != 2)
		//{
		//	std::cerr << "Usage: async_tcp_echo_server <port>\n";
		//	return 1;
		//}


		// 1. Parse a JSON string into DOM.
		const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
		Document d;
		d.Parse(json);

		// 2. Modify it by DOM.
		Value& ss = d["stars"];
		ss.SetInt(ss.GetInt() + 1);

		// 3. Stringify the DOM
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		d.Accept(writer);

		// Output {"project":"rapidjson","stars":11}
		std::cout << buffer.GetString() << std::endl;

		//Value sendJson(kObjectType);
		//{
		//	Value cmd(1);
		//	sendJson.AddMember(cmd);
		//}


		

		boost::asio::io_service io_service;

		//server s(io_service, std::atoi(argv[1]));
		server s(io_service, std::atoi("50000"));

		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
