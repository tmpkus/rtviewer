/*
 * improps.h
 *
 *  Created on: Jul 22, 2011
 *      Author: janrinze
 */

#ifndef IMPROPS_H_
#define IMPROPS_H_

#include <map>
#include <vector>
#include <cstdlib>
#include <cstring>

using namespace std;

struct ltstr
{
  bool operator()(char* s1, char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};
class configdata
{
	enum datatype
	{
		BOOL,
		FLOAT,
		INT,
		FLOAT_ARRAY,
		INT_ARRAY,
		TEXT,
	};

	enum datatype type;

public:
	char * text;
	template <typename T> void get(T & dst);
	template <typename T> void set(T & src);

	template <class U> configdata & operator= (U rhs);
	template <typename T> configdata & operator= (vector<T> & rhs);
	operator float()
	{
		char *t;
		type=FLOAT;
		return (float)strtod(text,&t);
	};
	operator int()
	{
		char *t;
		type=INT;
		return strtol(text,&t,0);
	};
	operator bool()
	{
		char *t="true";
		type=BOOL;
		bool test = ((strncmp(t,text,4))==0);
		return test;
	};
	operator char *()
	{
		type=TEXT;
		return text;
	};
	operator class vector<float> ()
		{
			vector<float> t;
			char * r=text;
			while(*r)
			{
				char *res;
				t.push_back(strtod(r,&res));
				if (*res==';') res++;
				r=res;
			}
			return t;
		}
	operator class vector<int> ()
		{
			vector<int> t;
			char * r=text;
			while(*r)
			{
				char *res;
				t.push_back(strtol(r,&res,0));
				if (*res==';') res++;
				r=res;
			}
			return t;
		}
};




typedef map<char*,configdata,ltstr> configitems;
typedef map<char*,configitems,ltstr> pp3_datamap;

class improps
{
private:
	char * raw_name;
	char * pp3_name;
public:
	pp3_datamap pp3;
	float expcomp;
	float contrast;
	float sh_radius;
	float sh_amount;
	float noise_lamount;
	float noise_camount;
	float noise_gamma;
	int read(int argc,char** argv);
	void dump(void);
};

#endif /* IMPROPS_H_ */
