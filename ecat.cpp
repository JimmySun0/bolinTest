#include <ecrt.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <malloc.h>
#include <math.h>
#include <ecrt.h>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sched.h>
#include <ecat.h>

using std::cout;
using std::endl;



#define FREQUENCY 1000
#define CLOCK_TO_USE CLOCK_MONOTONIC
//#define MEASURE_TIMING
#define SDO_ACCESSS 1
#define CONFIGURE_PDOS 1

/****************************************************************************/
#define NSEC_PER_SEC (1000000000L)
#define PERIOD_NS (NSEC_PER_SEC / FREQUENCY)
#define DIFF_NS(A, B) (((B).tv_sec - (A).tv_sec) * NSEC_PER_SEC + \
(B).tv_nsec - (A).tv_nsec)
#define TIMESPEC2NS(T) ((uint64_t) (T).tv_sec * NSEC_PER_SEC + (T).tv_nsec)

/****************************************************************************/
//assgment variables for ethercat master, slaves and domain etc
// EtherCAT
static ec_master_t *master = NULL;
static ec_master_state_t master_state = {};
static ec_domain_t *domain0 = NULL;
static ec_domain_state_t domain_state = {};
static ec_slave_config_t *sc_CDR = NULL;
/****************************************************************************/
// process data
static uint8_t *domain0_pd = NULL;
//ethrcat slave position, vendor and product ID
#define SLAVE_DRIVE_0   0, 0
#define CoolDrive0      0x00000153, 0x36483054

extern int run;
//
// offsets for PDO entries
static unsigned int off_cntlwd;
static unsigned int off_tarpos;
static unsigned int off_travel;
static unsigned int off_tartor;
static unsigned int off_opmod;
static unsigned int off_dumbyt1;
static unsigned int off_touprofun;

static unsigned int off_stawrd;
static unsigned int off_actpos;
static unsigned int off_actvel;
static unsigned int off_acttor;
static unsigned int off_moddis;
static unsigned int off_dumbyt2;
static unsigned int off_actfolerr;
static unsigned int off_diginput;
static unsigned int off_touprosta;
static unsigned int off_touproval;

//domain registration, you have to register domain so that you can send and receive PDO data
const static ec_pdo_entry_reg_t domain0_regs[] = {
{SLAVE_DRIVE_0, CoolDrive0, 0x6040, 0, &off_cntlwd}, // U16 0

{SLAVE_DRIVE_0, CoolDrive0, 0x607a, 0, &off_tarpos}, // S32 6

// {SLAVE_DRIVE_0, CoolDrive0, 0x60ff, 0, &off_travel}, // S32


{SLAVE_DRIVE_0, CoolDrive0, 0x6041, 0, &off_stawrd}, // U16 17
{SLAVE_DRIVE_0, CoolDrive0, 0x6064, 0, &off_actpos}, // U16 19

{}
};

static unsigned int counter = 0;
static unsigned int state_of_the_drive = 0;
static unsigned int sync_ref_counter = 0;
const struct timespec cycletime = {0, PERIOD_NS};
int32_t actualPosition = 0;
int32_t targetPosition = 0;
uint16_t controlWord;
uint16_t statusWord;

ec_pdo_entry_info_t slave_0_pdo_entries[] = {
    {0x6040, 0x00, 16},
    {0x607a, 0x00, 32},
    // {0x60ff, 0x00, 32},

    {0x6041, 0x00, 16},
    {0x6064, 0x00, 32},
};

ec_pdo_info_t slave_0_pdos[] = {
    {0x1602, 2, slave_0_pdo_entries + 0},
    // {0x1602, 3, slave_0_pdo_entries + 0},

    {0x1a02, 2, slave_0_pdo_entries + 2},
    // {0x1a02, 2, slave_0_pdo_entries + 3},

};

ec_sync_info_t slave_0_syncs[] = {
    {0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE},
    {1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE},
    {2, EC_DIR_OUTPUT, 1, slave_0_pdos + 0, EC_WD_ENABLE},
    {3, EC_DIR_INPUT, 1, slave_0_pdos + 1, EC_WD_DISABLE},
    {0xff}
};

