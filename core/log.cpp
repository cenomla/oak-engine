#include "log.h"

#include <ctime>
#include <cstring>

namespace oak::log {

	Logger cout{ "stdout" };
	Logger cwarn{ "stdwarn" };
	Logger cerr{ "stderr" };

	Logger::Logger(const char* name) : name_{ name } {

	}

	Logger::~Logger() {
		for (auto& stream : streams_) {
			stream->close();
		}
	}

	void Logger::addStream(Stream *stream) {
		stream->open();
		streams_.push_back(stream);
	}

	void Logger::print(const char* text, Level level, const char* file, int line) {
		//get time info
		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		switch (level) {
		case Level::MINIMAL:
			sprintf(buffer_, "[%s]: %s\n", name_, text);
		break;
		case Level::NORMAL:
			sprintf(buffer_, "[%i:%i:%i %i/%i/%i][%s]: %s\n", 
				tm->tm_hour, tm->tm_min, tm->tm_sec, 
				tm->tm_mon+1, tm->tm_mday, 1900+tm->tm_year, 
				name_, text);
		break;
		case Level::VERBOSE:
			sprintf(buffer_, "[%i:%i:%i %i/%i/%i][%s]:%s \n\t[file]:%s:%i \n", 
				tm->tm_hour, tm->tm_min, tm->tm_sec,
				tm->tm_mon+1, tm->tm_mday, 1900+tm->tm_year, 
				name_, text, file, line);
		break;
		}
		flush();
	}

	void Logger::flush() {
		for (auto& stream : streams_) {
			stream->write(buffer_, strlen(buffer_));
		}
	}

}