/*
  Copyright � Grame 2002

  This library is free software; you can redistribute it and modify it under 
  the terms of the GNU Library General Public License as published by the 
  Free Software Foundation version 2 of the License, or any later version.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License 
  for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
  grame@grame.fr
  
  modifications history:
 
*/

#include "msSharedMem.h"
#include "TShMem.h"

SharedMemHandler msSharedMemCreate(ShMemID id, unsigned long size, void ** memPtr)
{
    ShMem * shm = new ShMem();
    if (shm) {
        void *mem = shm->Create(id, size);
        if (mem) *memPtr = mem;
        else {
            delete shm;
            shm = 0;
        }
    }
    return shm;
}

SharedMemHandler msSharedMemOpen  (ShMemID id, void ** memPtr)
{
    ShMem * shm = new ShMem();
    if (shm) {
        void * ptr = shm->Open (id);
        if (ptr) *memPtr = ptr;
        else {
            delete shm;
            shm = 0;
        }
    }
    return shm;
}

void msSharedMemClose (SharedMemHandler shmh)
{
    if (shmh) {
        ShMem * shm = (ShMem *)shmh;
        shm->Close();
        delete shm;
    }
}

