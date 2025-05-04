/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* ExtMemStream.h:
**  Copyright (C) 2012-2018 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __MDFN_EXTMEMSTREAM_H
#define __MDFN_EXTMEMSTREAM_H

#include "Stream.h"

namespace Mednafen
{

class ExtMemStream : public Stream
{
 public:

 ExtMemStream(void*, uint64);
 ExtMemStream(const void*, uint64);

 virtual ~ExtMemStream();

 virtual uint64 attributes(void);

 virtual uint8 *map(void);
 virtual uint64 map_size(void);
 virtual void unmap(void);

 virtual uint64 read(void *data, uint64 count, bool error_on_eos = true);
 virtual void write(const void *data, uint64 count);
 virtual void seek(int64 offset, int whence);
 virtual uint64 tell(void);
 virtual uint64 size(void);
 virtual void flush(void);
 virtual void close(void);

 virtual int get_line(std::string &str);

 private:

 uint8* data_buffer;
 uint64 data_buffer_size;
 const bool ro;

 uint64 position;

 void grow_if_necessary(uint64 new_required_size, uint64 hole_end);
};

}
#endif
