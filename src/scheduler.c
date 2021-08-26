/*
 * Copyright (c) 2010-2019 Belledonne Communications SARL.
 *
 * This file is part of oRTP.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ortp/ortp.h>
#include "utils.h"
#include "scheduler.h"
#include "rtpsession_priv.h"

// To avoid warning during compile
extern void rtp_session_process (RtpSession * session, uint32_t time, RtpScheduler *sched);


void rtp_scheduler_init(RtpScheduler *sched)
{
	sched->list=0;
	sched->time_=0;
	/* default to the posix timer */
#if !defined(ORTP_WINDOWS_UNIVERSAL)
	// 这个posix_timer定时器是唯一可用的定时器
	rtp_scheduler_set_timer(sched,&posix_timer);
#endif
	ortp_mutex_init(&sched->lock,NULL);
	ortp_cond_init(&sched->unblock_select_cond,NULL);
	sched->max_sessions=sizeof(SessionSet)*8;  //  1024   : sizeof(SessionSet) = 128 bytes
	session_set_init(&sched->all_sessions);
	sched->all_max=0;
	session_set_init(&sched->r_sessions);
	sched->r_max=0;
	session_set_init(&sched->w_sessions);
	sched->w_max=0;
	session_set_init(&sched->e_sessions);
	sched->e_max=0;
}

RtpScheduler * rtp_scheduler_new()
{
	RtpScheduler *sched=(RtpScheduler *) ortp_malloc(sizeof(RtpScheduler));
	memset(sched,0,sizeof(RtpScheduler));
	// 调用 rtp_scheduler_init 初始化
	rtp_scheduler_init(sched);
	return sched;
}

void rtp_scheduler_set_timer(RtpScheduler *sched,RtpTimer *timer)
{
	if (sched->thread_running){
		ortp_warning("Cannot change timer while the scheduler is running !!");
		return;
	}
	sched->timer=timer;
	/* report the timer increment */  // 10000/1000 + 0*1000 = 10usec
	sched->timer_inc=(timer->interval.tv_usec/1000) + (timer->interval.tv_sec*1000);
}

void rtp_scheduler_start(RtpScheduler *sched)
{
	if (sched->thread_running==0){
		sched->thread_running=1;
		ortp_mutex_lock(&sched->lock);

		// 执行线程 调度线程
		ortp_thread_create(&sched->thread, NULL, rtp_scheduler_schedule,(void*)sched);
		ortp_cond_wait(&sched->unblock_select_cond,&sched->lock);
		ortp_mutex_unlock(&sched->lock);
	}
	else ortp_warning("Scheduler thread already running.");

}
void rtp_scheduler_stop(RtpScheduler *sched)
{
	if (sched->thread_running==1)
	{
		sched->thread_running=0;
		ortp_thread_join(sched->thread, NULL);
	}
	else ortp_warning("Scheduler thread is not running.");
}

void rtp_scheduler_destroy(RtpScheduler *sched)
{
	if (sched->thread_running) rtp_scheduler_stop(sched);
	ortp_mutex_destroy(&sched->lock);
	//g_mutex_free(sched->unblock_select_mutex);
	ortp_cond_destroy(&sched->unblock_select_cond);
	ortp_free(sched);
}

