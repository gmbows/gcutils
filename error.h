#pragma once
#include <map>
#include <string>

extern uint32_t nasserts;

enum ErrorType : short {
	ERR_WARN,
	ERR_FATAL,
	ERR_ASSERTION_FAILED,
};

enum ErrorMessage : short {
	ERR_DEREF_NULLPTR,
	ERR_RET_NULLPTR,
	ERR_DEFAULT,
};

extern std::map<ErrorMessage,std::string> errorMap;
std::string lookup_error(ErrorMessage e);

std::ostream& operator<<(std::ostream &out, ErrorMessage e);

void cthrow(ErrorType type, std::string err = "An unexpected error occurred",std::string caller = __builtin_FUNCTION());
void cthrow(ErrorType type, ErrorMessage = ERR_DEFAULT,std::string caller = __builtin_FUNCTION());

void cassert(bool expr, std::string err = "An unexpected error occurred", std::string caller = __builtin_FUNCTION());
void cassert(bool expr, ErrorMessage = ERR_DEFAULT, std::string caller = __builtin_FUNCTION());
