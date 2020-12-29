

#ifndef __OAL_LINUX_WORKQUEUE_H__
#define __OAL_LINUX_WORKQUEUE_H__

/* 其他头文件包含 */
#include <asm/atomic.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <asm/param.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/workqueue.h>

/* STRUCT定义 */
typedef struct workqueue_struct oal_workqueue_stru;
typedef struct work_struct oal_work_stru;
typedef struct delayed_work oal_delayed_work;

/* 宏定义 */
#define OAL_INIT_WORK(_p_work, _p_func)          INIT_WORK(_p_work, _p_func)
#define OAL_INIT_DELAYED_WORK(_work, _func)      INIT_DELAYED_WORK(_work, _func)
#define OAL_CREATE_SINGLETHREAD_WORKQUEUE(_name) create_singlethread_workqueue(_name)
#define oal_create_workqueue(name)               create_workqueue(name)

/* 函数声明 */
/*
 * 函 数 名  : oal_create_singlethread_workqueue
 * 功能描述  : 创建一个单线程的工作队列
 */
OAL_STATIC OAL_INLINE oal_workqueue_stru *oal_create_singlethread_workqueue(const oal_int8 *pc_workqueue_name)
{
    return create_singlethread_workqueue(pc_workqueue_name);
}

/*
 * 函 数 名  : oal_destroy_workqueue
 * 功能描述  : 销毁工作队列
 */
OAL_STATIC OAL_INLINE oal_void oal_destroy_workqueue(oal_workqueue_stru *pst_workqueue)
{
    destroy_workqueue(pst_workqueue);
}

/*
 * 函 数 名  : oal_queue_work
 * 功能描述  : 添加一个任务到工作队列
 */
OAL_STATIC OAL_INLINE oal_int32 oal_queue_work(oal_workqueue_stru *pst_workqueue, oal_work_stru *pst_work)
{
    return queue_work(pst_workqueue, pst_work);
}

/*
 * queue_delayed_work - queue work on a workqueue after delay
 * @wq: workqueue to use
 * @dwork: delayable work to queue
 * @delay: number of jiffies to wait before queueing
 *
 * Equivalent to queue_delayed_work_on() but tries to use the local CPU.
 */
OAL_STATIC OAL_INLINE oal_int32 oal_queue_delayed_work(oal_workqueue_stru *pst_workqueue,
                                                       oal_delayed_work *pst_work,
                                                       oal_ulong delay)
{
    return queue_delayed_work(pst_workqueue, pst_work, delay);
}

/*
 * queue_delayed_work_on - queue work on specific CPU after delay
 * @cpu: CPU number to execute work on
 * @wq: workqueue to use
 * @dwork: work to queue
 * @delay: number of jiffies to wait before queueing
 *
 * Returns %false if @work was already on a queue, %true otherwise.  If
 * @delay is zero and @dwork is idle, it will be scheduled for immediate
 */
OAL_STATIC OAL_INLINE oal_int32 oal_queue_delayed_work_on(oal_int32 cpu, oal_workqueue_stru *pst_workqueue,
                                                          oal_delayed_work *pst_work, oal_ulong delay)
{
    return queue_delayed_work_on(cpu, pst_workqueue, pst_work, delay);
}

/*
 * 函 数 名  : oal_queue_delayed_system_work
 * 功能描述  : queue work on system wq after delay
 * 输入参数  :  @dwork: delayable work to queue
 *              @delay: number of jiffies to wait before queueing
 */
OAL_STATIC OAL_INLINE oal_int32 oal_queue_delayed_system_work(oal_delayed_work *pst_work, oal_ulong delay)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))
    return queue_delayed_work(system_wq, pst_work, delay);
#else
    OAL_WARN_ON(1);
    return 1;
#endif
}

OAL_STATIC OAL_INLINE oal_int32 oal_work_is_busy(oal_work_stru *pst_work)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))
    return work_busy(pst_work);
#else
    return work_pending(pst_work);
#endif
}

#define oal_cancel_delayed_work_sync(dwork) cancel_delayed_work_sync(dwork)

#define oal_cancel_work_sync(work) cancel_work_sync(work)

#endif /* end of oal_workqueue.h */
