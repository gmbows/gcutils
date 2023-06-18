#include "gcutils.h"
#include "error.h"

#include <cmath>
#include <condition_variable>
#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>
#include <filesystem>
#include <random>

#include <libgen.h>
#include <cstring>

std::mutex print_mutex;

namespace gcutils {
	TaskManager print_manager;
	std::condition_variable print_cv;
	std::stringstream statement;
	std::stringstream log_statement;

	//Defaults to appending to a log file
	std::function<void(std::string)> log_handler = [](std::string s) {
		std::string fn = "log.txt";
		std::string lsnl = get_timestamp()+": "+s+"\n";
		append_to_file(fn,(unsigned char*)lsnl.data(),lsnl.size());
	};


	std::string command = "";
	std::string cursor = "";

	size_t import_file(std::filesystem::path path,unsigned char* &data) {

		if(!gcutils::file_exists(path)) {
			gcutils::log("import_file: file not found");
			return false;
		}

		std::filesystem::path p(path);

		std::ifstream image(p.string(),std::ios::binary);

		//Seek to EOF and check position in stream
		image.seekg(0,image.end);
		size_t size = image.tellg();
		image.seekg(0,image.beg);

		data = (unsigned char*)malloc(size);

		char c;
		int i = 0;
		while(image >> std::noskipws >> c) {
			 data[i++] = c;
		}

		image.close();
		return size;
	}

	std::vector<char> import_file(std::filesystem::path path) {

		std::vector<char> v;

		if(!gcutils::file_exists(path)) {
			gcutils::log("import_file: file not found");
			return {};
		}

		std::ifstream image(path,std::ios::binary);

		char c;
		int i = 0;
		while(image >> std::noskipws >> c) {
			 v.push_back(c);
		}

		image.close();
		return v;
	}

	bool export_file(std::filesystem::path path,unsigned char* bytes,size_t size) {
		gcutils::sanitize_path(path);

		ensure_path(path);

		std::ofstream file(path,std::ios::binary);

		for(int i=0;i<size;i++) {
			file << bytes[i];
		}
		file.close();
		return true;
	}

	bool append_to_file(std::filesystem::path path,unsigned char* bytes,size_t size) {
		gcutils::sanitize_path(path);

		ensure_path(path);

		std::ofstream file(path,std::ios::binary | std::ios_base::app);

		if(!file.is_open()) {
//			gcutils::log("append_to_file: Unable to open file ",filename);
			create_file(path);
//			return false;
		}

		std::stringstream s;
		s.rdbuf()->pubsetbuf((char*)bytes,size);

		file << s.rdbuf();

//		for(int i=0;i<size;i++) {
//			file << bytes[i];
//		}

		file.close();
		return true;
	}

	bool file_exists(std::filesystem::path path) {
		std::ifstream image(path,std::ios::binary);
		bool suc = image.is_open();
		image.close();
		return suc;
	}

	void clear_buffer(void* buf,int len) {
		memset(buf,'\0',len);
	}

	std::vector<std::string> split(const std::string &s,char token) {
		std::vector<std::string> v;
		std::string run = "";
		char e;

		for(int i=0;i<s.size();++i) {
			e = s[i];
			if(e == token) {
				if(run.size() == 0) continue;
				v.push_back(run);
				run = "";
			} else {
				run += e;
			}
		}
		v.push_back(run);
		return v;
	}

	std::vector<std::string> getlines(std::vector<char> data) {
		std::vector<std::string> v;
		std::string entry = "";
		for(auto &e : data) {
			if(e == '\n') {
				v.push_back(entry);
				entry = "";
			} else {
				entry += e;
			}
		}
		v.push_back(entry);
		return v;
	}

	void print() {
		print("");
	}

	std::string simple_encrypt(std::string s) {
		std::string enc;
		for(int i=0;i<s.size();i++) {
			byte_t c = s[i];
			byte_t t = (c + s.size() + 8*i);
			enc += t;
		}
		return "r"+as_hex(enc);
	}

	std::string simple_decrypt(std::string s) {
		std::string dec;
		s = hex_decode_2b(s);
		s.erase(0);
		for(int i=0;i<s.size();i++) {
			byte_t c = s[i];
			byte_t t =(c - s.size() - 8*i);
			dec += t;
		}
		return dec;
	}

	size_t file_size(std::filesystem::path path) {
		if(!file_exists(path)) {
			gcutils::log("file_size: file ",path," not found");
			return false;
		}

		std::ifstream image(path,std::ios::binary);

		//Seek to EOF and check position in stream
		image.seekg(0,image.end);
		size_t size = image.tellg();
		image.close();
		return size;

	}

