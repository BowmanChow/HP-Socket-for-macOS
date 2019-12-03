﻿/*
* Copyright: JessMA Open Source (ldcsaa@gmail.com)
*
* Author	: Bruce Liang
* Website	: https://github.com/ldcsaa
* Project	: https://github.com/ldcsaa/HP-Socket
* Blog		: http://www.cnblogs.com/ldcsaa
* Wiki		: http://www.oschina.net/p/hp-socket
* QQ Group	: 44636872, 75375912
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once

#include "GlobalDef.h"
#include "Singleton.h"
#include "FuncHelper.h"

#include <mutex>
#include <atomic>

using namespace std;

/*
  基于原子变量的自旋锁
  通过while的方式尝试改变一个预期值
*/
class CSpinGuard
{
public:
	CSpinGuard() : m_atFlag(FALSE)
	{

	}

	~CSpinGuard()
	{
		ASSERT(!m_atFlag);
	}

    /*
      每次以预期为False尝试改变原子变量
    */
	void Lock(BOOL bWeek = TRUE, memory_order m = memory_order_acquire)
	{
		for(UINT i = 0; !TryLock(bWeek, m); ++i)
			YieldThread(i);
	}

	BOOL TryLock(BOOL bWeek = FALSE, memory_order m = memory_order_acquire)
	{
		bool bExpect = false;

		return bWeek
			? m_atFlag.compare_exchange_weak(bExpect, (bool)TRUE, m)
			: m_atFlag.compare_exchange_strong(bExpect, (bool)TRUE, m);
	}

	void Unlock(memory_order m = memory_order_release)
	{
		ASSERT(m_atFlag);
		m_atFlag.store(FALSE, m);
	}

	DECLARE_NO_COPY_CLASS(CSpinGuard)

private:
	atomic_bool	m_atFlag;
};

/*
  递归自旋锁
  基于原子变量的方式
  记录上锁的线程ID
*/
class CReentrantSpinGuard
{
public:
	CReentrantSpinGuard()
	: m_atThreadID	(0)
	, m_iCount		(0)
	{

	}

	~CReentrantSpinGuard()
	{
		ASSERT(m_atThreadID	== 0);
		ASSERT(m_iCount		== 0);
	}

    /*
      每次以预期为False尝试改变原子变量
    */
	void Lock(BOOL bWeek = TRUE, memory_order m = memory_order_acquire)
	{
		for(UINT i = 0; !_TryLock(i == 0, bWeek, m); ++i)
			YieldThread(i);
	}

	BOOL TryLock(BOOL bWeek = FALSE, memory_order m = memory_order_acquire)
	{
		return _TryLock(TRUE, bWeek, m);
	}

	void Unlock(memory_order m = memory_order_release)
	{
        ASSERT(::IsSelfThread(m_atThreadID));

		if((--m_iCount) == 0)
			m_atThreadID.store(0, m);
	}

private:
	BOOL _TryLock(BOOL bFirst, BOOL bWeek = FALSE, memory_order m = memory_order_acquire)
    {
        THR_ID dwCurrentThreadID = SELF_THREAD_ID;

        //判断线程ID与记录的是否一致
		if(bFirst && ::IsSameThread(m_atThreadID, dwCurrentThreadID))
		{
			++m_iCount;
			return TRUE;
		}

		THR_ID ulExpect = 0;

		BOOL isOK = bWeek
			? m_atThreadID.compare_exchange_weak(ulExpect, dwCurrentThreadID, m)
			: m_atThreadID.compare_exchange_strong(ulExpect, dwCurrentThreadID, m);


		if(isOK)
		{
			ASSERT(m_iCount == 0);

			m_iCount = 1;

			return TRUE;
		}

		return FALSE;
	}

	DECLARE_NO_COPY_CLASS(CReentrantSpinGuard)

private:
	atomic_tid	m_atThreadID;
	int			m_iCount;
};

class CFakeGuard
{
public:
	void Lock()		{}
	void Unlock()	{}
	BOOL TryLock()	{return TRUE;}
};

/*
  哨兵
*/
template<class CLockObj> class CLocalLock
{
public:
	CLocalLock(CLockObj& obj) : m_lock(obj) {m_lock.Lock();}
	~CLocalLock() {m_lock.Unlock();}
private:
	CLockObj& m_lock;
};

template<class CLockObj> class CLocalTryLock
{
public:
	CLocalTryLock(CLockObj& obj) : m_lock(obj) {m_bValid = m_lock.TryLock();}
	~CLocalTryLock() {if(m_bValid) m_lock.Unlock();}

	BOOL IsValid() {return m_bValid;}

private:
	CLockObj&	m_lock;
	BOOL		m_bValid;
};

template<class CMTXObj> class CMTXTryLock
{
public:
	CMTXTryLock(CMTXObj& obj) : m_lock(obj) {m_bValid = m_lock.try_lock();}
	~CMTXTryLock() {if(m_bValid) m_lock.unlock();}

	BOOL IsValid() {return m_bValid;}

private:
	CMTXObj&	m_lock;
	BOOL		m_bValid;
};

using CSpinLock					= CLocalLock<CSpinGuard>;
using CReentrantSpinLock		= CLocalLock<CReentrantSpinGuard>;
using CFakeLock					= CLocalLock<CFakeGuard>;

using CCriSec					= mutex;
using CCriSecLock				= lock_guard<mutex>;
using CCriSecLock2				= unique_lock<mutex>;
using CCriSecTryLock			= CMTXTryLock<mutex>;

using CMTX						= CCriSec;
using CMutexLock				= CCriSecLock;
using CMutexLock2				= CCriSecLock2;
using CMutexTryLock				= CCriSecTryLock;

using CReentrantCriSec			= recursive_mutex;
using CReentrantCriSecLock		= lock_guard<recursive_mutex>;
using CReentrantCriSecLock2		= unique_lock<recursive_mutex>;
using CReentrantCriSecTryLock	= CMTXTryLock<recursive_mutex>;

using CReentrantMTX				= CReentrantCriSec;
using CReentrantMutexLock		= CReentrantCriSecLock;
using CReentrantMutexLock2		= CReentrantCriSecLock2;
using CReentrantMutexTryLock	= CReentrantCriSecTryLock;
