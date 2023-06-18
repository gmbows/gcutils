#pragma once

#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <functional>
#include <random>

#include <memory>
#include <queue>

#include "error.h"


extern std::mutex print_mutex;

typedef unsigned char byte_t;

namespace gcutils {

//===========
// Data structures
//==============

	typedef std::function<void(void)> task_t;
	//Runs bound functions asynchronously
	struct TaskManager {
		bool active = false;
		std::queue<task_t> tasks;
		std::mutex mtx;
		std::condition_variable cv;
		std::thread thr;
		void run() {
//			gcutils::print("Task Manager starting");
			while(this->active) {
				std::unique_lock<std::mutex> rlk(mtx);
				while(this->tasks.empty()) {
					this->cv.wait(rlk);
				}
				auto t = this->tasks.front();
				this->tasks.pop();
				rlk.unlock();
				t();
			}
//			gcutils::print("Task Manager exiting");
		}
		void add_task(task_t task) {
			std::unique_lock<std::mutex> lk(mtx);
			this->tasks.push(task);
			lk.unlock();
			this->cv.notify_one();
		}
		TaskManager() {
			this->active = true;
			this->thr = std::thread(&TaskManager::run,this);
	//		std::thread(&TaskManager::run,this).detach();
		} ~TaskManager() {
			this->thr.join();
		}
	};

	extern TaskManager print_manager;

	//=========
	// Prints
	//=========

	int wrap(int,int,int);
	extern std::function<void(std::string)> log_handler;

	template <class T>
	void print(T t) {
		{
			std::lock_guard<std::mutex> m{print_mutex};
			std::cout << t << std::endl;
		}
//		print_manager.add_task([t](){
//			std::cout << t << std::endl;
//		});
	}

	template <class T,class... Args>
	void print(T t,Args... args) {
		{
			std::lock_guard<std::mutex> m{print_mutex};
			std::cout << t;
		}
//		print_manager.add_task([t](){
//			std::cout << t;
//		});
		print(args...);
	}

	extern std::stringstream log_statement;
	template <class T>
	void log(T t) {
//		{
//			std::lock_guard<std::mutex> m{print_mutex};
//			log_statement << t;
//			log_handler(log_statement.str());
//		}
		print_manager.add_task([t](){
			log_statement << t;
			log_handler(log_statement.str());
//			print(log_statement.str());
			log_statement.str("");
		});
	}

	template <class T,class... Args>
	void log(T t,Args... args) {
//		{
//			std::lock_guard<std::mutex> m{print_mutex};
//			log_statement << t;
//		}
		print_manager.add_task([t](){
			log_statement << t;
		});

		gcutils::log(args...);
	}

	void print();

	//================
	//  Generic utils
	//================

	std::string get_timestamp();

	std::string get_device_fingerprint();

	//Byte utils/hex

	std::string conv_bytes(size_t);
	std::string random_hex_string(int);
	std::string as_hex(int);
	std::string as_hex(std::string);
	char hex_decode_char(std::string);
	std::string hex_decode_2b(std::string);
	std::string simple_encrypt(std::string);
	std::string simple_decrypt(std::string);

	template <typename T>
	size_t get_uid() {
		const size_t _size = sizeof(T);
		static std::random_device dev;
		static std::mt19937 rng(dev());
		std::uniform_int_distribution<T> dist(0, std::pow(2,_size*8)-1);
		return dist(rng);
	}

	//Wait on a variable
	struct Waiter {
		std::condition_variable cv;
		std::mutex m;
		bool *ready;
		bool override = false;
		void await() {
			while(this->override == false and *this->ready == false) {
				{
					std::unique_lock<std::mutex> lk(this->m);
					this->cv.wait(lk);
				}
			}
			this->override = false;
		}
		void wake_one() {
			this->cv.notify_one();
		}
		void wake_all() {
			this->cv.notify_all();
		}
		void wait() {
			if(*this->ready == false) this->await();
		}
		bool wait(unsigned timeout) {
			bool timed_out = false;
			//Awakens self if unready by timeout
			std::thread([&,this](){
				std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
				if(*this->ready == false) {
					timed_out = true;
					this->override = true;
					this->wake_one();
				}
			}).detach();
			this->wait();
			return timed_out;
		}
		Waiter(bool *_ready) {
			this->ready = _ready;
		}
	};

	//=========
	// VECTOR
	//=========

	std::string vector_to_string(const std::vector<unsigned char> v);

	template <class T>
	int v_find(std::vector<T> v, T t) {
		for(int i=0;i<v.size();i++) {
			T e = v.at(i);
			if((char*)e == (char*)t) return i;
		}
		return -1;
	}

	template <class T>
	void remove(std::vector<T> &v, const T &t) {
		unsigned idx = v_find(v,t);
		if(idx >= 0) {
			v.erase(v.begin()+idx);
		}
	}

	template <typename T>
	bool contains(std::vector<T> v, T t) {
		for(auto e : v) {
			if(e == t) return true;
		}
		return false;
	}

	template <typename T>
	bool contains(std::vector<T*> v, T *t) {
		for(auto &e : v) {
			if((char*)e == (char*)t) return true;
		}
		return false;
	}

	template <typename K,class V>
	bool contains(const std::map<K,V> &m,const K &k) {
		if(m.find(k) == m.end()) {
			return false;
		} else {
			return true;
		}
	}

	template <typename K,class V>
	bool remove(std::map<K,V> &m,const K &k) {
		if(contains(m,k) == false) return false;
		m.erase(k);
		return true;
	}



	std::vector<std::string> split(const std::string&,char);

	//=========
	// ARRAY
	//=========

	void clear_buffer(void*,int);

	//=========
	// I/O
	//=========

	std::string get_filename(std::filesystem::path path);
	size_t import_file(std::filesystem::path path,unsigned char*&);
	std::vector<char> import_file(std::filesystem::path path);
	std::vector<std::string> getlines(std::vector<char>); //Converts char vector into string vector split by newline
	bool export_file(std::filesystem::path path,unsigned char*,size_t size);
	bool append_to_file(std::filesystem::path path,unsigned char*,size_t size);
	bool file_exists(std::filesystem::path path);
	size_t file_size(std::filesystem::path path);
	std::vector<std::filesystem::path> dir_listing(std::filesystem::path path);
	std::vector<std::string> dir_listing_str(std::filesystem::path path);
	std::filesystem::path make_relative_to(std::filesystem::path path,std::filesystem::path relpath,bool include_root = true);

	void sanitize_path(std::string&);
	void sanitize_path(std::filesystem::path&);
	void ensure_path(std::filesystem::path);

	bool make_directory(std::string);
	bool create_file(std::filesystem::path path);

}
