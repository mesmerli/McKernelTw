/**
 * \file executer/kernel/ikc.c
 *  License details are found in the file LICENSE.
 * \brief
 *  inter kernel communication
 * \author Taku Shimosawa  <shimosawa@is.s.u-tokyo.ac.jp> \par
 *      Copyright (C) 2011 - 2012  Taku Shimosawa
 * \author Balazs Gerofi  <bgerofi@riken.jp> \par
 *      Copyright (C) 2012  RIKEN AICS
 * \author Gou Nakamura  <go.nakamura.yw@hitachi-solutions.com> \par
 *      Copyright (C) 2012 - 2013 Hitachi, Ltd.
 * \author Tomoki Shirasawa  <tomoki.shirasawa.kk@hitachi-solutions.com> \par
 *      Copyright (C) 2012 - 2013 Hitachi, Ltd.
 * \author Balazs Gerofi  <bgerofi@is.s.u-tokyo.ac.jp> \par
 *      Copyright (C) 2013  The University of Tokyo
 */
/*
 * HISTORY:
 *  2013/09/02 shirasawa add terminate thread
 *  2013/08/07 nakamura add page fault forwarding
 *  2013/06/06 shirasawa propagate error code for prepare image
 *  2013/06/02 shirasawa add error handling for prepare_process
 */
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include "mcctrl.h"
#ifdef ATTACHED_MIC
#include <sysdeps/mic/mic/micconst.h>
#endif

#define REQUEST_SHIFT    16

//#define DEBUG_IKC

#ifdef DEBUG_IKC
#define	dkprintf(...) kprintf(__VA_ARGS__)
#define	ekprintf(...) kprintf(__VA_ARGS__)
#else
#define dkprintf(...) do { if (0) printk(__VA_ARGS__); } while (0)
#define	ekprintf(...) printk(__VA_ARGS__)
#endif

//int num_channels;

//struct mcctrl_channel *channels;

void mcexec_prepare_ack(ihk_os_t os, unsigned long arg, int err);
static void mcctrl_ikc_init(ihk_os_t os, int cpu, unsigned long rphys, struct ihk_ikc_channel_desc *c);
int mcexec_syscall(struct mcctrl_usrdata *ud, struct ikc_scd_packet *packet);
void sig_done(unsigned long arg, int err);
void mcctrl_perf_ack(ihk_os_t os, struct ikc_scd_packet *packet);
void mcctrl_os_read_write_cpu_response(ihk_os_t os,
		struct ikc_scd_packet *pisp);
void mcctrl_eventfd(ihk_os_t os, struct ikc_scd_packet *pisp);

/* XXX: this runs in atomic context! */
static int syscall_packet_handler(struct ihk_ikc_channel_desc *c,
                                  void *__packet, void *__os)
{
	struct ikc_scd_packet *pisp = __packet;
	struct mcctrl_usrdata *usrdata = ihk_host_os_get_usrdata(__os);
	int msg = pisp->msg;

	switch (msg) {
	case SCD_MSG_INIT_CHANNEL:
		mcctrl_ikc_init(__os, pisp->ref, pisp->arg, c);
		break;

	case SCD_MSG_PREPARE_PROCESS_ACKED:
		mcexec_prepare_ack(__os, pisp->arg, 0);
		break;

	case SCD_MSG_PREPARE_PROCESS_NACKED:
		mcexec_prepare_ack(__os, pisp->arg, pisp->err);
		break;

	case SCD_MSG_SYSCALL_ONESIDE:
		mcexec_syscall(usrdata, pisp);
		break;

	case SCD_MSG_PROCFS_ANSWER:
		procfs_answer(usrdata, pisp->pid);
		break;

	case SCD_MSG_SEND_SIGNAL:
		sig_done(pisp->arg, pisp->err);
		break;

	case SCD_MSG_SYSFS_REQ_CREATE:
	case SCD_MSG_SYSFS_REQ_MKDIR:
	case SCD_MSG_SYSFS_REQ_SYMLINK:
	case SCD_MSG_SYSFS_REQ_LOOKUP:
	case SCD_MSG_SYSFS_REQ_UNLINK:
	case SCD_MSG_SYSFS_REQ_SETUP:
	case SCD_MSG_SYSFS_RESP_SHOW:
	case SCD_MSG_SYSFS_RESP_STORE:
	case SCD_MSG_SYSFS_RESP_RELEASE:
		sysfsm_packet_handler(__os, pisp->msg, pisp->err,
				pisp->sysfs_arg1, pisp->sysfs_arg2);
		break;

	case SCD_MSG_PROCFS_TID_CREATE:
	case SCD_MSG_PROCFS_TID_DELETE:
		procfsm_packet_handler(__os, pisp->msg, pisp->pid, pisp->arg);
		break;

	case SCD_MSG_GET_VDSO_INFO:
		get_vdso_info(__os, pisp->arg);
		break;

	case SCD_MSG_PERF_ACK:
		mcctrl_perf_ack(__os, pisp);
		break;

	case SCD_MSG_CPU_RW_REG_RESP:
		mcctrl_os_read_write_cpu_response(__os, pisp);
		break;

	case SCD_MSG_EVENTFD:
		mcctrl_eventfd(__os, pisp);
		break;

	default:
		printk(KERN_ERR "mcctrl:syscall_packet_handler:"
				"unknown message (%d.%d.%d.%d.%d.%#lx)\n",
				pisp->msg, pisp->ref, pisp->osnum, pisp->pid,
				pisp->err, pisp->arg);
		break;
	}

	/* 
	 * SCD_MSG_SYSCALL_ONESIDE holds the packet and frees is it
	 * mcexec_ret_syscall(), for the rest, free it here.
	 */
	if (msg != SCD_MSG_SYSCALL_ONESIDE) {
		ihk_ikc_release_packet((struct ihk_ikc_free_packet *)__packet,
				(usrdata->ikc2linux[smp_processor_id()] ?
				 usrdata->ikc2linux[smp_processor_id()] :
				 usrdata->ikc2linux[0]));
	}
	return 0;
}

