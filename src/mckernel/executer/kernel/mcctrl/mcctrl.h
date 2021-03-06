/**
 * \file mcctrl.h
 *  License details are found in the file LICENSE.
 * \brief
 *  define data structure
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
 *  2013/11/07 hamada added <sys/resource.h> which is required by getrlimit(2)
 *  2013/10/21 nakamura exclude interpreter's segment from data region
 *  2013/10/11 nakamura mcexec: add a upper limit of the stack size
 *  2013/10/11 nakamura mcexec: add a path prefix for interpreter search
 *  2013/10/11 nakamura mcexec: add a interpreter invocation
 *  2013/10/08 nakamura add a AT_ENTRY entry to the auxiliary vector
 *  2013/09/02 shirasawa add terminate thread
 *  2013/08/19 shirasawa mcexec forward signal to MIC process
 *  2013/08/07 nakamura add page fault forwarding
 *  2013/07/26 shirasawa mcexec print signum or exit status
 *  2013/07/17 nakamura create more mcexec thread so that all cpu to be serviced
 *  2013/04/17 nakamura add generic system call forwarding
 */
#ifndef HEADER_MCCTRL_H
#define HEADER_MCCTRL_H

#include <linux/fs.h>
#include <ihk/ihk_host_driver.h>
#include <linux/resource.h>
#include <uprotocol.h>
#include <linux/wait.h>
#include <ihk/ikc.h>
#include <ikc/master.h>
#include <ihk/msr.h>
#include <linux/semaphore.h>
#include <linux/rwlock.h>
#include <linux/threads.h>
#include "sysfs.h"

#define SCD_MSG_PREPARE_PROCESS         0x1
#define SCD_MSG_PREPARE_PROCESS_ACKED   0x2
#define SCD_MSG_PREPARE_PROCESS_NACKED  0x7
#define SCD_MSG_SCHEDULE_PROCESS        0x3
#define SCD_MSG_WAKE_UP_SYSCALL_THREAD  0x14

#define SCD_MSG_INIT_CHANNEL            0x5
#define SCD_MSG_INIT_CHANNEL_ACKED      0x6

#define SCD_MSG_SYSCALL_ONESIDE         0x4
#define SCD_MSG_SEND_SIGNAL     	0x8
#define SCD_MSG_CLEANUP_PROCESS         0x9
#define SCD_MSG_GET_VDSO_INFO           0xa

//#define SCD_MSG_GET_CPU_MAPPING         0xc
//#define SCD_MSG_REPLY_GET_CPU_MAPPING   0xd

#define	SCD_MSG_PROCFS_CREATE		0x10
#define	SCD_MSG_PROCFS_DELETE		0x11
#define	SCD_MSG_PROCFS_REQUEST		0x12
#define	SCD_MSG_PROCFS_ANSWER		0x13

#define SCD_MSG_DEBUG_LOG		0x20

#define SCD_MSG_SYSFS_REQ_CREATE        0x30
/* #define SCD_MSG_SYSFS_RESP_CREATE    0x31 */
#define SCD_MSG_SYSFS_REQ_MKDIR         0x32
/* #define SCD_MSG_SYSFS_RESP_MKDIR     0x33 */
#define SCD_MSG_SYSFS_REQ_SYMLINK       0x34
/* #define SCD_MSG_SYSFS_RESP_SYMLINK   0x35 */
#define SCD_MSG_SYSFS_REQ_LOOKUP        0x36
/* #define SCD_MSG_SYSFS_RESP_LOOKUP    0x37 */
#define SCD_MSG_SYSFS_REQ_UNLINK        0x38
/* #define SCD_MSG_SYSFS_RESP_UNLINK    0x39 */
#define SCD_MSG_SYSFS_REQ_SHOW          0x3a
#define SCD_MSG_SYSFS_RESP_SHOW         0x3b
#define SCD_MSG_SYSFS_REQ_STORE         0x3c
#define SCD_MSG_SYSFS_RESP_STORE        0x3d
#define SCD_MSG_SYSFS_REQ_RELEASE       0x3e
#define SCD_MSG_SYSFS_RESP_RELEASE      0x3f
#define SCD_MSG_SYSFS_REQ_SETUP         0x40
#define SCD_MSG_SYSFS_RESP_SETUP        0x41
/* #define SCD_MSG_SYSFS_REQ_CLEANUP    0x42 */
/* #define SCD_MSG_SYSFS_RESP_CLEANUP   0x43 */
#define SCD_MSG_PROCFS_TID_CREATE	0x44
#define SCD_MSG_PROCFS_TID_DELETE	0x45

