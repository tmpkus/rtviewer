/*
 *  This file is part of RTViewer.
 *
 *	copyright (c) 2011  Jan Rinze Peterzon (janrinze@gmail.com)
 *
 *  RTViewer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTViewer.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IMPROPS_H_
#define IMPROPS_H_

#include <map>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
		if (text)
			return (float)strtod(text,&t);
		else return 0.0f;
	};
	operator int()
	{
		char *t;
		type=INT;
		if (text) return strtol(text,&t,0);
		return 0;
	};
	operator bool()
	{
		char *t="true";
		type=BOOL;
		if (text)
			return ((strncmp(t,text,4))==0);
		return false;
	};
	operator char *()
	{
		type=TEXT;
		return text;
	};
	operator class vector<float> ()
		{
			vector<float> t;
			if (text)
			{
			char * r=text;
			while(*r)
			{
				char *res;
				t.push_back(strtod(r,&res));
				if (*res==';') res++;
				r=res;
			}
			}
			return t;
		}
	operator class vector<int> ()
		{
			vector<int> t;
			if (text)
			{
			char * r=text;
			while(*r)
			{
				char *res;
				t.push_back(strtol(r,&res,0));
				if (*res==';') res++;
				r=res;
			}
			}
			return t;
		}
};

typedef struct
{
	char * colspace_name;
	//colmatrix col_matrix;
} colorspace;


typedef map<char*,configdata,ltstr> configitems;
typedef map<char*,configitems,ltstr> pp3_datamap;




class improps
{
private:
	char * raw_name;
	char * pp3_name;
	struct timespec mtime;
public:
	pp3_datamap pp3;
	volatile int early;
	colorspace cols;
	float mat[3][3]; // color matrix used


	int read(char * name);
	void dump(void);
	int update();
};

#endif /* IMPROPS_H_ */