/*****************************************************************************/
struct timespec timespec_add(struct timespec time1, struct timespec time2)
{
    struct timespec result;
    if ((time1.tv_nsec + time2.tv_nsec) >= NSEC_PER_SEC)
    {
        result.tv_sec = time1.tv_sec + time2.tv_sec + 1;
        result.tv_nsec = time1.tv_nsec + time2.tv_nsec - NSEC_PER_SEC;
    }
    else
    {
        result.tv_sec = time1.tv_sec + time2.tv_sec;
        result.tv_nsec = time1.tv_nsec + time2.tv_nsec;
    }
    return result;
}

//we need to check the state of domain , state of the domain should be equal to 2 when it exchanged the data
void check_domain_state()
{
    ec_domain_state_t ds;

    ecrt_domain_state(domain0, &ds);

    if (ds.working_counter != domain_state.working_counter)
        printf("\nDomain: WC %u.\n", ds.working_counter);
    if (ds.wc_state != domain_state.wc_state)
        printf("\nDomain: State %u.\n", ds.wc_state);

    domain_state = ds;
}


/*****************************************************************************/
//check the state of the master that it is in operation, link in up and no of slaves responding
void check_master_state(void)
{
    ec_master_state_t ms;
    ecrt_master_state(master, &ms);
    if (ms.slaves_responding != master_state.slaves_responding)
        printf("\n%u slave(s).\n", ms.slaves_responding);
    if (ms.al_states != master_state.al_states)
        printf("\nAL states: 0x%02X.\n", ms.al_states);
    if (ms.link_up != master_state.link_up)
        printf("\nLink is %s.\n", ms.link_up ? "up" : "down");
    master_state = ms;
}

void inline enable()
{
    if(0 == run)
    {
        EC_WRITE_U16(domain0_pd+off_cntlwd,SHUDOWN);
    }
    else if (statusWord&0x0008 == 0x0008)
    {
        EC_WRITE_U16(domain0_pd+off_cntlwd, FAULT_RESET);
    }
    else if (statusWord&0x004f == 0x0040 )
    {
        EC_WRITE_U16(domain0_pd+off_cntlwd, SHUDOWN );
    }
    else if (statusWord&0x006f == 0x0021)
    {
        EC_WRITE_U16(domain0_pd+off_cntlwd, SWITCH_ON );
    }
    else if( (statusWord&0x006f) == 0x0023  )   //switch on enable
	{
        EC_WRITE_S32(domain0_pd+off_tarpos, actualPosition);
        EC_WRITE_U16(domain0_pd+off_cntlwd, ENABLE_OPERATION );
    }
    else if (statusWord&0x006f == 0x0027)
    {
        EC_WRITE_U16(domain0_pd+off_cntlwd, 0x001f);
    }

}