void * rtp_scheduler_schedule(void * psched)
{
	/*
	这个函数是调度器的工作函数，
	其主要任务就是检查是否有rtpsession到了需要唤醒的时间并更新相应session集合的状态。

	1. 首先启动定时器，然后遍历调度器上存储rtpsession的链表，
		对每个rtpsession调用rtp_session_process这个处理函数，
	2. 然后用条件变量唤醒所有可能在等待的select。
	3. 最后调用定时器的等待函数睡眠并更新运行时间。
	*/
	RtpScheduler *sched=(RtpScheduler*) psched;
	RtpTimer *timer=sched->timer;
	RtpSession *current;

	/* take this lock to prevent the thread to start until g_thread_create() returns
		because we need sched->thread to be initialized */
	ortp_mutex_lock(&sched->lock);
	ortp_cond_signal(&sched->unblock_select_cond);	/* unblock the starting thread */
	ortp_mutex_unlock(&sched->lock);
	timer->timer_init();
	while(sched->thread_running)
	{
		/* do the processing here: */
		ortp_mutex_lock(&sched->lock);
		
		current=sched->list;


		/* processing all scheduled rtp sessions */

		// 遍历scheduler上注册的所有会话、 如果不为空, 说明应用有会话需要调度管理
		while (current!=NULL)
		{
			ortp_debug("scheduler: processing session=0x%p.\n",current);
			// 调用 rtp_session_process 函数进行处理当前的会话  
			rtp_session_process(current,sched->time_,sched); // ❆❆❆❆❆❆❆❆❆❆❆❆❆❆❆❆❆❆❆❆❆
			current=current->next;
		}
		/* wake up all the threads that are sleeping in _select()  */  // 唤醒所有在select上的线程
		ortp_cond_broadcast(&sched->unblock_select_cond);
		ortp_mutex_unlock(&sched->lock);
		
		/* now while the scheduler is going to sleep, the other threads can compute their
		result mask and see if they have to leave, or to wait for next tick*/
		//ortp_message("scheduler: sleeping.");
		timer->timer_do();
		sched->time_+=sched->timer_inc;  // 每一次递增10usec
	}
	/* when leaving the thread, stop the timer */
	timer->timer_uninit();
	return NULL;
}

void rtp_scheduler_add_session(RtpScheduler *sched, RtpSession *session)
{
	RtpSession *oldfirst;
	int i;
	if (session->flags & RTP_SESSION_IN_SCHEDULER){  // 该已经存在于调度的进程中
		/* the rtp session is already scheduled, so return silently */
		return;
	}
	rtp_scheduler_lock(sched);
	/* enqueue the session to the list of scheduled sessions */
	oldfirst=sched->list;
	sched->list=session;
	session->next=oldfirst;
	if (sched->max_sessions==0){
		ortp_error("rtp_scheduler_add_session: max_session=0 !");
	}
	/* find a free pos in the session mask*/
	for (i=0;i<sched->max_sessions;i++){
		if (!ORTP_FD_ISSET(i,&sched->all_sessions.rtpset)){
			session->mask_pos=i;
			session_set_set(&sched->all_sessions,session);
			/* make a new session scheduled not blockable if it has not started*/
			if (session->flags & RTP_SESSION_RECV_NOT_STARTED) 
				session_set_set(&sched->r_sessions,session);
			if (session->flags & RTP_SESSION_SEND_NOT_STARTED) 
				session_set_set(&sched->w_sessions,session);
			if (i>sched->all_max){
				sched->all_max=i;
			}
			break;
		}
	}
	
	rtp_session_set_flag(session,RTP_SESSION_IN_SCHEDULER);
	rtp_scheduler_unlock(sched);
}

void rtp_scheduler_remove_session(RtpScheduler *sched, RtpSession *session)
{
	RtpSession *tmp;
	int cond=1;
	return_if_fail(session!=NULL); 
	if (!(session->flags & RTP_SESSION_IN_SCHEDULER)){
		/* the rtp session is not scheduled, so return silently */
		return;
	}

	rtp_scheduler_lock(sched);
	tmp=sched->list;
	if (tmp==session){
		sched->list=tmp->next;
		rtp_session_unset_flag(session,RTP_SESSION_IN_SCHEDULER);
		session_set_clr(&sched->all_sessions,session);
		rtp_scheduler_unlock(sched);
		return;
	}
	/* go the position of session in the list */
	while(cond){
		if (tmp!=NULL){
			if (tmp->next==session){
				tmp->next=tmp->next->next;
				cond=0;
			}
			else tmp=tmp->next;
		}else {
			/* the session was not found ! */
			ortp_warning("rtp_scheduler_remove_session: the session was not found in the scheduler list!");
			cond=0;
		}
	}
	rtp_session_unset_flag(session,RTP_SESSION_IN_SCHEDULER);
	/* delete the bit in the mask */
	session_set_clr(&sched->all_sessions,session);
	rtp_scheduler_unlock(sched);
}