#define SCD_MSG_EVENTFD			0x46

#define SCD_MSG_PERF_CTRL               0x50
#define SCD_MSG_PERF_ACK                0x51

#define SCD_MSG_CPU_RW_REG              0x52
#define SCD_MSG_CPU_RW_REG_RESP         0x53

#define DMA_PIN_SHIFT                   21

#define DO_USER_MODE

#define	__NR_coredump			999

struct coretable {
	int len;
	unsigned long addr;
};

enum mcctrl_os_cpu_operation {
	MCCTRL_OS_CPU_READ_REGISTER,
	MCCTRL_OS_CPU_WRITE_REGISTER,
	MCCTRL_OS_CPU_MAX_OP
};

struct ikc_scd_packet {
	int msg;
	int err;
	union {
		/* for traditional SCD_MSG_* */
		struct {
			int ref;
			int osnum;
			int pid;
			unsigned long arg;
			struct syscall_request req;
			unsigned long resp_pa;
		};

		/* for SCD_MSG_SYSFS_* */
		struct {
			long sysfs_arg1;
			long sysfs_arg2;
			long sysfs_arg3;
		};

		/* SCD_MSG_SCHEDULE_THREAD */
		struct {
			int ttid;
		};

		/* SCD_MSG_CPU_RW_REG */
		struct {
			struct ihk_os_cpu_register desc;
			enum mcctrl_os_cpu_operation op;
			void *resp;
		};
	};
	char padding[12];
};

struct mcctrl_priv { 
	ihk_os_t os;
	struct program_load_desc *desc;
};

struct ikc_scd_init_param {
	unsigned long request_page;
	unsigned long response_page;
	unsigned long doorbell_page;
	unsigned long post_page;
};

struct syscall_post {
	unsigned long v[8];
};

struct syscall_params {
	unsigned long request_pa;
	struct syscall_request *request_va;
	unsigned long response_rpa, response_pa;
	struct syscall_response *response_va;
	unsigned long post_pa;
	struct syscall_post *post_va;
	
	unsigned long doorbell_pa;
	unsigned long *doorbell_va;
};

struct wait_queue_head_list_node {
	struct list_head list;
	wait_queue_head_t wq_syscall;
	struct task_struct *task;
	/* Denotes an exclusive wait for requester TID rtid */
	int rtid;
	int req;
	struct ikc_scd_packet *packet;
};

struct mcctrl_channel {
	struct ihk_ikc_channel_desc *c;
	struct ikc_scd_init_param init;
	void *dma_buf;
};

struct mcctrl_per_thread_data {
	struct list_head hash;
	struct task_struct *task;
	void *data;
};

#define MCCTRL_PER_THREAD_DATA_HASH_SHIFT 8
#define MCCTRL_PER_THREAD_DATA_HASH_SIZE (1 << MCCTRL_PER_THREAD_DATA_HASH_SHIFT)
#define MCCTRL_PER_THREAD_DATA_HASH_MASK (MCCTRL_PER_THREAD_DATA_HASH_SIZE - 1) 

struct mcctrl_per_proc_data {
	struct mcctrl_usrdata *ud;
	struct list_head hash;
	int pid;
	unsigned long rpgtable;	/* per process, not per OS */

	struct list_head wq_list; /* All these requests come from mcexec */
	struct list_head wq_req_list; /* These requests come from IKC IRQ handler (can be processed by any threads) */
	struct list_head wq_list_exact;  /* These requests come from IKC IRQ handler targeting a particular thread */

	ihk_spinlock_t wq_list_lock;
	wait_queue_head_t wq_prepare;
	wait_queue_head_t wq_procfs;

