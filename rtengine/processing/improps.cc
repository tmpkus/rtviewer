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
#include <iostream>
#include <fstream>
#include <string>
#include <sys/inotify.h>

#include "improps.h"

using namespace std;
template<> configdata & configdata::operator=(float rhs)
{
  if (this->text)
    free(this->text); // was allocated by strdup;
  this->text = new char[32];
  snprintf(text, 32, "%f", rhs);
  return *this;
}
template<> configdata & configdata::operator=(int rhs)
{
  if (this->text)
    free(this->text); // was allocated by strdup;
  this->text = new char[32];
  snprintf(text, 32, "%d", rhs);
  return *this;
}
template<> configdata & configdata::operator=(bool rhs)
{
  if (this->text)
    free(this->text); // was allocated by strdup;
  const char * t = (rhs) ? "true" : "false";
  this->text = strdup(t);
  return *this;
}

template<typename TY> configdata & configdata::operator=(vector<TY> & rhs)
{
  if (this->text)
    free(this->text); // was allocated by strdup;
  string s;
  int i;
  for (i = 0; i < rhs.size(); i++)
    s << rhs[i] << ";";
  this->text = strdup(s.c_str());
  return *this;
}
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
int improps::update()
{

  char buf[EVENT_BUF_LEN];
  if (ino_fd)
    {
	  int first = ::read(ino_fd,buf,EVENT_BUF_LEN);
      // cout << "testing filechange" << pp3_name << " " << ino_fd << "  " << first << endl;

      
      //cout << " " << first <<endl;
      if (first<1) return 0;
      while (::read(ino_fd,buf,EVENT_BUF_LEN)>0);
      cout << "detected filechange on " << pp3_name << endl;
      /*
      // re-issue our watcher because editors like gedit will do strange stuff when saving.
	  // that causes inotofy to lose the watch on that file.
	  // To avoid that we simply remove the current watch and add a new watch on our pp3 file.
	   
	  inotify_rm_watch(ino_fd,wd);
	  wd = inotify_add_watch( ino_fd, pp3_name, IN_MODIFY );
      if (wd<0)
        {
          cout << "watch descriptor failed" << endl;
	  }
	  * apparently doing it here will make our app loop..
      */
      return 1;
    }
  return 0;
  struct stat64 fileinfo;
  if (-1 != stat64(pp3_name, &fileinfo))
    {
      if ((mtime.tv_sec != fileinfo.st_mtim.tv_sec)
          && (mtime.tv_nsec != fileinfo.st_mtim.tv_nsec))
        {
          mtime = fileinfo.st_mtim;
          cout << "file has changed" << endl;
          return 1;
        }
    }
  //cout << "no filechange" << endl;
  return 0;
}

int improps::read(char * toread)
{
  early = 1; // flag for early termination of processing
  filebuf config_file_p, *tst;

  // we load the config_ file derived from the file name
  if (toread)
    {

      string default_config, config_file;
      default_config = getenv("HOME");
      default_config += "/.config/RawTherapee4/profiles/Default.pp3";
      config_file = toread;
      config_file += ".pp3";

      cout << "checking for config_ file: " << config_file << endl;
      cout << "failover is: " << default_config << endl;
      raw_name = strdup(toread);

      tst = config_file_p.open(config_file.c_str(), ios::in);
      if (tst == NULL)
        {
          cout << config_file << " not found so trying fail over\n";
          tst = config_file_p.open(default_config.c_str(), ios::in);
          config_file = default_config;
        }
      if (tst == NULL)
        {
          cout << "file " << config_file << " and " << default_config
               << " not found" << endl;
          return 0;
        }
      pp3_name = strdup(config_file.c_str());
      cout << "using config " << pp3_name << endl;
      ino_fd=inotify_init1(IN_NONBLOCK);
      if (ino_fd == -1) { 
		  cout << " can't create inotify instance.\n";
	  } else {
      wd = inotify_add_watch( ino_fd, pp3_name, IN_MODIFY);
      if (wd<0)
        {
          cout << "watch descriptor failed" << endl;
          return 0;
        }
	}

    }
  else
    {
      cout << " trying an update" << endl;
      if (pp3_name == NULL
         ) return 0;
      tst = config_file_p.open(pp3_name, ios::in);
      if (tst == NULL) 
      {
		  cout << " failed to open " << pp3_name << " for update\n";
		  return 0;
	  }
	  
	  // re-issue our watcher because editors like gedit will do strange stuff when saving.
	  // that causes inotofy to lose the watch on that file.
	  // To avoid that we simply remove the current watch and add a new watch on our pp3 file.
	  // gedit usually uses a backup file .. this will make inotify even more difficult to use.
	  // when editing pp3 files in gedit try to save it several times.
	  // if you want reliable updates after saving in gedit then disable backup in the editor settings.
	   
	  inotify_rm_watch(ino_fd,wd);
	  wd = inotify_add_watch( ino_fd, pp3_name, IN_MODIFY );
      if (wd<0)
        {
          cout << "watch descriptor failed" << endl;
	  }
      cout << " doing an update" << endl;
    }
  char line[4096];
  //update();
  istream is(&config_file_p);
  char * chapter = "<default>";
  int len;
  pp3.clear(); // make sure we will be using the new values. without clear it seems the map does not get updated correctly?
  while (is.getline(line, 4096))
    {
      cout << line << endl;
      if (line[0] == '[')
        {
          chapter = strdup(line+1); // skip '['
          char *t=chapter;
          while(*t && *t!=']') t++;
          if (*t==']') t[0]=0; // remove trailing ']' and avoid CR LF stuff
          configitems temp;
          cout << "added chapter " << chapter <<endl;
          pp3[chapter] = temp;
          cout << "added chapter " << chapter <<endl;
        }
      else
        {
          if ((len = strlen(line)) > 1)
            {
              int i = 0;
              while ((i < len) && (line[i] != '=')) // find space
                i++;
              if ((i < len - 1) && (i > 0))
                {
                  line[i] = 0; // replace space with lineend
                  configdata item;
                  item.text = strdup(line + i + 1);
                  pp3[chapter][strdup(line)] = item;
                }
            }
        }
    }
  config_file_p.close();

  // now we walk through the config_ data
/*
  pp3_datamap::iterator chapt = pp3.begin();
  while (chapt != pp3.end())
    {

      map<char*, configdata, ltstr>::iterator conf = chapt->second.begin();
      cout << "chapter: " << chapt->first << endl;
      while (conf != chapt->second.end())
        {
          cout << "     item:" << conf->first << " = " << conf->second.text
               << endl;
          conf++;
        }
      chapt++;
    }

  dump();*/
  return 1;
}


void improps::dump(void)
{
  pp3_datamap::iterator chapter;
  configitems::iterator item;
  cout << "RAW file:" << raw_name << endl;
  cout << "PP3 file:" << pp3_name << endl;
  for (chapter = pp3.begin(); chapter != pp3.end(); chapter++)
    {
      cout << chapter->first << endl;
      for (item = chapter->second.begin(); item != chapter->second.end(); item++)
        cout << "    " << item->first << "=" << item->second.text << endl;
    }
}