	std::string conv_bytes(size_t bytes) {
		std::string suffix;
		double b_conv;
		if(bytes < 1000) {
			return std::to_string(bytes) + " bytes";
		} else if(bytes < 1000000) {
			b_conv = bytes/1000.0f;
			suffix = "kB";
		} else if(bytes < 1000000000) {
			b_conv = bytes/1000000.0f;
			suffix = "MB";
		} else if(true or bytes < 1000000000000) {
			b_conv = bytes/1000000000.0f;
			suffix = "GB";
		}
		size_t b_i = (size_t)b_conv;
		size_t b_m = 100*(b_conv-b_i);

		std::string b = std::to_string(b_i)+"."+std::to_string(b_m)[0];
		return b+suffix;
	}

	std::string random_hex_string(int len) {
		std::stringstream s;
		for(int i=0;i<len;i++) s << std::hex << rand()%16;
		return s.str();
	}

	bool create_file(std::filesystem::path path) {
		if(file_exists(path)) return true;
		bool success = false;
		std::ofstream f(path.string(),std::ios::binary);
		success = f.is_open();
		f.close();
		return success;
	}

	std::string as_hex(int s)	{
		std::stringstream out;
		out << std::hex << s;
		return out.str();
	}

	int wrap(int n, int lb,int ub) {
		return lb + (n - lb) % (ub - lb);
	}

	std::string as_hex(std::string s) {
		std::stringstream out;
		for(auto c : s) {
			out << std::hex << (int)c;
		}
		return out.str();
	}

	char hex_decode_char(std::string s)	{
		int val = 0;
		int col = 1;
		for(byte_t c : s) {
			if(c >= 'a') {
				val += (pow(16,col))*(c-'a'+10);
			} else {
				val += (pow(16,col))*(c-48);
			}
			col--;
		}
		return val;
	}

	std::string hex_decode_2b(std::string h) {
		if(h.size()%2) {
			h = "0"+h;
		}
		std::string d;
		for(int i=0;i<h.size();i+=2) {
			std::string pair;
			pair += h[i];
			pair += h[i+1];
			d += hex_decode_char(pair);
		}
		return d;
	}

	std::string get_device_fingerprint() {
		return "";
	}

	std::string get_timestamp()	{
		time_t now = time(0);
		char* dt = ctime(&now);
		std::string s(dt);
		return s.substr(0,s.size()-1);
	}

	std::vector<std::filesystem::path> dir_listing(std::filesystem::path path) {
		std::vector<std::filesystem::path> files;
		for(const auto & entry : std::filesystem::recursive_directory_iterator(path)) {
			try {
				if(entry.is_regular_file()) files.push_back(entry.path());
			} catch(const std::exception &e) {
				cthrow(ERR_WARN,e.what());
			}
		}
		return files;
	}

	bool make_directory(std::filesystem::path path) {
		std::error_code ec;
		if(file_exists(path)) {
			return true;
		}
		bool success = std::filesystem::create_directory(path,ec);
		if(!success) log("Error creating directory: '",path,"'");
		return success;
	}

	std::string get_filename(std::filesystem::path path) {
		return path.filename().string();
	}

	std::filesystem::path make_relative_to(std::filesystem::path path,std::filesystem::path relpath,bool include_root) {
		std::filesystem::path last;
		for(auto &p : std::filesystem::path(relpath)) last = p;
		auto rel = std::filesystem::proximate(path,std::filesystem::path(relpath));
		if(include_root == false) return rel;
		last /= rel;
		return last;
	}

	void ensure_path(std::filesystem::path p) {
		auto parents = p.parent_path();
		if(parents.empty() == false) {
			std::filesystem::create_directories(parents);
		}
	}

	void sanitize_path(std::filesystem::path &path) {
		std::string sp = path.string();
		std::replace(sp.begin(),sp.end(),'\\','/');
		path = std::filesystem::path(sp);
	}

	void sanitize_path(std::string &sp) {
		std::replace(sp.begin(),sp.end(),'\\','/');
	}

	std::vector<std::string> dir_listing_str(std::filesystem::path path) {
		auto listing = gcutils::dir_listing(path);
		std::vector<std::string> s;
		for(auto &e : listing) {
			auto t = e.string();
			sanitize_path(t);
			s.push_back(t);
		}
		return s;
	}

}