//cyclic tast use to send and receive PDO data
void cyclic_task()
{
    struct timespec wakeupTime, time;


#ifdef MEASURE_TIMING
    struct timespec startTime, endTime, lastStartTime = {};
    uint32_t period_ns = 0, exec_ns = 0, latency_ns = 0,
             latency_min_ns = 0, latency_max_ns = 0,
             period_min_ns = 0, period_max_ns = 0,
             exec_min_ns = 0, exec_max_ns = 0;
#endif // MEASURE_TIMING

    // get current time
    clock_gettime(CLOCK_TO_USE, &wakeupTime); // use to get the clock time

    while(run) {
        wakeupTime = timespec_add(wakeupTime, cycletime);
        clock_nanosleep(CLOCK_TO_USE, TIMER_ABSTIME, &wakeupTime, NULL);

        // Write application time to master
        //
        // It is a good idea to use the target time (not the measured time) as
        // application time, because it is more stable.
        //
        ecrt_master_application_time(master, TIMESPEC2NS(wakeupTime));

#ifdef MEASURE_TIMING
        clock_gettime(CLOCK_TO_USE, &startTime);
        latency_ns = DIFF_NS(wakeupTime, startTime);
        period_ns = DIFF_NS(lastStartTime, startTime);
        exec_ns = DIFF_NS(lastStartTime, endTime);
        lastStartTime = startTime;

        if (latency_ns > latency_max_ns) {
            latency_max_ns = latency_ns;
        }
        if (latency_ns < latency_min_ns) {
            latency_min_ns = latency_ns;
        }
        if (period_ns > period_max_ns) {
            period_max_ns = period_ns;
        }
        if (period_ns < period_min_ns) {
            period_min_ns = period_ns;
        }
        if (exec_ns > exec_max_ns) {
            exec_max_ns = exec_ns;
        }
        if (exec_ns < exec_min_ns) {
            exec_min_ns = exec_ns;
        }
#endif

        ecrt_master_receive(master);
        ecrt_domain_process(domain0);

        // check process data state (optional)
        check_domain_state();

        // state_of_the_drive = EC_READ_U16(domain0_pd + off_stawrd);
        // actualPosition = EC_READ_S32(domain0_pd + off_actpos);

        if (counter) {
            counter--;
        } else { // do this at 1 Hz
            counter = FREQUENCY;

            // check for master state (optional)
            check_master_state();
            printf("")
            printf("actualPos:\t%d\t",actualPosition);

#ifdef MEASURE_TIMING
            // output timing stats
            printf("period     %10u ... %10u\n",
                    period_min_ns, period_max_ns);
            printf("exec       %10u ... %10u\n",
                    exec_min_ns, exec_max_ns);
            printf("latency    %10u ... %10u\n",
                    latency_min_ns, latency_max_ns);
            period_max_ns = 0;
            period_min_ns = 0xffffffff;
            exec_max_ns = 0;
            exec_min_ns = 0xffffffff;
            latency_max_ns = 0;
            latency_min_ns = 0xffffffff;
#endif
        }

        statusWord = EC_READ_U16(domain0_pd + off_stawrd);
        actualPosition = EC_READ_S32(domain0_pd + off_actpos);

        enable();

        EC_WRITE_S32(domain0_pd+off_tarpos, 100000*sin( 0.5*M_PI* (time.tv_sec + time.tv_nsec/1000000000.0) ) );
        
        // write application time to master
        clock_gettime(CLOCK_TO_USE, &time); //this command should be use for sync dc,


        // EC_WRITE_S32(domain0_pd+off_travel, sin( 2*M_PI* (time.tv_sec + time.tv_nsec/1000000000.0) ) );

        ecrt_master_application_time(master, TIMESPEC2NS(time)); //sync ethrcat master with current time

        if (sync_ref_counter) {
            sync_ref_counter--;
        } else {
            sync_ref_counter = 1; // sync every cycle

            clock_gettime(CLOCK_TO_USE, &time);
            ecrt_master_sync_reference_clock_to(master, TIMESPEC2NS(time));
        }
        ecrt_master_sync_slave_clocks(master);

        // send process data
        ecrt_domain_queue(domain0);
        ecrt_master_send(master);

#ifdef MEASURE_TIMING
        clock_gettime(CLOCK_TO_USE, &endTime);
#endif // MEASURE_TIMING
    }
}

void * ecat_task(void *arg)
{
    master = ecrt_request_master(0);
	printf("ecrt_request_master is called \n");
	if (!master)
    {
        printf("ecrt_request_master failed \n");
		return NULL;
    }

	domain0 = ecrt_master_create_domain(master);
	if(!domain0)
    {
        printf("ecrt_master_create_domain failed \n");
		return NULL;
    }

	if(!(sc_CDR = ecrt_master_slave_config(master, SLAVE_DRIVE_0, CoolDrive0)))
	{
		fprintf(stderr, "Failed to get slave configuration. \n");
        return NULL;
	}


#if SDO_ACCESSS
    if (ecrt_slave_config_sdo8(sc_CDR, 0x6060, 0, 8))  /*set the operation mode 8 RSP */
    {
        fprintf(stderr, "Failed to configure sdo. \n");
        return NULL;
    }
#endif

#if CONFIGURE_PDOS
	printf("Configuring PDOs...\n");
	if (ecrt_slave_config_pdos(sc_CDR, EC_END, slave_0_syncs))
	{
		fprintf(stderr, "Failed to configure PDOs.\n");
		return NULL;
	}
	printf("configureing PDO is completed!\n");
#endif

	if( ecrt_domain_reg_pdo_entry_list(domain0, domain0_regs))
	{
		fprintf(stderr, "PDO entty registration filed! \n");
		return NULL;
	}

    // configure SYNC signals for this slave
	ecrt_slave_config_dc(sc_CDR, 0x0300, PERIOD_NS, cycletime.tv_nsec/2, 0, 0);

	printf("Activating master...\n");
	if (ecrt_master_activate(master))
		return NULL;

    ecrt_master_reset(master);

	if( !(domain0_pd = ecrt_domain_data(domain0)))
	{
		return NULL;
	}

    printf("Starting cyclic function.\n");
	cyclic_task();
	ecrt_release_master(master);

	return NULL;
}