static int dummy_packet_handler(struct ihk_ikc_channel_desc *c,
                                  void *__packet, void *__os)
{
	kprintf("%s: WARNING: packet received\n", __FUNCTION__);
	ihk_ikc_release_packet((struct ihk_ikc_free_packet *)__packet, c);
	return 0;
}

int mcctrl_ikc_send(ihk_os_t os, int cpu, struct ikc_scd_packet *pisp)
{
	struct mcctrl_usrdata *usrdata = ihk_host_os_get_usrdata(os);

	if (cpu < 0 || os == NULL || usrdata == NULL ||
	    cpu >= usrdata->num_channels || !usrdata->channels[cpu].c) {
		return -EINVAL;
	}
	return ihk_ikc_send(usrdata->channels[cpu].c, pisp, 0);
}

int mcctrl_ikc_send_msg(ihk_os_t os, int cpu, int msg, int ref, unsigned long arg)
{
	struct ikc_scd_packet packet;
	struct mcctrl_usrdata *usrdata = ihk_host_os_get_usrdata(os);

	if (cpu < 0 || cpu >= usrdata->num_channels || !usrdata->channels[cpu].c) {
		return -EINVAL;
	}

	packet.msg = msg;
	packet.ref = ref;
	packet.arg = arg;

	return ihk_ikc_send(usrdata->channels[cpu].c, &packet, 0);
}

int mcctrl_ikc_set_recv_cpu(ihk_os_t os, int cpu)
{
	struct mcctrl_usrdata *usrdata = ihk_host_os_get_usrdata(os);

	ihk_ikc_channel_set_cpu(usrdata->channels[cpu].c,
	                        ihk_ikc_get_processor_id());
	return 0;
}

int mcctrl_ikc_is_valid_thread(ihk_os_t os, int cpu)
{
	struct mcctrl_usrdata *usrdata = ihk_host_os_get_usrdata(os);

	if (cpu < 0 || cpu >= usrdata->num_channels || !usrdata->channels[cpu].c) {
		return 0;
	} else {
		return 1;
	}
}

static void mcctrl_ikc_init(ihk_os_t os, int cpu, unsigned long rphys, struct ihk_ikc_channel_desc *c)
{
	struct mcctrl_usrdata *usrdata = ihk_host_os_get_usrdata(os);
	struct ikc_scd_packet packet;
	struct mcctrl_channel *pmc = usrdata->channels + cpu;

	if (c->port == 502) {
		pmc = usrdata->channels + usrdata->num_channels - 1;
	}

	if (!pmc) {
		kprintf("%s: error: no channel found?\n", __FUNCTION__);
		return;
	}

	packet.msg = SCD_MSG_INIT_CHANNEL_ACKED;
	packet.ref = cpu;
	packet.arg = rphys;

	ihk_ikc_send(pmc->c, &packet, 0);
}

static int connect_handler_ikc2linux(struct ihk_ikc_channel_info *param)
{
	struct ihk_ikc_channel_desc *c;
	ihk_os_t os = param->channel->remote_os;
	struct mcctrl_usrdata  *usrdata = ihk_host_os_get_usrdata(os);
	int linux_cpu;

	c = param->channel;
	linux_cpu = c->send.queue->write_cpu;
	if (linux_cpu > nr_cpu_ids) {
		kprintf("%s: invalid Linux CPU id %d\n",
				__FUNCTION__, linux_cpu);
		return -1;
	}
	dkprintf("%s: Linux CPU: %d\n", __FUNCTION__, linux_cpu);

	param->packet_handler = syscall_packet_handler;
	usrdata->ikc2linux[linux_cpu] = c;

	return 0;
}

static int connect_handler_ikc2mckernel(struct ihk_ikc_channel_info *param)
{
	struct ihk_ikc_channel_desc *c;
	int mck_cpu;
	ihk_os_t os = param->channel->remote_os;
	struct mcctrl_usrdata  *usrdata = ihk_host_os_get_usrdata(os);

	c = param->channel;
	mck_cpu = c->send.queue->read_cpu;

	if (mck_cpu < 0 || mck_cpu >= usrdata->num_channels) {
		kprintf("Invalid connect source processor: %d\n", mck_cpu);
		return 1;
	}
	param->packet_handler = dummy_packet_handler;

	usrdata->channels[mck_cpu].c = c;

	return 0;
}

