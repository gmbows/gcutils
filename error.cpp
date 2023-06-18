#include "error.h"
#include "gcutils.h"

uint32_t nasserts = 0;

void cthrow(ErrorType type,std::string err, std::string caller) {
	switch(type) {
		case ERR_WARN:
			gcutils::print("WARN ",caller,"(): ",err);
			break;
		case ERR_FATAL:
			gcutils::print("FATAL ",caller,"(): ",err);
			exit(1);
		case ERR_ASSERTION_FAILED:
			gcutils::print("ASSERTION FAILED ",caller,"(): ",err);
			exit(1);
	}
}

void cassert(bool expr, std::string err, std::string caller) {
	nasserts++;
	if(!expr) {
		cthrow(ERR_ASSERTION_FAILED,err,caller);
	}
}

void cassert(bool expr, ErrorMessage err, std::string caller) {
	nasserts++;
	std::string err_string = lookup_error(err);
	cassert(expr,err_string,caller);
}

void cthrow(ErrorType type, ErrorMessage err, std::string caller) {
	std::string err_string = lookup_error(err);
	cthrow(type,err_string,caller);
}

std::ostream& operator<<(std::ostream &out, ErrorMessage e) {
	return out << errorMap.at(e);
}

std::map<ErrorMessage,std::string> errorMap = {
	{ERR_DEREF_NULLPTR,"Dereferencing null pointer"},
	{ERR_RET_NULLPTR,"Returning nullpointer"},
	{ERR_DEFAULT,"An unexpected error occurred"},
};

std::string lookup_error(ErrorMessage e) {
	if(errorMap.find(e) == errorMap.end()) {
		cthrow(ERR_WARN,"Returning default string");
		return "Error string not found";
	}
	return errorMap.at(e);
}
