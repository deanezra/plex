/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <SDL/SDL.h>

#include "XHandle.h"
#include "../utils/log.h"

int CXHandle::m_objectTracker[10] = {0};

CXHandle::CXHandle()
{
  Init();
  m_objectTracker[m_type]++;
}

CXHandle::CXHandle(HandleType nType)
{
  Init();
  m_type=nType;
  m_objectTracker[m_type]++;
}

CXHandle::CXHandle(const CXHandle &src)
{
  // we shouldnt get here EVER. however, if we do - try to make the best. copy what we can
  // and most importantly - not share any pointer.
  CLog::Log(LOGWARNING,"%s, copy handle.", __FUNCTION__);

  Init();
  if (src.m_hSem)
    m_hSem = SDL_CreateSemaphore(SDL_SemValue(src.m_hSem));

  if (m_hThread)
  {
    CLog::Log(LOGERROR, "%s - thread handle copied instead of passed!", __FUNCTION__);
  }

  if (src.m_hMutex)
    m_hMutex = SDL_CreateMutex();

  fd = src.fd;
  m_bManualEvent = src.m_bManualEvent;
  m_tmCreation = time(NULL);
  m_FindFileResults = src.m_FindFileResults;
  m_nFindFileIterator = src.m_nFindFileIterator;
  m_FindFileDir = src.m_FindFileDir;
  m_iOffset = src.m_iOffset;
  m_bCDROM = src.m_bCDROM;
  m_objectTracker[m_type]++;
}

CXHandle::~CXHandle()
{

  m_objectTracker[m_type]--;

  if (RecursionCount > 0) {
    CLog::Log(LOGERROR,"%s, destroying handle with recursion count %d", __FUNCTION__, RecursionCount);
    assert(false);
  }

  if (m_nRefCount > 1) {
    CLog::Log(LOGERROR,"%s, destroying handle with ref count %d", __FUNCTION__, m_nRefCount);
    assert(false);
  }

  if (m_hSem) {
    SDL_DestroySemaphore(m_hSem);
  }

  if (m_hMutex) {
    SDL_DestroyMutex(m_hMutex);
  }

  if (m_internalLock) {
    SDL_DestroyMutex(m_internalLock);
  }

  if (m_hCond) {
    SDL_DestroyCond(m_hCond);
  }

  if (m_hThread) {
    //SDL_WaitThread should only be called once (here, when destructing) since it will destroy the thread object.
    SDL_WaitThread(m_hThread,NULL);
  }

  if ( fd != 0 ) {
    close(fd);
  }

}

void CXHandle::Init()
{
  fd=0;
  m_hSem=NULL;
  m_hMutex=NULL;
  m_hThread=NULL;
  m_hCond=NULL;
  m_type = HND_NULL;
  RecursionCount=0;
  OwningThread=0;
  m_bManualEvent=FALSE;
  m_bEventSet=FALSE;
  m_nFindFileIterator=0 ;
  m_nRefCount=1;
  m_tmCreation = time(NULL);
  m_internalLock = SDL_CreateMutex();
#ifdef __APPLE__
  m_machThreadPort = 0;
#endif
}

void CXHandle::ChangeType(HandleType newType) {
  m_objectTracker[m_type]--;
  m_type = newType;
  m_objectTracker[m_type]++;
}

void CXHandle::DumpObjectTracker() {
  for (int i=0; i< 10; i++) {
    CLog::Log(LOGDEBUG,"object %d --> %d instances\n", i, m_objectTracker[i]);
  }
}

bool CloseHandle(HANDLE hObject) {
  if (!hObject)
    return false;

  if (hObject == INVALID_HANDLE_VALUE || hObject == (HANDLE)-1)
    return true;

  bool bDelete = false;
  SDL_LockMutex(hObject->m_internalLock);
  if (--hObject->m_nRefCount == 0)
    bDelete = true;
  SDL_UnlockMutex(hObject->m_internalLock);

  if (bDelete)
    delete hObject;

  return true;
}

