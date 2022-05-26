#include "Utils.h"


std::string trimString(std::string stringCopy)
{
	stringCopy.erase(stringCopy.begin(), std::find_if(stringCopy.begin(), stringCopy.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));
	stringCopy.erase(std::find_if(stringCopy.rbegin(), stringCopy.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), stringCopy.end());

	return stringCopy;
}