	struct list_head per_thread_data_hash[MCCTRL_PER_THREAD_DATA_HASH_SIZE];
	rwlock_t per_thread_data_hash_lock[MCCTRL_PER_THREAD_DATA_HASH_SIZE];
	cpumask_t cpu_set;
	int ikc_target_cpu;
	atomic_t refcount;

	struct list_head devobj_pager_list;
	struct semaphore devobj_pager_lock;
};

struct sysfsm_req {
	int busy;
	int padding;
	long lresult;
	wait_queue_head_t wq;
};

struct sysfsm_data {
	size_t sysfs_bufsize;
	void *sysfs_buf;
	long sysfs_buf_rpa;
	long sysfs_buf_pa;
	struct kobject *sysfs_kobj;
	struct sysfsm_node *sysfs_root;
	struct semaphore sysfs_tree_sem;
	struct semaphore sysfs_io_sem;
	struct sysfsm_req sysfs_req;
	ihk_os_t sysfs_os;
};

static inline int sysfs_inited(struct sysfsm_data *sdp)
{
	return !!(sdp->sysfs_buf);
} /* sysfs_inited() */

struct cache_topology {
	struct ihk_cache_topology *saved;
	cpumask_t shared_cpu_map;

	struct list_head chain;
};

struct cpu_topology {
	//struct mcctrl_usrdata *udp;
	struct ihk_cpu_topology *saved;
	int mckernel_cpu_id;
	cpumask_t core_siblings;
	cpumask_t thread_siblings;

	struct list_head chain;
	struct list_head cache_list;
};

#define NODE_DISTANCE_S_SIZE	1024

struct node_topology {
	struct ihk_node_topology *saved;
	int mckernel_numa_id;
	char mckernel_numa_distance_s[NODE_DISTANCE_S_SIZE];
	cpumask_t cpumap;

	struct list_head chain;
};

struct process_list_item {
	int ready;
	struct task_struct *task;
	struct list_head list;
	wait_queue_head_t pli_wq;
};

struct mcctrl_part_exec {
	struct mutex lock;	
	int nr_processes;
	int nr_processes_left;
	cpumask_t cpus_used;
	struct list_head pli_list;
};

#define CPU_LONGS (((NR_CPUS) + (BITS_PER_LONG) - 1) / (BITS_PER_LONG))

#define MCCTRL_PER_PROC_DATA_HASH_SHIFT 7
#define MCCTRL_PER_PROC_DATA_HASH_SIZE (1 << MCCTRL_PER_PROC_DATA_HASH_SHIFT)
#define MCCTRL_PER_PROC_DATA_HASH_MASK (MCCTRL_PER_PROC_DATA_HASH_SIZE - 1)

struct mcctrl_usrdata {
	struct ihk_ikc_listen_param listen_param;
	struct ihk_ikc_listen_param listen_param2;
	ihk_os_t	os;
	int	num_channels;
	/* Channels used for sending messages to LWK */
	struct mcctrl_channel *channels;
	/* Channels used for receiving messages from LWK */
	struct ihk_ikc_channel_desc **ikc2linux;
	int	remaining_job;
	int	base_cpu;
	int	job_pos;
	int	mcctrl_dma_abort;
	struct mutex reserve_lock;
	unsigned long	last_thread_exec;
	wait_queue_head_t wq_procfs;
	struct list_head per_proc_data_hash[MCCTRL_PER_PROC_DATA_HASH_SIZE];
	rwlock_t per_proc_data_hash_lock[MCCTRL_PER_PROC_DATA_HASH_SIZE];

	void **keys;
	struct sysfsm_data sysfsm_data;
	unsigned long cpu_online[CPU_LONGS];
	struct ihk_cpu_info *cpu_info;
	struct ihk_mem_info *mem_info;
	nodemask_t numa_online;
	struct list_head cpu_topology_list;
	struct list_head node_topology_list;
	struct mcctrl_part_exec part_exec;
	int perf_event_num;
};

struct mcctrl_signal {
	int	cond;
	int	sig;
	int	pid;
	int	tid;
	char	info[128];
};