static struct ihk_ikc_listen_param lp_ikc2linux = {
	.port = 503,
	.ikc_direction = IHK_IKC_DIRECTION_RECV,
	.handler = connect_handler_ikc2linux,
	.pkt_size = sizeof(struct ikc_scd_packet),
	.queue_size = PAGE_SIZE * 4,
	.magic = 0x1129,
};

static struct ihk_ikc_listen_param lp_ikc2mckernel = {
	.port = 501,
	.ikc_direction = IHK_IKC_DIRECTION_SEND,
	.handler = connect_handler_ikc2mckernel,
	.pkt_size = sizeof(struct ikc_scd_packet),
	.queue_size = PAGE_SIZE * 4,
	.magic = 0x1329,
};

int prepare_ikc_channels(ihk_os_t os)
{
	struct mcctrl_usrdata *usrdata;
	int i;
	int ret = 0;

	usrdata = kzalloc(sizeof(struct mcctrl_usrdata), GFP_KERNEL);
	if (!usrdata) {
		printk("%s: error: allocating mcctrl_usrdata\n", __FUNCTION__);
		ret = -ENOMEM;
		goto error;
	}

	usrdata->cpu_info = ihk_os_get_cpu_info(os);
	usrdata->mem_info = ihk_os_get_memory_info(os);

	if (!usrdata->cpu_info || !usrdata->mem_info) {
		printk("%s: cannot obtain OS CPU and memory information.\n",
			__FUNCTION__);
		ret = -EINVAL;
		goto error;
	}

	if (usrdata->cpu_info->n_cpus < 1) {
		printk("%s: Error: # of cpu is invalid.\n", __FUNCTION__);
		ret = -EINVAL;
		goto error;
	}

	usrdata->num_channels = usrdata->cpu_info->n_cpus;
	usrdata->channels = kzalloc(sizeof(struct mcctrl_channel) *
			usrdata->num_channels,
			GFP_KERNEL);

	if (!usrdata->channels) {
		printk("Error: cannot allocate channels.\n");
		ret = -ENOMEM;
		goto error;
	}

	usrdata->ikc2linux = kzalloc(sizeof(struct ihk_ikc_channel_desc *) *
			nr_cpu_ids, GFP_KERNEL);

	if (!usrdata->ikc2linux) {
		printk("Error: cannot allocate ikc2linux channels.\n");
		ret = -ENOMEM;
		goto error;
	}

	usrdata->os = os;
	ihk_host_os_set_usrdata(os, usrdata);

	ihk_ikc_listen_port(os, &lp_ikc2linux);
	ihk_ikc_listen_port(os, &lp_ikc2mckernel);

	init_waitqueue_head(&usrdata->wq_procfs);
	mutex_init(&usrdata->reserve_lock);

	for (i = 0; i < MCCTRL_PER_PROC_DATA_HASH_SIZE; ++i) {
		INIT_LIST_HEAD(&usrdata->per_proc_data_hash[i]);
		rwlock_init(&usrdata->per_proc_data_hash_lock[i]);
	}

	INIT_LIST_HEAD(&usrdata->cpu_topology_list);
	INIT_LIST_HEAD(&usrdata->node_topology_list);

	mutex_init(&usrdata->part_exec.lock);
	INIT_LIST_HEAD(&usrdata->part_exec.pli_list);
	usrdata->part_exec.nr_processes = -1;

	return 0;

error:
	if (usrdata) {
		if (usrdata->channels) kfree(usrdata->channels);
		if (usrdata->ikc2linux) kfree(usrdata->ikc2linux);
		kfree(usrdata);
	}

	return ret;
}

void __destroy_ikc_channel(ihk_os_t os, struct mcctrl_channel *pmc)
{
	return;
}

void destroy_ikc_channels(ihk_os_t os)
{
	int i;
	struct mcctrl_usrdata *usrdata = ihk_host_os_get_usrdata(os);

	if (!usrdata) {
		printk("%s: WARNING: no mcctrl_usrdata found\n", __FUNCTION__);
		return;
	}

	ihk_host_os_set_usrdata(os, NULL);

	for (i = 0; i < usrdata->num_channels; i++) {
		if (usrdata->channels[i].c) {
			ihk_ikc_destroy_channel(usrdata->channels[i].c);
		}
	}

	for (i = 0; i < nr_cpu_ids; i++) {
		if (usrdata->ikc2linux[i]) {
			ihk_ikc_destroy_channel(usrdata->ikc2linux[i]);
		}
	}

	kfree(usrdata->channels);
	kfree(usrdata->ikc2linux);
	kfree(usrdata);
}

void
mcctrl_eventfd(ihk_os_t os, struct ikc_scd_packet *pisp)
{
	ihk_os_eventfd(os, 0);
}
