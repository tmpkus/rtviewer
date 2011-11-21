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

#include "improps.h"

using namespace std;
template<> configdata & configdata::operator=(float rhs)
{
	free(this->text); // was allocated by strdup;
	this->text = new char[32];
	snprintf(text, 32, "%f", rhs);
	return *this;
}
template<> configdata & configdata::operator=(int rhs)
{
	free(this->text); // was allocated by strdup;
	this->text = new char[32];
	snprintf(text, 32, "%d", rhs);
	return *this;
}
template<> configdata & configdata::operator=(bool rhs)
{
	free(this->text); // was allocated by strdup;
	const char * t = (rhs) ? "true" : "false";
	this->text = strdup(t);
	return *this;
}

template<typename TY> configdata & configdata::operator=(vector<TY> & rhs)
{
	free(this->text); // was allocated by strdup;
	string s;
	int i;
	for ( i = 0; i < rhs.size() ; i++ )
		s << rhs[i] << ";";
	this->text = strdup(s.c_str());
	return *this;
}

int improps::read(char * toread)
{
	early=0; // flag for early termination of processing

	// we load the config_ file derived from the file name
/*	expcomp = 0.0f;
	contrast = 0.0f;
	noise_lamount = 0.0f; //config__data["[Directional Pyramid Denoising]"]["Luma"].fval*0.01f;
	noise_camount = 0.0f; //config__data["[Directional Pyramid Denoising]"]["Chroma"].fval*0.01f;
	noise_gamma = 0.0f; //config__data["[Directional Pyramid Denoising]"]["Gamma"].fval*0.01f;

	if (argc < 2) {
		cout << " no file name given\n";
		return 0;
	}
	else {*/
		string default_config,config_file;
		filebuf	config_file_p;
		default_config=getenv ("HOME");
		default_config+="/.config/RawTherapeeAlpha/profiles/default.pp3";
		config_file=toread;
		config_file+=".pp3";

		cout << "checking for config_ file: " << config_file << endl;
		cout << "failover is: " << default_config <<endl;
		raw_name = strdup(toread);
		pp3_name = strdup(config_file.c_str());
		void * tst=(void *)config_file_p.open(config_file.c_str(), ios::in);
		if ( tst == NULL)
			{
				cout << config_file << " not found so trying fail over\n";
				tst=(void*)config_file_p.open(default_config.c_str(), ios::in);
			}
		if (tst==NULL)
		{
			cout << "file " << config_file << " and " << default_config
					<< " not found\n";
			return 0;
		} else
		{
			char line[256];
			istream is(&config_file_p);
			char * chapter = "<default>";
			int len;
			while (is.getline(line, 256)) {
				if (line[0] == '[') {
					chapter = strdup(line);
					configitems temp;
					pp3[chapter] = temp;
				}
				else {
					if ((len = strlen(line)) > 1) {
						int i = 0;
						while ((i < len) && (line[i] != '=')) // find space
							i++;
						if ((i < len - 1) && (i > 0)) {
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

			pp3_datamap::iterator chapt = pp3.begin();
			while (chapt != pp3.end()) {
				//configitems
				map<char*, configdata, ltstr>::iterator conf =
						chapt->second.begin();
				cout << "chapter: " << chapt->first << endl;
				while (conf != chapt->second.end()) {
					cout << "     item:" << conf->first << " = "
							<< conf->second.text << endl;
					conf++;
				}
				chapt++;
			}

/*			expcomp = pp3["[Exposure]"]["Compensation"];
			contrast = pp3["[Exposure]"]["Contrast"];
			noise_lamount = pp3["[Directional Pyramid Denoising]"]["Luma"];
			noise_camount = pp3["[Directional Pyramid Denoising]"]["Chroma"];
			noise_gamma = pp3["[Directional Pyramid Denoising]"]["Gamma"];
			sh_radius = pp3["[Sharpening]"]["Radius"];
			sh_amount = pp3["[Sharpening]"]["Amount"];

			pp3["[test]"]["Compensation"] = 1.0f;
			pp3["[test]"]["Contrast"] = contrast;*/

			dump();
			return 1;
		}
/*	}*/

	cout << "config file does not exist\n";
	return 0;
}
void improps::dump(void)
{
	pp3_datamap::iterator chapter;
	configitems::iterator item;
	cout << "RAW file:" << raw_name << endl;
	cout << "PP3 file:" << pp3_name << endl;
	for ( chapter = pp3.begin(); chapter != pp3.end() ; chapter++ ) {
		cout << chapter->first << endl;
		for ( item = chapter->second.begin(); item != chapter->second.end() ;
				item++ )
			cout << "    " << item->first << "=" << item->second.text << endl;
	}
}