int mcctrl_ikc_send(ihk_os_t os, int cpu, struct ikc_scd_packet *pisp);
int mcctrl_ikc_send_msg(ihk_os_t os, int cpu, int msg, int ref, unsigned long arg);
int mcctrl_ikc_is_valid_thread(ihk_os_t os, int cpu);

ihk_os_t osnum_to_os(int n);

/* syscall.c */
void pager_add_process(void);
void pager_remove_process(struct mcctrl_per_proc_data *ppd);

int __do_in_kernel_syscall(ihk_os_t os, struct ikc_scd_packet *packet);
int mcctrl_add_per_proc_data(struct mcctrl_usrdata *ud, int pid, 
	struct mcctrl_per_proc_data *ppd);
int mcctrl_delete_per_proc_data(struct mcctrl_usrdata *ud, int pid);
struct mcctrl_per_proc_data *mcctrl_get_per_proc_data(
		struct mcctrl_usrdata *ud, int pid);
void mcctrl_put_per_proc_data(struct mcctrl_per_proc_data *ppd);

int mcctrl_add_per_thread_data(struct mcctrl_per_proc_data* ppd,
	struct task_struct *task, void *data);
int mcctrl_delete_per_thread_data(struct mcctrl_per_proc_data* ppd,
	struct task_struct *task);
inline struct mcctrl_per_thread_data *mcctrl_get_per_thread_data(
	struct mcctrl_per_proc_data *ppd, struct task_struct *task);

void __return_syscall(ihk_os_t os, struct ikc_scd_packet *packet, 
		long ret, int stid);

#define PROCFS_NAME_MAX 768

struct procfs_read {
	unsigned long pbuf;	/* physical address of the host buffer (request) */
	unsigned long offset;	/* offset to read (request) */
	int count;		/* bytes to read (request) */
	int eof;		/* if eof is detected, 1 otherwise 0. (answer)*/
	int ret;		/* read bytes (answer) */
	int status;		/* non-zero if done (answer) */
	int newcpu;		/* migrated new cpu (answer) */
	int readwrite;		/* 0:read, 1:write */
	char fname[PROCFS_NAME_MAX];	/* procfs filename (request) */
};

struct procfs_file {
	int status;			/* status of processing (answer) */
	int mode;			/* file mode (request) */
	char fname[PROCFS_NAME_MAX];	/* procfs filename (request) */
};

void procfs_answer(struct mcctrl_usrdata *ud, int pid);
int procfsm_packet_handler(void *os, int msg, int pid, unsigned long arg);
void add_tid_entry(int osnum, int pid, int tid);
void add_pid_entry(int osnum, int pid);
void delete_tid_entry(int osnum, int pid, int tid);
void delete_pid_entry(int osnum, int pid);
void proc_exe_link(int osnum, int pid, const char *path);
void procfs_init(int osnum);
void procfs_exit(int osnum);

/* sysfs_files.c */
void setup_sysfs_files(ihk_os_t os);
void reply_get_cpu_mapping(long req_pa);
void free_topology_info(ihk_os_t os);

/* archdep.c */
#define VDSO_MAXPAGES 2
struct vdso {
	long busy;
	int vdso_npages;
	char vvar_is_global;
	char hpet_is_global;
	char pvti_is_global;
	char padding;
	long vdso_physlist[VDSO_MAXPAGES];
	void *vvar_virt;
	long vvar_phys;
	void *hpet_virt;
	long hpet_phys;
	void *pvti_virt;
	long pvti_phys;
};

int reserve_user_space(struct mcctrl_usrdata *usrdata, unsigned long *startp,
		unsigned long *endp);
void get_vdso_info(ihk_os_t os, long vdso_pa);

struct get_cpu_mapping_req {
	int busy;		/* INOUT: */
	int error;		/* OUT: */
	long buf_rpa;		/* OUT: physical address of struct cpu_mapping */
	int buf_elems;		/* OUT: # of elements of buf */
	int padding;

	/* work for mcctrl */
	wait_queue_head_t wq;
};

struct ihk_perf_event_attr{
	unsigned long config; 
	unsigned disabled:1;
	unsigned pinned:1;
	unsigned exclude_user:1;
	unsigned exclude_kernel:1;
	unsigned exclude_hv:1;
	unsigned exclude_idle:1;
};

#endif
