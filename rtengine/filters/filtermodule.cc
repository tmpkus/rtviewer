/*
 * filtermodule.cc
 *
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

#include "filtermodule.h"
#include <iostream>
using namespace std;

module * modules = NULL;

void list_filters(void) {
	module * list = modules;
	while (list) {
		cout << "filter: " << list->name << " rank: " << list->rank << endl;
		list = list->next;
	}
}

void addmodule(module & moduleinfo) {
	module * found = modules;
	module * next = modules;
	while (next && (next->rank < moduleinfo.rank)) {
		found = next;
		next = found->next;
	}
	if ((modules == 0) || (next == modules)) {
		moduleinfo.next = modules;
		modules = &moduleinfo;
	} else {
		moduleinfo.next = found->next;
		found->next = &moduleinfo;
	}
}
